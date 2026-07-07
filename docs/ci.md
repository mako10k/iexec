# CI

The default CI path should stay non-privileged.

The GitHub Actions workflow in [../.github/workflows/check.yml](../.github/workflows/check.yml)
runs:

```sh
autoreconf -fi
./configure
make V=1
make check V=1
make distcheck V=1
```

The Docker entrypoint integration test is opt-in and is skipped by default
unless `IEXEC_TEST_DOCKER=1` is set. Privileged PID namespace validation should
also remain outside the default CI path unless a dedicated runner is configured.

To run the privileged PID namespace test manually:

```sh
sudo -n env IEXEC_TEST_PIDNS=1 IEXEC_TEST_BINARY="$PWD/src/iexec" \
  tests/pidns-validation.sh
```
