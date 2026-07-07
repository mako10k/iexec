#!/bin/sh

set -u

if [ "${IEXEC_TEST_DOCKER:-0}" != 1 ]; then
  echo "SKIP: set IEXEC_TEST_DOCKER=1 to run Docker entrypoint tests"
  exit 77
fi

if ! command -v docker >/dev/null 2>&1; then
  echo "SKIP: docker command not found"
  exit 77
fi

if ! docker info >/dev/null 2>&1; then
  echo "SKIP: docker daemon unavailable"
  exit 77
fi

IEXEC=${IEXEC_TEST_BINARY:-./src/iexec}

fail() {
  echo "FAIL: $*" >&2
  exit 1
}

tmpdir=$(mktemp -d "${TMPDIR:-/tmp}/iexec-docker-test.XXXXXX") || exit 99
image=iexec-entrypoint-test:$$
container=

cleanup() {
  if [ -n "$container" ]; then
    docker rm -f "$container" >/dev/null 2>&1 || true
  fi
  docker rmi "$image" >/dev/null 2>&1 || true
  rm -rf "$tmpdir"
}

trap cleanup EXIT HUP INT TERM

cp "$IEXEC" "$tmpdir/iexec" || fail "failed to copy iexec binary"
cat >"$tmpdir/Dockerfile" <<'EOF'
FROM busybox:1.36
COPY iexec /usr/local/bin/iexec
ENTRYPOINT ["/usr/local/bin/iexec"]
CMD ["/bin/true"]
EOF

docker build -q -t "$image" "$tmpdir" >/dev/null ||
  fail "docker build failed"

docker run --rm "$image" /bin/true ||
  fail "expected /bin/true to exit 0"

docker run --rm "$image" /bin/sh -c 'exit 7'
status=$?
if [ "$status" -ne 7 ]; then
  fail "expected Docker entrypoint status 7, got $status"
fi

output=$(docker run --rm "$image" /bin/sh -c \
  '(sleep 1; echo orphan-done) & exit 0')
case "$output" in
  *orphan-done*) ;;
  *) fail "Docker entrypoint exited before orphan output was reaped" ;;
esac

container=iexec-entrypoint-test-$$
docker run -d --name "$container" "$image" /bin/sh -c \
  'trap "exit 42" TERM; while true; do sleep 1 & wait $!; done' >/dev/null ||
  fail "docker run -d failed"
docker stop -t 5 "$container" >/dev/null ||
  fail "docker stop failed"
status=$(docker inspect -f '{{.State.ExitCode}}' "$container")
docker rm "$container" >/dev/null || true
container=
if [ "$status" -ne 42 ]; then
  fail "expected Docker stop to produce child status 42, got $status"
fi

