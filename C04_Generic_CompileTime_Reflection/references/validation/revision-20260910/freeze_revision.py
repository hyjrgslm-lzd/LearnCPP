"""Freeze this approved C04/C05 revision, respecting the repository's ignore rules."""
from __future__ import annotations
import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[4]
BASE = Path(__file__).resolve().parent
INDEX = ROOT / 'C04_Generic_CompileTime_Reflection/references/revision-delivery-manifest.md'
OUTPUT = BASE / 'delivery-final.json'
NAVIGATION = BASE / 'closeout-navigation-final.json'
sys.path.insert(0, str(ROOT / 'C03_Type_Modeling_Interface_Design/references/validation'))
from write_manifest import description


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def git(*args: str) -> str:
    return subprocess.run(['git', '-c', 'core.quotepath=false', *args], cwd=ROOT,
                          capture_output=True, text=True, encoding='utf-8', check=True, timeout=30).stdout


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--index-only', action='store_true', help='prepare stable index and a pending JSON before navigation validation')
    args = parser.parse_args()
    if OUTPUT.exists() and json.loads(OUTPUT.read_text(encoding='utf-8')).get('status') == 'FROZEN':
        parser.error('final snapshot is already frozen; do not overwrite historical delivery evidence')
    excluded = {INDEX, OUTPUT, NAVIGATION, BASE / 'delivery-readback.json'}
    names = set(git('ls-files', '--cached', '--others', '--exclude-standard', '--',
                    'C04_Generic_CompileTime_Reflection', 'C05_Data_Representation_Standard_Facilities').splitlines())
    names.update(('README.md', 'LEARNCPP_GLOBAL_PLAN.md'))
    before = {x['path']: x['sha256'] for x in json.loads((BASE / 'initial-scope.json').read_text(encoding='utf-8'))['files']}
    entries = []
    for name in sorted(names):
        path = ROOT / name
        if path in excluded or not path.is_file():
            continue
        data = path.read_bytes()
        sha = hashlib.sha256(data).hexdigest()
        summary = '有界依赖/实验/验证工具' if path.suffix in ('.py', '.ps1') else description(name)
        entries.append({'path': name, 'bytes': len(data), 'sha256': sha,
                        'text_lf_sha256': hashlib.sha256(data.replace(b'\r\n', b'\n')).hexdigest(),
                        'change': 'unchanged' if before.get(name) == sha else 'modified' if name in before else 'added',
                        'summary': summary})
    changed = [x for x in entries if x['change'] != 'unchanged']
    lines = ['# C04/C05 修订交付清单', '',
             f'范围：两课源码、文档、验证证据与约定根导航；冻结 {len(entries)} 个文件，其中相对初始快照新增/修改 {len(changed)} 个。',
             '本页逐文件列出本次变化；完整源文件SHA与LF归一化SHA见JSON。构建产物及下载的依赖checkout不作为源码交付。',
             '[完整冻结JSON](validation/revision-20260910/delivery-final.json)；导航检查记录为 `validation/revision-20260910/closeout-navigation-final.json`，其SHA绑定于冻结JSON。', '',
             'C06及其它并行改动未纳入。根README/全局计划是共享文件，整文件指纹不表示本任务独占创作。未提交、未推送。', '',
             '| 文件 | 变化 | 一行说明 |', '|---|---|---|']
    for item in changed:
        link = os.path.relpath(ROOT / item['path'], INDEX.parent).replace('\\', '/')
        lines.append(f"| [{item['path']}](<{link}>) | {item['change']} | {item['summary']} |")
    INDEX.write_text('\n'.join(lines) + '\n', encoding='utf-8')
    if args.index_only:
        if not OUTPUT.exists():
            OUTPUT.write_text('{"status":"PENDING"}\n', encoding='utf-8')
        print(json.dumps({'index_files': len(entries), 'changed': len(changed), 'status': 'INDEX_READY'}))
        return 0
    navigation = json.loads(NAVIGATION.read_text(encoding='utf-8'))
    if navigation['verdict'] != 'PASS':
        raise RuntimeError('navigation must pass before freezing')
    binaries = []
    for folder in sorted((ROOT / 'build').glob('c0[45]-revision-*')):
        for path in sorted(folder.rglob('*.exe')):
            if 'CMakeFiles' in path.parts or not path.name.startswith(('L', 'A', 'U', 'P', 'F', 'B')):
                continue
            binaries.append({'path': path.relative_to(ROOT).as_posix(), 'bytes': path.stat().st_size, 'sha256': digest(path)})
    shared = [ROOT / p for p in ('C01_Build_Compile_Link/exercises/include/check.hpp',
              'C01_Build_Compile_Link/exercises/tools/process_runner.py',
              'C02_Objects_Lifetime_Ownership/exercises/tools/record_process.py',
              'C02_Objects_Lifetime_Ownership/exercises/tools/audit_student.py',
              'C03_Type_Modeling_Interface_Design/references/validation/check_navigation.py',
              'C03_Type_Modeling_Interface_Design/references/validation/write_manifest.py')]
    result = {'status': 'FROZEN', 'recorded_utc': datetime.now(timezone.utc).isoformat(),
              'head': git('rev-parse', 'HEAD').strip(), 'file_count': len(entries), 'changed_file_count': len(changed),
              'files': entries, 'shared_read_only_inputs': {p.relative_to(ROOT).as_posix(): digest(p) for p in shared},
              'local_binary_metadata': binaries, 'navigation_sha256': digest(NAVIGATION), 'index_sha256': digest(INDEX),
              'boundaries': ['Index, this JSON, navigation output and readback output are excluded from recursive file hashes.',
                             'Index/navigation hashes are separately bound above.',
                             'Binaries are local metadata only; test verdicts are in the cited process/JUnit records.',
                             'Existing gitignore excludes build trees, binary artifacts and dependency checkouts.',
                             'C06 and other parallel work are outside this delivery scope.']}
    OUTPUT.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps({'status': 'FROZEN', 'files': len(entries), 'changed': len(changed), 'binaries': len(binaries)}))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
