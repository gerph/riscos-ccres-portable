#!/bin/bash
##
# Run the ccres conversion tests, writing a JUnitXML report.
#
# Four groups of tests are run:
#
#   reference - each binary file in TestQueue/Res01 and TestQueue/Tem01
#               which has a reference text in TestQueue/<dir>.ref must
#               decode to exactly that text.
#   roundtrip - each binary file in those directories is decoded, encoded
#               and decoded again; both decoded texts must be identical.
#   skeleton  - the --skeleton and --describe outputs for resources and
#               templates encode without messages and decode back to the
#               plain skeleton.
#   case      - each case in tests/cases (see tests/cases/README.md) is
#               encoded, decoded and encoded again, checking the exit
#               status, the messages and that both binaries are identical.
#
# Every conversion fails its test if it reports an AddressSanitizer error,
# so the same tests can be run against a sanitizer build.
#
# Usage: run-tests.sh [--ccres <path>]
#
# Environment variables JUNITXML_DIR, JUNITXML_SUITE and JUNITXML_NAME may be
# set to change where the report is written and what it is called.

set -e
set -o pipefail

scriptdir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
rootdir="$(cd "$scriptdir/.." && pwd -P)"

source "${scriptdir}/progress.sh"
source "${scriptdir}/junitxml.sh"
junitxml_tempdir ""

ccres="${rootdir}/ccres"

while [[ "$#" -gt 0 ]] ; do
    case "$1" in
        --ccres)
            ccres="$2"
            shift 2
            ;;
        --help|-h)
            echo "Usage: $0 [--ccres <path>]"
            exit 0
            ;;
        *)
            echo "Unrecognised option: $1" >&2
            exit 1
            ;;
    esac
done

# Make the tool path absolute, as the tests run in a work directory.
if [[ "$ccres" != /* ]] ; then
    ccres="$(pwd -P)/$ccres"
fi
if [[ ! -x "$ccres" ]] ; then
    echo "The ccres tool '$ccres' does not exist or is not executable; build it first" >&2
    exit 1
fi

testqueue="${rootdir}/TestQueue"
casedir="${scriptdir}/cases"
workdir="${junitxml_tempdir}/work"
mkdir -p "$workdir"

# Text that marks a sanitizer failure in a conversion's stderr.
asan_marker='ERROR: AddressSanitizer'

# Seconds a conversion may take before it is treated as hung; even the
# largest cases take well under a second.
convert_timeout=${CCRES_TIMEOUT:-30}

# User variables that they can use to override our defaults
JUNITXML_DIR=${JUNITXML_DIR:-${rootdir}/test-results}
JUNITXML_SUITE=${JUNITXML_SUITE:-ccres}
JUNITXML_NAME=${JUNITXML_NAME:-Test-ccres.xml}

# JUnit XML file to output.
XML="${JUNITXML_DIR}/${JUNITXML_NAME}"
mkdir -p "${JUNITXML_DIR}"

junitxml_testsuite "${JUNITXML_SUITE}"
junitxml_ci_properties
junitxml_property "ccres" "$("$ccres" 2>&1 | head -1 || true)"

# Are we done
all_done=false
in_test=false

# Counts
pass=0
fail=0

# File the test functions write their failure reason to, as they run in a
# pipeline subshell.
reason_file="${junitxml_tempdir}/reason"


function cleanup() {
    local rc=$?
    if $in_test ; then
        result "fail" "Test runner stopped unexpectedly"
    fi
    if ! $all_done ; then
        finish
    fi

    return $rc
}

trap cleanup EXIT


##
# Finish all our processing and exit if error.
function finish() {
    all_done=true
    junitxml_report "$XML"
    junitxml_cleanup

    if [[ "$junitxml_nfail" != 0 ]] ; then
        error "$junitxml_nfail tests failed ($junitxml_npass passed)"
    else
        step "$junitxml_nfail tests failed ($junitxml_npass passed)"
    fi
}


##
# Report the start of the test's run
#
# @param $1     Class (group) of the test
# @param $2     Name of the test
function start() {
    local class="$1"
    local name="$2"
    step "Test: ${class}: ${name}"
    junitxml_start "$name"
    junitxml_class "$class"
    in_test=true
    rm -f "$reason_file"
}


##
# Report the result of the test's run
function result() {
    local result="$1"
    local reason="$2"
    in_test=false
    if [[ "$result" = 'pass' ]] ; then
        success "Passed"
        junitxml_result "pass"
        pass=$((pass + 1))
    elif [[ "$result" = 'skip' ]] ; then
        notice "Skipped"
        junitxml_result "skip" "" "$reason"
    else
        failure "FAIL: $reason"
        junitxml_result "fail" "" "$reason"
        fail=$((fail + 1))
    fi
}


##
# Record why the current test failed, and return failure.
#
# @param $1     Reason for the failure
function fail_because() {
    local reason="$1"
    echo "$reason" > "$reason_file"
    echo "Failed: $reason"
    return 1
}


##
# Run a test function, attaching its output to the test, and report it.
#
# @param $@     Test function and its parameters
function run_test() {
    if "$@" 2>&1 | junitxml_output | sed "s/^/  /" ; then
        result "pass"
    else
        result "fail" "$(cat "$reason_file" 2>/dev/null || echo "Test failed")"
    fi
}


##
# Run one conversion, keeping its stderr in a file and showing it.
#
# @param $1     Input file
# @param $2     Output file
# @param $3     File to keep stderr in
# @return       Exit status of ccres
function convert() {
    local input="$1"
    local output="$2"
    local errors="$3"
    local rc=0

    echo "\$ ccres $(basename "$input") $(basename "$output")"
    timeout "$convert_timeout" "$ccres" "$input" "$output" 2> "$errors" || rc=$?
    if [[ -s "$errors" ]] ; then
        # Only the start, in case a fault repeats a message endlessly
        head -50 "$errors" | sed "s/^/    stderr: /" | cut -c1-300
    fi
    if [[ "$rc" = 124 ]] ; then
        echo "    timed out after ${convert_timeout} seconds" | tee -a "$errors"
    fi
    echo "    exit status: $rc"
    return $rc
}


##
# Check that a conversion's stderr has no sanitizer report.
#
# @param $1     File holding stderr
function check_no_asan() {
    local errors="$1"
    if grep -q "$asan_marker" "$errors" ; then
        fail_because "AddressSanitizer reported an error"
        return 1
    fi
    return 0
}


##
# Test that a binary file decodes to its reference text.
#
# @param $1     Binary file
# @param $2     Reference text
function test_reference() {
    local input="$1"
    local reference="$2"
    local out="${workdir}/reference.txt"
    local errors="${workdir}/reference.err"

    convert "$input" "$out" "$errors" || { check_no_asan "$errors" || return 1 ; fail_because "Decoding failed" ; return 1 ; }
    check_no_asan "$errors" || return 1
    if ! diff -u "$reference" "$out" | head -40 ; then
        fail_because "Decoded text differs from the reference"
        return 1
    fi
    return 0
}


##
# Test that a binary file decodes, encodes and decodes to the same text.
#
# @param $1     Binary file
function test_roundtrip() {
    local input="$1"
    local text1="${workdir}/roundtrip1.txt"
    local binary="${workdir}/roundtrip.bin"
    local text2="${workdir}/roundtrip2.txt"
    local errors="${workdir}/roundtrip.err"

    convert "$input" "$text1" "$errors" || { check_no_asan "$errors" || return 1 ; fail_because "Decoding failed" ; return 1 ; }
    check_no_asan "$errors" || return 1
    convert "$text1" "$binary" "$errors" || { check_no_asan "$errors" || return 1 ; fail_because "Encoding the decoded text failed" ; return 1 ; }
    check_no_asan "$errors" || return 1
    convert "$binary" "$text2" "$errors" || { check_no_asan "$errors" || return 1 ; fail_because "Decoding the encoded file failed" ; return 1 ; }
    check_no_asan "$errors" || return 1
    if ! diff -u "$text1" "$text2" | head -40 ; then
        fail_because "Text differs after a round trip"
        return 1
    fi
    return 0
}


##
# Test a text case: encode, decode and encode again.
#
# @param $1     Case name
# @param $2     Case input (.txt or .gen)
function test_case() {
    local name="$1"
    local source="$2"
    local expect="${casedir}/${name}.expect"
    local input="${workdir}/case.txt"
    local binary1="${workdir}/case1.bin"
    local text="${workdir}/case.decoded.txt"
    local binary2="${workdir}/case2.bin"
    local errors="${workdir}/case.err"
    local errors_encode="${workdir}/case.encode.err"
    local expect_rc=0
    local contains=()
    local bytes=()
    local keyword value rc offset want got

    if [[ -f "$expect" ]] ; then
        while IFS= read -r line || [[ -n "$line" ]] ; do
            [[ "$line" = '' || "$line" = '#'* ]] && continue
            keyword="${line%% *}"
            value="${line#* }"
            case "$keyword" in
                encode-rc)
                    expect_rc="$value"
                    ;;
                stderr-contains)
                    contains+=("$value")
                    ;;
                binary-bytes)
                    bytes+=("$value")
                    ;;
                *)
                    fail_because "Unknown keyword '$keyword' in $(basename "$expect")"
                    return 1
                    ;;
            esac
        done < "$expect"
    fi

    if [[ "$source" = *.gen ]] ; then
        if ! bash "$source" > "$input" ; then
            fail_because "Generating the input failed"
            return 1
        fi
    else
        cp "$source" "$input"
    fi

    rc=0
    convert "$input" "$binary1" "$errors_encode" || rc=$?
    check_no_asan "$errors_encode" || return 1
    if [[ "$rc" != "$expect_rc" ]] ; then
        fail_because "Encoding exit status was $rc, expected $expect_rc"
        return 1
    fi
    if [[ "${#contains[@]}" -eq 0 ]] ; then
        if [[ -s "$errors_encode" ]] ; then
            fail_because "Encoding wrote unexpected messages to stderr"
            return 1
        fi
    else
        for value in "${contains[@]}" ; do
            if ! grep -qF -- "$value" "$errors_encode" ; then
                fail_because "Encoding did not report '$value'"
                return 1
            fi
        done
    fi
    if [[ "$expect_rc" != 0 ]] ; then
        return 0
    fi
    for value in "${bytes[@]}" ; do
        offset="${value%% *}"
        want="${value#* }"
        want="${want// /}"
        got="$(od -An -v -tx1 -j "$offset" -N "$(( ${#want} / 2 ))" "$binary1" | tr -d ' \n')"
        echo "Bytes at ${offset}: ${got}"
        if [[ "$got" != "$want" ]] ; then
            fail_because "Bytes at offset ${offset} were '${got}', expected '${want}'"
            return 1
        fi
    done

    convert "$binary1" "$text" "$errors" || { check_no_asan "$errors" || return 1 ; fail_because "Decoding failed" ; return 1 ; }
    check_no_asan "$errors" || return 1
    convert "$text" "$binary2" "$errors" || { check_no_asan "$errors" || return 1 ; fail_because "Encoding the decoded text failed" ; return 1 ; }
    check_no_asan "$errors" || return 1
    if ! cmp "$binary1" "$binary2" ; then
        fail_because "Binary differs after a round trip"
        return 1
    fi
    return 0
}


##
# Test that a skeleton (plain or annotated) converts to binary and back to
# the plain skeleton, with no messages.
#
# @param $1     Option: --skeleton or --describe
# @param $2     Kind: resource or template
function test_skeleton() {
    local option="$1"
    local kind="$2"
    local plain="${workdir}/skeleton-plain.txt"
    local text="${workdir}/skeleton.txt"
    local binary="${workdir}/skeleton.bin"
    local decoded="${workdir}/skeleton.decoded.txt"
    local errors="${workdir}/skeleton.err"
    local rc=0

    echo "\$ ccres --skeleton $kind"
    "$ccres" --skeleton "$kind" "$plain" 2> "$errors" || rc=$?
    check_no_asan "$errors" || return 1
    if [[ "$rc" != 0 || -s "$errors" ]] ; then
        cat "$errors"
        fail_because "Writing the plain skeleton failed"
        return 1
    fi
    echo "\$ ccres $option $kind"
    "$ccres" "$option" "$kind" "$text" 2> "$errors" || rc=$?
    check_no_asan "$errors" || return 1
    if [[ "$rc" != 0 || -s "$errors" ]] ; then
        cat "$errors"
        fail_because "Writing the skeleton failed"
        return 1
    fi
    echo "    $(grep -c '{' "$text") blocks, $(grep -c '^ *#@ field' "$text" || true) field descriptions"

    convert "$text" "$binary" "$errors" || { check_no_asan "$errors" || return 1 ; fail_because "Encoding the skeleton failed" ; return 1 ; }
    check_no_asan "$errors" || return 1
    if [[ -s "$errors" ]] ; then
        fail_because "Encoding the skeleton wrote messages to stderr"
        return 1
    fi
    convert "$binary" "$decoded" "$errors" || { check_no_asan "$errors" || return 1 ; fail_because "Decoding the skeleton failed" ; return 1 ; }
    check_no_asan "$errors" || return 1
    if ! diff -u "$plain" "$decoded" | head -40 ; then
        fail_because "Skeleton differs after a round trip"
        return 1
    fi
    return 0
}


for option in --skeleton --describe ; do
    for kind in resource template ; do
        echo
        start "skeleton" "${option#--} ${kind}"
        run_test test_skeleton "$option" "$kind"
    done
done

for dir in Res01 Tem01 ; do
    for input in "${testqueue}/${dir}"/* ; do
        leaf="$(basename "$input")"
        reference="${testqueue}/${dir}.ref/${leaf%%,*}"
        echo
        start "reference" "${dir}/${leaf}"
        if [[ -f "$reference" ]] ; then
            run_test test_reference "$input" "$reference"
        else
            result "skip" "No reference text"
        fi
        echo
        start "roundtrip" "${dir}/${leaf}"
        run_test test_roundtrip "$input"
    done
done

for source in "${casedir}"/*.txt "${casedir}"/*.gen ; do
    [[ -f "$source" ]] || continue
    leaf="$(basename "$source")"
    name="${leaf%.*}"
    echo
    start "case" "$name"
    run_test test_case "$name" "$source"
done


# Report results
echo
notice "Tests complete"
echo "Pass : $pass"
echo "Fail : $fail"

finish

# finish will already have exited if there was a failure.
exit 0
