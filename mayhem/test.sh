#!/usr/bin/env bash
#
# mayhem/test.sh — run the known-answer self-tests that mayhem/build.sh compiled (normal flags, no
# sanitizers). They check json-c, Mini-XML, PCRE2, miniz, Zydis and TLSH (the libraries System Informer
# vendors) against hard-coded expected values; the exact number of checks is pinned so a neutered
# binary that prints nothing (or exits 0 early) cannot pass.
set -uo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
: "${SRC:=/mayhem}"
cd "$SRC"

emit_ctrf() {
  local tool="$1" passed="$2" failed="$3" skipped="${4:-0}" pending="${5:-0}" other="${6:-0}"
  local tests=$(( passed + failed + skipped + pending + other ))
  cat > "${CTRF_REPORT:-$SRC/ctrf-report.json}" <<JSON
{
  "results": {
    "tool": { "name": "$tool" },
    "summary": {
      "tests": $tests,
      "passed": $passed,
      "failed": $failed,
      "pending": $pending,
      "skipped": $skipped,
      "other": $other
    }
  }
}
JSON
  printf 'CTRF {"results":{"tool":{"name":"%s"},"summary":{"tests":%d,"passed":%d,"failed":%d,"pending":%d,"skipped":%d,"other":%d}}}\n' \
    "$tool" "$tests" "$passed" "$failed" "$pending" "$skipped" "$other"
  [ "$failed" -eq 0 ]
}

EXPECTED_SELFTEST=34
EXPECTED_TLSH=7

for exe in mayhem/selftest mayhem/selftest_tlsh; do
  if [ ! -x "$exe" ]; then
    echo "missing $exe — mayhem/build.sh should have built it" >&2
    emit_ctrf "processhacker-vendored-libs" 0 1
    exit 1
  fi
done

passed=0; failed=0
for spec in "mayhem/selftest:$EXPECTED_SELFTEST" "mayhem/selftest_tlsh:$EXPECTED_TLSH"; do
  exe="${spec%%:*}"; want="${spec##*:}"
  out="$(timeout 120 "$exe" 2>/dev/null)"
  echo "$out"
  p=$(grep -c '^PASS ' <<<"$out"); f=$(grep -c '^FAIL ' <<<"$out")
  passed=$(( passed + p )); failed=$(( failed + f ))
  if [ $(( p + f )) -ne "$want" ] || ! grep -q "^RESULT passed=$p failed=$f\$" <<<"$out"; then
    echo "FAIL $exe reported $((p + f)) checks (expected $want) or no matching RESULT line" >&2
    failed=$(( failed + 1 ))
  fi
done

emit_ctrf "processhacker-vendored-libs" "$passed" "$failed"
