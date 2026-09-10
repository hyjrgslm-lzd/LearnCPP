"""Check the two-course revision's Markdown links using the existing C03 parser."""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
from urllib.parse import unquote, urlsplit

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT / 'C03_Type_Modeling_Interface_Design/references/validation'))
from check_navigation import LINK, prose, anchors


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists():
        parser.error('choose a new evidence filename')
    courses = ['C04_Generic_CompileTime_Reflection', 'C05_Data_Representation_Standard_Facilities']
    listed = subprocess.run(['git', '-c', 'core.quotepath=false', 'ls-files', '--cached',
                             '--others', '--exclude-standard', '--', *courses], cwd=ROOT,
                            capture_output=True, text=True, encoding='utf-8', check=True, timeout=30)
    files = sorted({ROOT / p for p in listed.stdout.splitlines() if p.endswith('.md')} |
                   {ROOT / 'README.md', ROOT / 'LEARNCPP_GLOBAL_PLAN.md'})
    failures, external, cached = [], [], {}
    hashes, checked = {}, 0
    for source in files:
        hashes[source.relative_to(ROOT).as_posix()] = hashlib.sha256(source.read_bytes()).hexdigest()
        # operator[](i) inside a code span is not a Markdown link.
        body = re.sub(r'(`+)[^\n]*?\1', ' ', prose(source.read_text(encoding='utf-8-sig')))
        for match in LINK.finditer(body):
            raw = match.group(1).strip().split(' "', 1)[0].strip('<>')
            if re.match(r'^(https?|mailto|app):|^//', raw):
                continue
            if re.match(r'^[A-Za-z]:[/\\]', raw):
                path_text, _, fragment = raw.partition('#')
            else:
                parts = urlsplit(raw)
                path_text, fragment = parts.path, parts.fragment
            path_text = re.sub(r':\d+$', '', unquote(path_text))
            target = (source.parent / path_text).resolve() if path_text else source
            entry = {'source': source.relative_to(ROOT).as_posix(), 'target': raw}
            if not target.is_relative_to(ROOT):
                external.append(entry)
                continue
            checked += 1
            if not target.exists():
                failures.append({**entry, 'error': 'missing target'})
            elif fragment and target.suffix == '.md':
                if target not in cached:
                    cached[target] = anchors(target)
                if unquote(fragment).lower() not in cached[target]:
                    failures.append({**entry, 'error': 'missing anchor'})
    result = {'verdict': 'FAIL' if failures else 'PASS', 'documents': len(files),
              'local_links_checked': checked, 'failures': failures, 'external_local_links': external,
              'sha256': hashes, 'scope': 'repo targets/anchors and editor :line links; not web validation'}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps({k: v for k, v in result.items() if k != 'sha256'}, ensure_ascii=False))
    return bool(failures)


if __name__ == '__main__':
    raise SystemExit(main())
