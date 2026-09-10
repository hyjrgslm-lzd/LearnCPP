"""Validate C04 links and the explicitly authorized navigation edits."""
from __future__ import annotations
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import re
from urllib.parse import unquote, urlsplit

COURSE = Path(__file__).resolve().parents[2]
REPO = COURSE.parent
BRIDGES = ('C01_Build_Compile_Link', 'C02_Objects_Lifetime_Ownership',
           'C03_Type_Modeling_Interface_Design', 'C06_Ranges', 'C09_Coroutines',
           'C10_Execution', 'C14_GPU', 'C15_Unreal_Engine')
spec = importlib.util.spec_from_file_location('course_navigation',
    REPO / 'C03_Type_Modeling_Interface_Design/references/validation/check_navigation.py')
shared = importlib.util.module_from_spec(spec)
spec.loader.exec_module(shared)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists():
        parser.error('use a new evidence filename')
    files = sorted(p for p in COURSE.rglob('*.md')
                   if not any(part == 'build' or part.startswith('build-') for part in p.parts))
    files += [REPO / name for name in ('README.md', 'LEARNCPP_GLOBAL_PLAN.md', 'CONTENT_REFACTORING_GUIDE.md')]
    # Old course bodies are out of scope: validate the newly added bridge itself.
    bridge_files = [REPO / folder / 'README.md' for folder in BRIDGES]
    failures, inputs, excluded, count = [], {}, [], 0
    for source in files + bridge_files:
        raw = source.read_bytes()
        inputs[str(source.relative_to(REPO))] = hashlib.sha256(raw).hexdigest()
        body = raw.decode('utf-8-sig')
        if source in bridge_files:
            body = body.split('## C04 泛型与编译期桥接', 1)[-1]
            body = body.split('\n## ', 1)[0]
        for match in shared.LINK.finditer(shared.prose(body)):
            link = match.group(1).strip().split(' "', 1)[0].strip('<>')
            if re.match(r'^(https?|mailto|app):|^//', link):
                continue
            parts = urlsplit(link)
            target = (source.parent / unquote(parts.path)).resolve() if parts.path else source
            if not source.is_relative_to(COURSE) and target.is_relative_to(REPO / 'C05_Data_Representation_Standard_Facilities'):
                excluded.append({'source': str(source.relative_to(REPO)), 'link': link,
                                 'exists': target.exists(), 'reason': 'concurrent C05 work outside C04 scope'})
                continue
            count += 1
            error = ''
            if not target.is_relative_to(REPO):
                error = 'local link escapes repository'
            elif not target.exists():
                error = 'target missing'
            elif parts.fragment and target.suffix == '.md' and unquote(parts.fragment).lower() not in shared.anchors(target):
                error = 'anchor missing'
            if error:
                failures.append({'source': str(source.relative_to(REPO)), 'link': link, 'error': error})
    result = {'verdict': 'FAIL' if failures else 'PASS', 'documents': len(inputs),
              'links_checked': count, 'failures': failures, 'sha256': inputs,
              'excluded_concurrent_links': excluded,
              'scope': 'C04 docs, root navigation and new bridges; concurrent C05 links reported separately; no teaching-quality or web-link claim'}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps({k:v for k,v in result.items() if k != 'sha256'}, ensure_ascii=False))
    return int(bool(failures))


if __name__ == '__main__':
    raise SystemExit(main())
