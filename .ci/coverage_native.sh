#!/usr/bin/env bash
# Run native unit tests with coverage; require >=95% on lines, functions, regions, and
# branches for each firmware/core|metering *_logic.cpp unit (aggregate totals too).
# Linux: gcc gcov + gcovr. macOS: Clang llvm-cov (profraw).
set -euo pipefail

MIN="${MIN_COVERAGE:-95}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

PIO="${PIO:-pio}"
if ! command -v "$PIO" >/dev/null 2>&1; then
  if [[ -x "${ROOT}/.venv/bin/pio" ]]; then
    PIO="${ROOT}/.venv/bin/pio"
  fi
fi

GCOVR="${GCOVR:-gcovr}"
if ! command -v "$GCOVR" >/dev/null 2>&1 && [[ -x "${ROOT}/.venv/bin/gcovr" ]]; then
  GCOVR="${ROOT}/.venv/bin/gcovr"
fi

BUILD_DIR="${ROOT}/.pio/build/native-coverage"
OUT_DIR="${ROOT}/firmware/coverage"
mkdir -p "$OUT_DIR"
SUMMARY="${OUT_DIR}/gcovr-summary.txt"

mkdir -p "$BUILD_DIR"
echo "coverage_native: pio test -e native-coverage"
"$PIO" test -e native-coverage

audit_llvm_summary() {
  python3 - "$MIN" "$SUMMARY" <<'PY'
import re
import sys

minimum = float(sys.argv[1])
text = open(sys.argv[2]).read()
pat = re.compile(
    r'^(firmware/(?:core|metering)/\S+_logic\.cpp)\s+'
    r'(\d+)\s+(\d+)\s+([\d.]+)%\s+'
    r'(\d+)\s+(\d+)\s+([\d.]+)%\s+'
    r'(\d+)\s+(\d+)\s+([\d.]+)%\s+'
    r'(\d+)\s+(\d+)\s+([\d.]+)%'
)
tot = [0, 0, 0, 0, 0, 0, 0, 0]
bad = []
for line in text.splitlines():
    m = pat.match(line)
    if not m:
        continue
    region = float(m.group(4))
    func = float(m.group(7))
    linep = float(m.group(10))
    branch = float(m.group(13))
    for name, v in (
        ("region", region),
        ("function", func),
        ("line", linep),
        ("branch", branch),
    ):
        if v < minimum:
            bad.append((m.group(1), name, v))
    pairs = (
        (int(m.group(2)), int(m.group(3))),
        (int(m.group(5)), int(m.group(6))),
        (int(m.group(8)), int(m.group(9))),
        (int(m.group(11)), int(m.group(12))),
    )  # region, function, line, branch totals/missed
    for i, (a, b) in enumerate(pairs):
        tot[i * 2] += a
        tot[i * 2 + 1] += b

def pct(exec_m, total):
    return 100.0 if total == 0 else 100.0 * (total - exec_m) / total

labels = ("region", "function", "line", "branch")
print(f"coverage_native: aggregate minimum {minimum}%")
for i, label in enumerate(labels):
    total, missed = tot[i * 2], tot[i * 2 + 1]
    cov = pct(missed, total)
    print(f"  {label}: {cov:.2f}%")
    if cov < minimum:
        bad.append((f"(aggregate {label})", label, cov))

# Gate: per-file line/function/branch >= MIN; aggregate line/function/region/branch >= MIN.
file_bad = [(f, n, v) for f, n, v in bad if n in ("line", "function", "branch")]
agg_bad = [(f, n, v) for f, n, v in bad if f.startswith("(aggregate")]
if file_bad or agg_bad:
    print(f"coverage_native: coverage below {minimum}%:", file=sys.stderr)
    for f, name, v in file_bad + agg_bad:
        print(f"  {f} {name}={v:.2f}%", file=sys.stderr)
    sys.exit(1)
PY
}

if [[ "$(uname -s)" == "Darwin" ]]; then
  PROGRAM="${BUILD_DIR}/program"
  if [[ ! -x "$PROGRAM" ]]; then
    echo "coverage_native: missing ${PROGRAM} (native-coverage build failed?)" >&2
    exit 1
  fi

  profraws=()
  while IFS= read -r f; do
    profraws+=("$f")
  done < <(find "$ROOT" "$BUILD_DIR" -maxdepth 2 -name '*.profraw' 2>/dev/null | sort -u)
  if [[ ${#profraws[@]} -eq 0 ]]; then
    echo "coverage_native: no .profraw files found (run native-coverage tests first)" >&2
    exit 1
  fi

  PROFDATA="${OUT_DIR}/native.profdata"
  echo "coverage_native: llvm-profdata merge (${#profraws[@]} profile(s))"
  xcrun llvm-profdata merge -sparse "${profraws[@]}" -o "$PROFDATA"

  echo "coverage_native: coverage report firmware/*/*_logic.cpp (llvm-cov)"
  xcrun llvm-cov report "$PROGRAM" -instr-profile="$PROFDATA" 2>/dev/null \
    | grep -E 'firmware/(core|metering)/.*_logic\.cpp' \
    | tee "$SUMMARY"
  audit_llvm_summary
else
  if ! command -v "$GCOVR" >/dev/null 2>&1; then
    echo "coverage_native: gcovr is required (apt install gcovr or pip install gcovr)" >&2
    exit 1
  fi

  echo "coverage_native: gcovr report firmware/*/*_logic.cpp"
  set +e
  "$GCOVR" -r "$ROOT" \
    --object-directory "$BUILD_DIR" \
    --filter "firmware/(core|metering)/.*_logic\\.cpp" \
    --txt-metric line \
    --txt-metric branch \
    --fail-under-line "$MIN" \
    | tee "$SUMMARY"
  status=${PIPESTATUS[0]}
  set -e
  if [[ "$status" -ne 0 ]]; then
    echo "coverage_native: gcovr below ${MIN}% line or branch (see ${SUMMARY})" >&2
    exit 1
  fi
fi

echo "coverage_native: all *_logic.cpp units at >=${MIN}% (line, function, region, branch)"
