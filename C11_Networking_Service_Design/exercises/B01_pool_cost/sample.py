"""One warmup and five fresh-process samples, with external timeout/cleanup."""
import argparse,json,statistics,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[3]/'C01_Build_Compile_Link/exercises/tools'))
from process_runner import run_process
p=argparse.ArgumentParser();p.add_argument('executable',type=Path);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
if a.output.exists():p.error('use a fresh output file')
records=[]
for mode in ('fresh','pool'):
    for index in range(6):
        result=run_process([str(a.executable.resolve()),mode,'100'],timeout=30)
        records.append({'mode':mode,'warmup':index==0,'result':result})
        # Preserve failures before making any comparison.
        a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(records,indent=2),encoding='utf-8')
        if result['exit_code']!=0 or result.get('timeout') or result.get('cleanup_error'):raise SystemExit('sample failed; inspect output')
for mode in ('fresh','pool'):
    rows=[json.loads(r['result']['stdout']) for r in records if r['mode']==mode and not r['warmup']]
    print(mode,{key:statistics.median(r[key] for r in rows) for key in ('connections','acquire_us','transfer_us','work_us','drain_us')})
