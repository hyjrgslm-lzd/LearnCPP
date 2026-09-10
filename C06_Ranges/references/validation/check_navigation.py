"""Check C06 Markdown links and C06 links in the two authorized root indexes."""
from __future__ import annotations
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import sys
from urllib.parse import unquote, urlsplit

COURSE = Path(__file__).resolve().parents[2]
REPO = COURSE.parent
spec = importlib.util.spec_from_file_location(
    'course_navigation', REPO / 'C03_Type_Modeling_Interface_Design/references/validation/check_navigation.py')
shared = importlib.util.module_from_spec(spec)
spec.loader.exec_module(shared)


def links(text: str):
    # C++ lambdas inside inline code are not Markdown links.
    body = re.sub(r'(`+)(.*?)\1', '', shared.prose(text), flags=re.DOTALL)
    return shared.LINK.finditer(body)


def main() -> int:
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')
    assert [m.group(1) for m in links('`[](int x){}` [lesson](a.md)')] == ['a.md']
    assert not list(links('```cpp\n[](int x) {}\n```'))
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists():
        parser.error('choose a new evidence filename')
    sources = sorted(path for path in COURSE.rglob('*.md')
                     if not any(part == 'build' or part.startswith(('build-', '.'))
                                for part in path.relative_to(COURSE).parts))
    sources += [REPO / name for name in ('README.md', 'LEARNCPP_GLOBAL_PLAN.md')]
    failures, hashes, checked, excluded = [], {}, 0, 0
    for source in sources:
        raw = source.read_bytes()
        hashes[str(source.relative_to(REPO))] = hashlib.sha256(raw).hexdigest()
        for match in links(raw.decode('utf-8-sig')):
            link = match.group(1).strip().split(' "', 1)[0].strip('<>')
            if re.match(r'^(https?|mailto|app):|^//', link):
                continue
            parts = urlsplit(link)
            target = (source.parent / unquote(parts.path)).resolve() if parts.path else source
            if source.parent == REPO and not target.is_relative_to(COURSE):
                excluded += 1
                continue
            checked += 1
            error = ''
            if not target.is_relative_to(REPO):
                error = 'local link escapes repository'
            elif not target.exists():
                error = 'target missing'
            elif parts.fragment and target.suffix == '.md' and unquote(parts.fragment).lower() not in shared.anchors(target):
                error = 'anchor missing'
            if error:
                failures.append({'source': str(source.relative_to(REPO)), 'link': link, 'error': error})
    result = {'verdict': 'FAIL' if failures else 'PASS', 'documents': len(hashes),
              'links_checked': checked, 'failures': failures, 'sha256': hashes,
              'root_links_outside_c06': excluded,
              'scope': 'C06 non-build public Markdown and root links into C06; not teaching quality or web availability'}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open('x', encoding='utf-8') as output:
        json.dump(result, output, ensure_ascii=False, indent=2)
        output.write('\n')
    print(json.dumps({key: value for key, value in result.items() if key != 'sha256'}, ensure_ascii=False))
    return int(bool(failures))


if __name__ == '__main__':
    raise SystemExit(main())
