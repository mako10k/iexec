# Release Notes

This project is licensed under the MIT License. See [../LICENSE](../LICENSE).

## Release Checklist

- Update `AC_INIT` in [../configure.ac](../configure.ac) when changing the
  release version.
- Run `autoreconf -fi`.
- Run `./configure`.
- Run `make check`.
- Run `make distcheck`.
- Optionally run `IEXEC_TEST_DOCKER=1 make check` on a Docker-capable host.
- Confirm the default install does not set the setuid bit.
- Confirm `./configure --enable-setuid-install` is only used for deliberate
  PID namespace validation workflows.

## Distribution Archive

After the checklist passes, create the source archive with:

```sh
make dist
```

The archive is redistributable under the MIT License.
