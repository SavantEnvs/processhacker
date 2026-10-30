#!/usr/bin/env bash
#
# mayhem/build.sh — build the fuzz harnesses (sanitized) AND the known-answer self-tests (normal flags).
#
# System Informer / Process Hacker is a Windows application; what builds and runs on Linux is the set of
# third-party libraries it vendors under tools/thirdparty (json-c, Mini-XML, PCRE2, miniz, Zydis, TLSH).
# Those copies carry Windows-isms (MSVC <io.h>, _strdup, ...); mayhem/shim/ maps them back to libc so the
# upstream sources compile UNCHANGED.
set -euo pipefail

[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH

: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
export SANITIZER_FLAGS DEBUG_FLAGS CC CXX LIB_FUZZING_ENGINE MAYHEM_JOBS COVERAGE_FLAGS

SRC="${SRC:-/mayhem}"
cd "$SRC"
TP="$SRC/tools/thirdparty"
SHIM="$SRC/mayhem/shim"
OBJ="$SRC/mayhem/build-obj"          # gitignored (mayhem/.gitignore)

# Per-library compile flags (the product's own CMake defines HAVE_CONFIG_H for all of them).
JSONC_FLAGS=(-include "$SHIM/jsonc_compat.h" -D_GNU_SOURCE -DHAVE_STRNCASECMP=1 -DHAVE_VASPRINTF=1 -I"$TP/jsonc")
# Mini-XML calls its read/write callbacks through a pointer whose type differs from the definition's
# (benign on Linux, trips UBSan "function" on every input), so only that check is relaxed, for this library.
MXML_FLAGS=(-fno-sanitize=function -I"$SHIM" -I"$TP/mxml")
PCRE_FLAGS=(-DHAVE_CONFIG_H -DPCRE2_STATIC=1 -I"$TP/pcre")
MINIZ_FLAGS=(-I"$TP/miniz")
ZYDIS_FLAGS=(-I"$TP/zydis")
TLSH_FLAGS=(-include string.h -I"$TP/tlsh")

JSONC_SRC=(arraylist debug json_c_version json_object json_object_iterator json_pointer json_tokener
           json_util json_visit libjson linkhash printbuf random_seed)
MXML_SRC=(mxml-attr mxml-file mxml-get mxml-index mxml-node mxml-options mxml-private mxml-search mxml-set)
PCRE_SRC=(pcre2_auto_possess pcre2_chartables pcre2_chkdint pcre2_compile pcre2_compile_class pcre2_config
          pcre2_context pcre2_convert pcre2_dfa_match pcre2_error pcre2_extuni pcre2_find_bracket
          pcre2_maketables pcre2_match pcre2_match_data pcre2_newline pcre2_ord2utf pcre2_pattern_info
          pcre2_script_run pcre2_serialize pcre2_string_utils pcre2_study pcre2_substitute pcre2_substring
          pcre2_tables pcre2_ucd pcre2_ucptables pcre2_valid_utf pcre2_xclass)

# build_lib <outdir> <cflags-string> : compile every vendored library into <outdir>/lib<name>.a
build_lib() {
  local out="$1" cflags="$2" cxxflags="$3"
  mkdir -p "$out"
  compile_set() {   # <libname> <srcdir> <flags-array-name> <source basenames...>
    local lname="$1" ldir="$2"; local -n lflags="$3"; shift 3
    local objs=() s
    for s in "$@"; do
      $CC $cflags "${lflags[@]}" -w -c "$ldir/$s.c" -o "$out/$lname-$s.o" &
      objs+=("$out/$lname-$s.o")
      while [ "$(jobs -rp | wc -l)" -ge "$MAYHEM_JOBS" ]; do wait -n; done
    done
    wait
    rm -f "$out/lib$lname.a"; ar rcs "$out/lib$lname.a" "${objs[@]}"
  }
  compile_set jsonc "$TP/jsonc" JSONC_FLAGS "${JSONC_SRC[@]}"
  $CC $cflags -w -c "$SRC/mayhem/phlib_stubs.c" -o "$out/jsonc-phlib_stubs.o"
  ar rcs "$out/libjsonc.a" "$out/jsonc-phlib_stubs.o"
  compile_set mxml  "$TP/mxml"  MXML_FLAGS  "${MXML_SRC[@]}"
  compile_set pcre  "$TP/pcre"  PCRE_FLAGS  "${PCRE_SRC[@]}"
  compile_set miniz "$TP/miniz" MINIZ_FLAGS miniz
  compile_set zydis "$TP/zydis" ZYDIS_FLAGS Zydis
  local t
  for t in tlsh tlsh_impl tlsh_util; do
    $CXX $cxxflags "${TLSH_FLAGS[@]}" -w -c "$TP/tlsh/$t.cpp" -o "$out/tlsh-$t.o"
  done
  rm -f "$out/libtlsh.a"; ar rcs "$out/libtlsh.a" "$out"/tlsh-*.o
}

# ---------------------------------------------------------------------------------------------
# 1) Sanitized libraries + fuzz harnesses (each linked twice: libFuzzer + run-once standalone).
# ---------------------------------------------------------------------------------------------
FUZZ_C_FLAGS="$SANITIZER_FLAGS $DEBUG_FLAGS -O1"
rm -rf "$OBJ/fuzz"
build_lib "$OBJ/fuzz" "$FUZZ_C_FLAGS" "$FUZZ_C_FLAGS"

$CC $FUZZ_C_FLAGS -c "$STANDALONE_FUZZ_MAIN" -o "$OBJ/fuzz/standalone_main.o"
$CXX $FUZZ_C_FLAGS -c "$SRC/mayhem/lsan_off.cc" -o "$OBJ/fuzz/lsan_off.o"

# harness  source  extra-includes...  -- libs...
build_harness() {
  local name="$1" srcfile="$2" lang="$3"; shift 3
  local compiler="$CC"; [ "$lang" = cxx ] && compiler="$CXX"
  local inc=() libs=()
  while [ $# -gt 0 ] && [ "$1" != "--" ]; do inc+=("$1"); shift; done
  shift
  libs=("$@")
  $compiler $FUZZ_C_FLAGS "${inc[@]}" -c "$SRC/mayhem/$srcfile" -o "$OBJ/fuzz/$name.o"
  $CXX $FUZZ_C_FLAGS $LIB_FUZZING_ENGINE "$OBJ/fuzz/$name.o" "${libs[@]}" "$OBJ/fuzz/lsan_off.o" -o "$SRC/$name"
  $CXX $FUZZ_C_FLAGS "$OBJ/fuzz/$name.o" "$OBJ/fuzz/standalone_main.o" "${libs[@]}" "$OBJ/fuzz/lsan_off.o" -o "$SRC/$name-standalone"
}

L="$OBJ/fuzz"
build_harness fuzz_jsonc fuzz_jsonc.c c -I"$TP/jsonc" -include "$SHIM/jsonc_compat.h" -D_GNU_SOURCE -DHAVE_STRNCASECMP=1 -- "$L/libjsonc.a"
build_harness fuzz_mxml  fuzz_mxml.c  c -I"$SHIM" -I"$TP/mxml" -- "$L/libmxml.a"
build_harness fuzz_pcre2 fuzz_pcre2.c c -I"$TP/pcre" -DPCRE2_STATIC=1 -- "$L/libpcre.a"
build_harness fuzz_miniz fuzz_miniz.c c -I"$TP/miniz" -- "$L/libminiz.a"
build_harness fuzz_zydis fuzz_zydis.c c -I"$TP/zydis" -- "$L/libzydis.a"
build_harness fuzz_tlsh  fuzz_tlsh.cpp cxx -I"$TP/tlsh" -include string.h -- "$L/libtlsh.a"

# ---------------------------------------------------------------------------------------------
# 2) Known-answer self-tests with the project's NORMAL flags (no sanitizers), run by test.sh.
# ---------------------------------------------------------------------------------------------
TEST_FLAGS="-O1 $COVERAGE_FLAGS"
rm -rf "$OBJ/test"
build_lib "$OBJ/test" "$TEST_FLAGS" "$TEST_FLAGS"
T="$OBJ/test"
$CC $TEST_FLAGS "${JSONC_FLAGS[@]}" "${MXML_FLAGS[@]}" "${PCRE_FLAGS[@]}" "${MINIZ_FLAGS[@]}" "${ZYDIS_FLAGS[@]}" \
    -c "$SRC/mayhem/selftest.c" -o "$T/selftest.o"
$CC $TEST_FLAGS "$T/selftest.o" "$T/libjsonc.a" "$T/libmxml.a" "$T/libpcre.a" "$T/libminiz.a" "$T/libzydis.a" \
    -lm -o "$SRC/mayhem/selftest"
$CXX $TEST_FLAGS "${TLSH_FLAGS[@]}" -c "$SRC/mayhem/selftest_tlsh.cpp" -o "$T/selftest_tlsh.o"
$CXX $TEST_FLAGS "$T/selftest_tlsh.o" "$T/libtlsh.a" -o "$SRC/mayhem/selftest_tlsh"
