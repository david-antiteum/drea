#!/usr/bin/env bash
#
# End-to-end checks on the example apps: the exit code of a misused command line,
# and the machine-readable output against the published JSON Schemas.
#
# The unit tests exercise the library through its API; this exercises the
# binaries as a user runs them. Both classes of bug it covers have happened:
# an exit code of 0 for a refused invocation, and a warning on stdout that left
# --validate --json unparseable.
#
# Usage: cli-checks.sh <bin-dir> <repo-root>
# Skips (exit 77, ctest SKIP_RETURN_CODE) when python3 is unavailable: the JSON
# checks need it. Schema validation additionally needs the jsonschema module and
# degrades to a parse-only check without it.

set -u

BIN_DIR="${1:?bin directory required}"
ROOT="${2:?repo root required}"

failures=0
checks=0

fail()
{
	echo "FAIL: $*"
	failures=$(( failures + 1 ))
}

# expect_rc <expected> <argv...>
expect_rc()
{
	local expected="$1"
	shift
	checks=$(( checks + 1 ))
	"$@" >/dev/null 2>&1
	local actual=$?
	if [ "${actual}" != "${expected}" ]; then
		fail "expected exit ${expected}, got ${actual}: $(basename "$1") ${*:2}"
	fi
}

if ! command -v python3 >/dev/null 2>&1; then
	echo "python3 not found: skipping the CLI checks"
	exit 77
fi

if python3 -c "import jsonschema" 2>/dev/null; then
	SCHEMA_CHECK=1
else
	echo "note: the jsonschema module is missing, checking that the output parses only"
	SCHEMA_CHECK=0
fi

# expect_json <schema-file> <argv...>: the output must parse, and validate when
# the jsonschema module is available
expect_json()
{
	local schema="$1"
	shift
	checks=$(( checks + 1 ))
	local out
	if ! out=$( "$@" 2>/dev/null ); then
		: # a non-zero exit is fine here, --validate uses it to report findings
	fi
	if ! printf '%s' "${out}" | python3 -c "
import json, sys
json.load(sys.stdin)
" 2>/dev/null; then
		fail "output is not JSON: $(basename "$1") ${*:2}"
		return
	fi
	if [ "${SCHEMA_CHECK}" = "1" ]; then
		if ! printf '%s' "${out}" | python3 -c "
import json, sys, jsonschema
jsonschema.validate( json.load( sys.stdin ), json.load( open( '${schema}' ) ) )
" 2>/dev/null; then
			fail "output does not match ${schema##*/}: $(basename "$1") ${*:2}"
		fi
	fi
}

DESCRIBE_SCHEMA="${ROOT}/docs/describe.schema.json"
VALIDATE_SCHEMA="${ROOT}/docs/validate.schema.json"
CALC_INVALID="${ROOT}/examples/calculator/config-invalid.yml"

EXAMPLES="hello saythis say calculator multi hidden groups"

echo "== machine readable output of every example =="
for app in ${EXAMPLES}; do
	[ -x "${BIN_DIR}/${app}" ] || continue
	expect_json "${DESCRIBE_SCHEMA}" "${BIN_DIR}/${app}" describe
	expect_json "${VALIDATE_SCHEMA}" "${BIN_DIR}/${app}" --validate --json
done

# A malformed command line must still leave the report parseable: a warning on
# stdout ahead of the JSON used to break exactly this.
expect_json "${VALIDATE_SCHEMA}" "${BIN_DIR}/calculator" -z --validate --json
expect_json "${VALIDATE_SCHEMA}" "${BIN_DIR}/calculator" --equal=nan --validate --json
expect_json "${VALIDATE_SCHEMA}" "${BIN_DIR}/calculator" --no-help --validate --json
expect_json "${VALIDATE_SCHEMA}" "${BIN_DIR}/hello" --config-file=/nonexistent.yml --validate --json

echo "== exit codes: a valid invocation =="
expect_rc 0  "${BIN_DIR}/hello" Ada
expect_rc 0  "${BIN_DIR}/hello" --help
expect_rc 0  "${BIN_DIR}/hello" describe
expect_rc 0  "${BIN_DIR}/hello" man
expect_rc 0  "${BIN_DIR}/hello" completion bash
expect_rc 0  "${BIN_DIR}/hello" -- man
expect_rc 0  "${BIN_DIR}/calculator" sum 1 2
expect_rc 0  "${BIN_DIR}/say" this hola
expect_rc 0  "${BIN_DIR}/hidden" --dev debug
expect_rc 0  "${BIN_DIR}/groups" --tier operator rotate-keys

echo "== exit codes: a misused command line is a usage error (64) =="
expect_rc 64 "${BIN_DIR}/calculator" pow            # unknown command
expect_rc 64 "${BIN_DIR}/calculator" power 2        # too few arguments
expect_rc 64 "${BIN_DIR}/calculator" power 2 3 4    # too many
expect_rc 64 "${BIN_DIR}/calculator"                # no command at all
expect_rc 64 "${BIN_DIR}/hello" completion powershell   # outside param-choices
expect_rc 64 "${BIN_DIR}/hidden" debug              # hidden command
expect_rc 64 "${BIN_DIR}/groups" rotate-keys        # gated by a disabled group
expect_rc 64 "${BIN_DIR}/say" repeat                # parent needing a subcommand
expect_rc 64 "${BIN_DIR}/say" -- this hola          # app takes no root params

echo "== exit codes: --validate reports the configuration (78 structural, 65 values) =="
expect_rc 0  "${BIN_DIR}/hello" --validate
expect_rc 78 "${BIN_DIR}/calculator" --config-file "${CALC_INVALID}" --validate
expect_rc 78 "${BIN_DIR}/calculator" --config-file="${CALC_INVALID}" --validate   # inline form
expect_rc 78 "${BIN_DIR}/calculator" -z --validate     # unknown short option
expect_rc 78 "${BIN_DIR}/calculator" --zz --validate   # unknown long option
expect_rc 78 "${BIN_DIR}/calculator" --no-help --validate       # not_negatable
expect_rc 78 "${BIN_DIR}/calculator" --verbose=false --validate # unexpected_value
expect_rc 78 "${BIN_DIR}/calculator" pow --validate    # unknown command
expect_rc 65 "${BIN_DIR}/calculator" --equal=nan --validate     # a value is wrong
expect_rc 66 "${BIN_DIR}/hello" --config-file /nonexistent.yml --validate

echo "== a reported flag does not change what the app does =="
expect_rc 0  "${BIN_DIR}/hello" --help=false Ada    # reported, ignored, app runs
expect_rc 0  "${BIN_DIR}/hello" --unknown-flag Ada  # unknown argument is not fatal

echo
if [ "${failures}" != "0" ]; then
	echo "${failures} of ${checks} CLI checks failed"
	exit 1
fi
echo "all ${checks} CLI checks passed"
