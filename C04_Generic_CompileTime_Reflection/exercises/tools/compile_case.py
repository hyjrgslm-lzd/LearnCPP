"""Compile a positive control, then require the intended semantic rejection."""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import re
import sys

sys.dont_write_bytecode = True
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
sys.stderr.reconfigure(encoding='utf-8', errors='replace')
REPO = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(REPO / 'C01_Build_Compile_Link/exercises/tools'))
from process_runner import run_process


def diagnostic_messages(text: str) -> str:
    """Exclude include traces and source paths from semantic pattern matching."""
    messages = []
    for line in text.splitlines():
        match = re.search(r'(?:fatal\s+)?error(?:\s+[A-Z]+\d+)?\s*:\s*.*', line, re.I)
        if match:
            messages.append(match.group(0).split(' [', 1)[0])
    return '\n'.join(messages)


def is_infrastructure_failure(text: str) -> bool:
    return re.search(
        r'(?:(?:fatal\s+)?error\s+C(?:1001|1060|1076|1083)|error\s+(?:MSB80\d+|LNK\d+)|'
        r'internal compiler error|内部编译器错误|PLEASE submit a bug report|frontend command failed due to signal)',
        text, re.I) is not None


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    for key in ('source', 'control', 'build'):
        parser.add_argument('--' + key, type=Path, required=True)
    for key in ('pattern', 'generator', 'platform', 'config', 'compiler'):
        parser.add_argument('--' + key, required=True)
    parser.add_argument('--setup', type=Path,
                        help='trusted course CMake file configuring control/subject dependencies')
    args = parser.parse_args()
    args.build.mkdir(parents=True, exist_ok=True)
    case_source = args.build / 'source'
    case_source.mkdir(exist_ok=True)
    setup = Path(__file__).resolve().parents[1] / 'cmake/StudySetup.cmake'
    extra_setup = ''
    if args.setup:
        if not args.setup.is_file():
            parser.error(f'missing diagnostic setup: {args.setup}')
        extra_setup = f'include("{args.setup.resolve().as_posix()}")\n'
    inputs = [args.source, args.control, Path(__file__), setup]
    if args.setup:
        inputs.append(args.setup)
    input_hashes = {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs}
    (case_source / 'CMakeLists.txt').write_text(
        'cmake_minimum_required(VERSION 3.28)\nproject(c04_diagnostic LANGUAGES CXX)\n'
        f'include("{setup.as_posix()}")\n'
        f'add_executable(control "{args.control.resolve().as_posix()}")\n'
        f'add_executable(subject "{args.source.resolve().as_posix()}")\n'
        'c04_configure_target(control)\nc04_configure_target(subject)\n' + extra_setup +
        'if(MSVC)\n  target_compile_options(control PRIVATE /showIncludes)\n'
        '  target_compile_options(subject PRIVATE /showIncludes)\nendif()\n', encoding='utf-8')
    binary = args.build / 'binary'
    command = ['cmake', '-S', str(case_source), '-B', str(binary), '-G', args.generator,
               '-DBUILD_TESTING=OFF', f'-DCMAKE_CXX_COMPILER={args.compiler}']
    if args.platform:
        command += ['-A', args.platform]
    records = {'configure': run_process(command, 60)}
    reason = 'configuration must succeed'
    accepted = False
    if records['configure']['status'] == 'PASS':
        records['control'] = run_process(['cmake', '--build', str(binary), '--config', args.config,
                                         '--target', 'control', '--clean-first'], 180)
        reason = 'positive control must compile and link'
        if records['control']['status'] == 'PASS':
            subject = run_process(['cmake', '--build', str(binary), '--config', args.config,
                                   '--target', 'subject'], 180)
            records['subject'] = subject
            text = subject['stdout'] + '\n' + subject['stderr']
            records['semantic_diagnostics'] = diagnostic_messages(text)
            infrastructure = is_infrastructure_failure(text)
            accepted = (subject['exit_code'] is not None and subject['exit_code'] != 0
                        and not subject['timeout'] and not subject['error']
                        and not subject['cleanup_error'] and not infrastructure
                        and re.search(args.pattern, records['semantic_diagnostics']) is not None)
            reason = 'expected semantic diagnostic with a normal nonzero compiler exit'
    changed_inputs = [str(p) for p in inputs if hashlib.sha256(p.read_bytes()).hexdigest() != input_hashes[str(p)]]
    if changed_inputs:
        accepted, reason = False, 'diagnostic inputs changed during the run'
    local_headers = {}
    for result in (records.get('control', {}), records.get('subject', {})):
        for line in (result.get('stdout', '') + '\n' + result.get('stderr', '')).splitlines():
            match = re.search(r'(?:including file|包含文件):\s*(.+)$', line, re.I)
            if match:
                path = Path(match.group(1).strip()).resolve()
                if path.is_file() and path.is_relative_to(REPO):
                    local_headers[str(path)] = hashlib.sha256(path.read_bytes()).hexdigest()
    records.update(verdict='PASS' if accepted else 'FAIL', reason=reason,
                   subject_source=str(args.source), control_source=str(args.control), pattern=args.pattern,
                   compiler=args.compiler,
                   input_sha256=input_hashes, changed_inputs=changed_inputs,
                   local_include_sha256=local_headers)
    # Run history is append-only, even inside an ignored build tree.
    serial = 1
    while (args.build / f'evidence-{serial}.json').exists():
        serial += 1
    evidence = args.build / f'evidence-{serial}.json'
    evidence.write_text(json.dumps(records, ensure_ascii=False, indent=2), encoding='utf-8')
    print(f"{records['verdict']}: {reason}; {evidence}")
    if not accepted:
        for key in ('configure', 'control', 'subject'):
            if key in records:
                print(records[key]['stdout'] + records[key]['stderr'])
    return 0 if accepted else 1


if __name__ == '__main__':
    raise SystemExit(main())
