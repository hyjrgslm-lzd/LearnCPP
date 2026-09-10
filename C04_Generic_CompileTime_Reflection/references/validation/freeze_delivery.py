"""Bind a delivery to source/evidence bytes and local compiler command records."""
from __future__ import annotations
import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import subprocess
from check_navigation import BRIDGES

COURSE = Path(__file__).resolve().parents[2]
REPO = COURSE.parent


def git(*args: str) -> list[str]:
    return subprocess.run(['git', '-c', 'core.quotepath=false', *args], cwd=REPO,
                          text=True, encoding='utf-8', capture_output=True, check=True,
                          timeout=30).stdout.splitlines()


def purpose(path: str) -> str:
    if not path.startswith(COURSE.name + '/'):
        return '组织约束、课程导航或实施状态'
    for token, label in (('/src/student/', '独立学生起点'), ('/src/reference/', '完整参考实现'),
                         ('/validation/good/', '独立正确完成体'), ('/validation/bad', '错误实现控制'),
                         ('/chapters/', '教学正文与机制推导'), ('/checks/', '契约检查'),
                         ('/references/validation/', '原始验证证据或验证工具'),
                         ('/references/benchmarks/', '原始成本数据及分析'),
                         ('/references/reviews/', '非作者审查与修复复验')):
        if token in path:
            return label
    return '课程说明、示例、构建或实验工具'


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--tag', required=True)
    args = parser.parse_args()
    if not args.tag.replace('-', '').isalnum():
        parser.error('tag must be alphanumeric with optional hyphens')
    output = COURSE / 'references/validation' / f'delivery-{args.tag}.json'
    index = COURSE / 'references/delivery-manifest.md'
    if output.exists():
        parser.error('use a new snapshot tag')
    allowed = {'README.md', 'LEARNCPP_GLOBAL_PLAN.md', 'CONTENT_REFACTORING_GUIDE.md'}
    allowed.update(folder + '/README.md' for folder in BRIDGES)
    modified = set(git('diff', '--name-only')) | set(git('diff', '--cached', '--name-only'))
    unexpected = {p for p in modified if p not in allowed and not p.startswith(COURSE.name + '/')}
    paths = set(git('ls-files', '--', COURSE.name)) | set(git('ls-files', '--others', '--exclude-standard', '--', COURSE.name))
    paths |= modified - unexpected
    paths -= {str(output.relative_to(REPO)).replace('\\', '/'), str(index.relative_to(REPO)).replace('\\', '/')}
    files = []
    for relative in sorted(paths):
        data = (REPO / relative).read_bytes()
        files.append({'path': relative, 'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest(),
                      'lf_sha256': hashlib.sha256(data.replace(b'\r\n', b'\n')).hexdigest(), 'summary': purpose(relative)})
    binaries, commands = [], []
    for preset in ('verify-core', 'verify-debug', 'student', 'asan', 'frontier'):
        build = COURSE / 'exercises/build' / preset
        for path in sorted(build.rglob('*.exe')):
            if 'CMakeFiles' not in path.parts:
                binaries.append({'path': str(path.relative_to(REPO)), 'bytes': path.stat().st_size,
                                 'sha256': hashlib.sha256(path.read_bytes()).hexdigest()})
        for path in sorted(build.rglob('*.command.1.tlog')):
            if 'CMakeFiles' in path.parts:
                continue
            data = path.read_bytes()
            commands.append({'path': str(path.relative_to(REPO)), 'sha256': hashlib.sha256(data).hexdigest(),
                             'command': data.decode('utf-16' if data.startswith((b'\xff\xfe', b'\xfe\xff')) else 'utf-8', 'replace')})
    result = {'head': git('rev-parse', 'HEAD')[0], 'recorded_utc': datetime.now(timezone.utc).isoformat(),
              'files': files, 'local_binaries': binaries, 'compiler_command_records': commands,
              'preserved_out_of_scope_tracked_paths': sorted(unexpected),
              'boundaries': ['Current snapshot and manifest exclude themselves.',
                             'Local binary/tlog metadata is not a claim that build artifacts are shipped.',
                             'Verdicts come from process, CTest and review evidence, not file existence.',
                             'Concurrent C05 work is outside this delivery; shared navigation hashes bind complete current files, not exclusive authorship.']}
    output.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
    lines = ['# C04 交付文件与冻结指纹', '', f'快照 `{args.tag}`，{len(files)} 个文件。',
             f'[原始SHA、LF归一化SHA及本机编译命令](validation/{output.name})。',
             '二进制仅记录本机元数据，不随源码交付。历史失败记录保留原义。', '',
             '| 文件 | 一行用途 |', '|---|---|']
    for entry in files:
        relative = os.path.relpath(REPO / entry['path'], index.parent).replace('\\', '/')
        lines.append(f"| [{entry['path']}](<{relative}>) | {entry['summary']} |")
    index.write_text('\n'.join(lines) + '\n', encoding='utf-8')
    print(json.dumps({'files': len(files), 'binary_metadata': len(binaries), 'commands': len(commands),
                      'output': str(output)}, ensure_ascii=False))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
