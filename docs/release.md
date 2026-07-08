# Release Notes

This project is licensed under the MIT License. See [../LICENSE](../LICENSE).

## Release Checklist

- Update `AC_INIT` in [../configure.ac](../configure.ac) when changing the
  release version.
- Run `autoreconf -fi`.
- Run `./configure`.
- Run `make check`.
- Run `make distcheck`.
- Run `sudo -n tests/install-policy.sh` on a host with `setcap` and `getcap`.
- Optionally run `IEXEC_TEST_DOCKER=1 make check` on a Docker-capable host.
- Confirm the default install does not set Linux file capabilities or the setuid
  bit.
- Prefer `./configure --enable-cap-install` for deliberate PID namespace
  validation workflows.
- Confirm `./configure --enable-setuid-install` is only used as an explicit
  fallback.

## Distribution Archive

After the checklist passes, create the source archive with:

```sh
make dist
```

The archive is redistributable under the MIT License.
