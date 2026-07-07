#!/bin/sh

set -u

IEXEC=${IEXEC_TEST_BINARY:-./src/iexec}

fail() {
  echo "FAIL: $*" >&2
  exit 1
}

tmpdir=$(mktemp -d "${TMPDIR:-/tmp}/iexec-test.XXXXXX") || exit 99
watchdog_pid=

cleanup() {
  if [ -n "$watchdog_pid" ]; then
    kill "$watchdog_pid" 2>/dev/null || true
  fi
  rm -rf "$tmpdir"
}

trap cleanup EXIT HUP INT TERM

run_expect_status() {
  expected=$1
  shift

  "$IEXEC" "$@"
  status=$?
  if [ "$status" -ne "$expected" ]; then
    fail "expected status $expected, got $status: $*"
  fi
}

run_expect_status 0 /bin/true
run_expect_status 1 /bin/false
run_expect_status 7 /bin/sh -c 'exit 7'
run_expect_status 143 /bin/sh -c 'kill -TERM $$'

help_output=$("$IEXEC" --help)
case "$help_output" in
  *"--pidns[=MODE]"*"for validation"*) ;;
  *) fail "--pidns help text must frame PID namespace mode as validation" ;;
esac

"$IEXEC" >/dev/null 2>"$tmpdir/no-command.err"
status=$?
if [ "$status" -ne 1 ]; then
  fail "expected no-command status 1, got $status"
fi
if ! grep -q "No command specified" "$tmpdir/no-command.err"; then
  fail "missing no-command diagnostic"
fi

signal_file=$tmpdir/signal-forwarded
"$IEXEC" /bin/sh -c \
  'trap '\''echo term > "$1"; exit 42'\'' TERM; while :; do sleep 1 & wait $!; done' \
  sh "$signal_file" &
pid=$!
(sleep 5; kill -KILL "$pid" 2>/dev/null || true) &
watchdog_pid=$!
sleep 1
kill -TERM "$pid"
wait "$pid"
status=$?
kill "$watchdog_pid" 2>/dev/null || true
watchdog_pid=
if [ "$status" -ne 42 ]; then
  fail "expected forwarded SIGTERM to produce status 42, got $status"
fi
if ! grep -q "term" "$signal_file"; then
  fail "child did not observe forwarded SIGTERM"
fi

orphan_file=$tmpdir/orphan-reaped
"$IEXEC" /bin/sh -c '(sleep 1; echo done > "$1") & exit 0' sh "$orphan_file"
status=$?
if [ "$status" -ne 0 ]; then
  fail "expected orphan test status 0, got $status"
fi
if ! grep -q "done" "$orphan_file"; then
  fail "iexec exited before reaping orphaned child"
fi
