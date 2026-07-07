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
run_expect_status 0 FOO=bar /bin/sh -c 'test "$FOO" = bar'
run_expect_status 0 --deathsig=NONE /bin/true
run_expect_status 0 --deathsig=TERM /bin/true
run_expect_status 0 --deathsig=15 /bin/true
run_expect_status 0 --pidns=inherit /bin/true

version_output=$("$IEXEC" --version)
status=$?
if [ "$status" -ne 0 ]; then
  fail "expected --version status 0, got $status"
fi
case "$version_output" in
  iexec\ *) ;;
  *) fail "unexpected --version output: $version_output" ;;
esac

help_output=$("$IEXEC" --help)
status=$?
if [ "$status" -ne 0 ]; then
  fail "expected --help status 0, got $status"
fi
case "$help_output" in
  *"Run COMMAND as an init/reaper wrapper"*) ;;
  *) fail "--help output must describe init/reaper wrapper purpose" ;;
esac
case "$help_output" in
  *"--pidns[=MODE]"*"for validation"*) ;;
  *) fail "--pidns help text must frame PID namespace mode as validation" ;;
esac

"$IEXEC" --deathsig=NOPE /bin/true 2>"$tmpdir/invalid-deathsig.err"
status=$?
if [ "$status" -ne 1 ]; then
  fail "expected invalid deathsig status 1, got $status"
fi
if ! grep -q "Invalid signal: NOPE" "$tmpdir/invalid-deathsig.err"; then
  fail "missing invalid deathsig diagnostic"
fi

"$IEXEC" --pidns=pid:not-a-pid /bin/true 2>"$tmpdir/invalid-pidns.err"
status=$?
if [ "$status" -ne 1 ]; then
  fail "expected invalid pidns status 1, got $status"
fi
if ! grep -q "Invalid pidns: pid:not-a-pid" "$tmpdir/invalid-pidns.err"; then
  fail "missing invalid pidns diagnostic"
fi

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
