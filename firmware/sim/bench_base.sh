#!/bin/sh
# Pooled latency over several seeds, run in parallel:
#   ./bench.sh <bundle> <scenario> <len> <presses> <seeds>
# Prints p50/p99/max per depth and lost presses.
b=$1 sc=$2 len=$3 np=$4 seeds=${5:-8}
out=$(mktemp)
seq 1 "$seeds" | xargs -P 8 -I{} sh -c "SEED={} LAT_DUMP=$out ./sim_base $b $sc $len $np > /dev/null"
python3 - "$out" <<'P'
import sys, collections
d = collections.defaultdict(list); lost = collections.Counter()
for line in open(sys.argv[1]):
    k, v = line.split(); k = int(k); v = float(v)
    if v < 0: lost[k] += 1
    else: d[k].append(v)
for k in sorted(set(d) | set(lost)):
    v = sorted(d[k]); n = len(v)
    q = lambda p: v[min(n - 1, int(n * p))] if n else 0
    print(f"  depth {k}: n={n + lost[k]:4d} lost={lost[k]}  p50={q(.5):6.1f}  p90={q(.9):6.1f}  p99={q(.99):6.1f}  max={v[-1] if n else 0:6.1f} ms")
P
rm -f "$out"
