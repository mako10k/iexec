#!/bin/sh

set -u

if [ "${IEXEC_TEST_PIDNS:-0}" != 1 ]; then
  echo "SKIP: set IEXEC_TEST_PIDNS=1 to run privileged PID namespace tests"
  exit 77
fi

if [ "$(id -u)" -ne 0 ]; then
  echo "SKIP: privileged PID namespace tests require root"
  exit 77
fi

IEXEC=${IEXEC_TEST_BINARY:-./src/iexec}

fail() {
  echo "FAIL: $*" >&2
  exit 1
}

tmpdir=$(mktemp -d "${TMPDIR:-/tmp}/iexec-pidns-test.XXXXXX") || exit 99
holder_pid=

cleanup() {
  if [ -n "$holder_pid" ]; then
    kill "$holder_pid" 2>/dev/null || true
    wait "$holder_pid" 2>/dev/null || true
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

assert_inside_iexec_pidns='
  test "$(cat /proc/1/comm)" = "iexec" &&
  test "$$" -ne 1
'

run_expect_status 7 --pidns=new /bin/sh -c 'exit 7'
run_expect_status 0 --pidns=new /bin/sh -c "$assert_inside_iexec_pidns"

orphan_file=$tmpdir/pidns-orphan-reaped
"$IEXEC" --pidns=new /bin/sh -c \
  '(sleep 1; echo done > "$1") & exit 0' sh "$orphan_file"
status=$?
if [ "$status" -ne 0 ]; then
  fail "expected pidns orphan test status 0, got $status"
fi
if ! grep -q "done" "$orphan_file"; then
  fail "iexec exited before reaping orphaned child in new PID namespace"
fi

holder_err=$tmpdir/holder.err
"$IEXEC" --pidns=new /bin/sh -c \
  'trap "exit 0" TERM; while :; do sleep 1 & wait $!; done' \
  2>"$holder_err" &
holder_pid=$!

init_pid=
for _ in 1 2 3 4 5 6 7 8 9 10; do
  init_pid=$(sed -n 's/.*--pidns=pid:\([0-9][0-9]*\).*/\1/p' "$holder_err" | tail -1)
  if [ -n "$init_pid" ]; then
    break
  fi
  sleep 1
done

if [ -z "$init_pid" ]; then
  cat "$holder_err" >&2
  fail "could not discover holder PID namespace init process"
fi

run_expect_status 0 --pidns=pid:"$init_pid" /bin/sh -c "$assert_inside_iexec_pidns"
run_expect_status 0 --pidns=file:/proc/"$init_pid"/ns/pid /bin/sh -c "$assert_inside_iexec_pidns"

exec 9<"/proc/$init_pid/ns/pid"
run_expect_status 0 --pidns=fd:9 /bin/sh -c "$assert_inside_iexec_pidns"
exec 9<&-

