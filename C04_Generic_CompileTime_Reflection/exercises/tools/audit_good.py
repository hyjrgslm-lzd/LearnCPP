"""Check selected good targets and a fresh include trace for answer dependencies.

This checks wiring, not authorship or algorithmic independence. The latter needs
the separately recorded blind implementation and teaching review.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import re
import sys

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parents[3] /
                       'C02_Objects_Lifetime_Ownership/exercises/tools'))
from audit_student import read_json, reference_path


def audit(build: Path, config: str, trace_path: Path) -> dict:
    reply = build / '.cmake/api/v1/reply'
    index = read_json(sorted(reply.glob('index-*.json'))[-1])
    model_ref = next(x for x in index['objects'] if x['kind'] == 'codemodel')
    model = read_json(reply / model_ref['jsonFile'])
    configuration = next(x for x in model['configurations'] if x['name'] == config)
    targets = {x['id']: read_json(reply / x['jsonFile']) for x in configuration['targets']}
    good = [x for x in targets.values() if x['name'].endswith('_validation_good')]
    failures = [] if good else ['no good targets found']
    names = sorted(x['name'] for x in good)
    pending, visited = [x['id'] for x in good], set()
    while pending:
        identity = pending.pop()
        if identity in visited:
            continue
        visited.add(identity)
        target = targets[identity]
        if '_reference' in target['name']:
            failures.append('Reference build dependency: ' + target['name'])
        pending.extend(x['id'] for x in target.get('dependencies', []))
        paths = [x['path'] for x in target.get('sources', [])]
        paths += [x['path'] for group in target.get('compileGroups', [])
                  for x in group.get('includes', [])]
        paths += [x['fragment'] for x in target.get('link', {}).get('commandFragments', [])]
        failures += ['Reference target input: ' + p for p in paths if reference_path(p)]
    trace = read_json(trace_path)
    if trace.get('status') != 'PASS' or trace.get('exit_code') != 0:
        failures.append('include-trace build failed')
    command = trace.get('command', [])
    if '--build' not in command or (Path(trace['cwd']) /
            command[command.index('--build') + 1]).resolve() != build.resolve():
        failures.append('trace belongs to a different build')
    if '--config' not in command or command[command.index('--config') + 1] != config:
        failures.append('trace configuration mismatch')
    if '--clean-first' not in command or '--target' not in command or not set(names).issubset(command):
        failures.append('trace must cleanly rebuild every good target explicitly')
    includes = []
    for line in (trace.get('stdout', '') + '\n' + trace.get('stderr', '')).splitlines():
        match = re.search(r'(?:including file|包含文件):\s*(.+)$', line, re.I)
        if match:
            includes.append(match.group(1).strip())
    if not includes:
        failures.append('actual MSVC include trace absent')
    failures += ['Reference in actual include trace: ' + p for p in includes if reference_path(p)]
    return {'verdict': 'FAIL' if failures else 'PASS', 'failures': sorted(set(failures)),
            'targets': names, 'actual_include_count': len(includes),
            'trace_sha256': hashlib.sha256(trace_path.read_bytes()).hexdigest(),
            'scope': 'build inputs and actual includes; not proof of independent authorship'}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', type=Path, required=True)
    parser.add_argument('--config', default='Release')
    parser.add_argument('--trace', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists():
        parser.error('choose a new evidence filename')
    try:
        result = audit(args.build, args.config, args.trace)
    except (OSError, ValueError, KeyError, IndexError, StopIteration, TypeError) as error:
        result = {'verdict': 'FAIL', 'failures': [f'audit input error: {error}']}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open('x', encoding='utf-8') as output:
        json.dump(result, output, ensure_ascii=False, indent=2)
    print(result['verdict'], args.output)
    return int(result['verdict'] != 'PASS')


if __name__ == '__main__':
    raise SystemExit(main())
