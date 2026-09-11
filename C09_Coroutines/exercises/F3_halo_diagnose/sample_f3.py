import argparse
import random
import statistics
import subprocess
import sys


def run(exe, version, n, iterations):
    cp = subprocess.run(
        [exe, "--bench", version, str(n), str(iterations)],
        text=True,
        encoding="utf-8",
        errors="replace",
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        timeout=30,
    )
    if cp.returncode:
        raise RuntimeError(cp.stderr.strip() or cp.stdout.strip())
    fields = dict(item.split("=", 1) for item in cp.stdout.strip().split(",") if "=" in item)
    return float(fields["ns_per_iter"]), cp.stdout.strip()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("exe")
    parser.add_argument("--n", type=int, default=32)
    parser.add_argument("--iterations", type=int, default=200000)
    parser.add_argument("--seed", type=int, default=20260911)
    args = parser.parse_args()

    rng = random.Random(args.seed)
    versions = ["local", "escaped"]
    print(f"# protocol seed={args.seed} n={args.n} measured_iterations={args.iterations} warmup_iterations=1000 samples=5")
    for version in versions:
        _, raw = run(args.exe, version, args.n, 1000)
        print(f"warmup,{raw}")

    samples = {version: [] for version in versions}
    for sample in range(1, 6):
        order = versions[:]
        rng.shuffle(order)
        for version in order:
            ns, raw = run(args.exe, version, args.n, args.iterations)
            samples[version].append(ns)
            print(f"sample={sample},{raw}")

    for version in versions:
        vals = samples[version]
        print(
            f"summary,version={version},median={statistics.median(vals):.3f},"
            f"min={min(vals):.3f},max={max(vals):.3f},samples={';'.join(f'{v:.3f}' for v in vals)}"
        )


if __name__ == "__main__":
    try:
        main()
    except Exception as exc:
        print(f"sample failed: {exc}", file=sys.stderr)
        raise SystemExit(1)
