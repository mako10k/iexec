# Privilege and Install Policy

`iexec` has two different operating contexts. Keep them separate when building,
installing, and reviewing the program.

## Docker Init/Reaper Path

The ordinary Docker entrypoint path does not require setuid installation.

In this path, Docker already creates the container's PID namespace and starts
`iexec` as PID 1 inside it. `iexec` only needs to run the command, forward
shutdown signals, and reap orphaned descendants.

Use the default build and install behavior for this path:

```sh
./configure
make
make install
```

By default, `make install` installs `iexec` without setting the setuid bit.

## Non-Docker PID Namespace Validation

`--pidns` is for validating init/reaper behavior outside Docker. Creating or
entering PID namespaces and remounting `/proc` may require elevated privileges.

If a local validation workflow intentionally needs a setuid-installed `iexec`,
enable it explicitly:

```sh
./configure --enable-setuid-install
make
make install
```

This mode should be treated as a separate risk surface. It is not required for
ordinary Docker entrypoint use.

## Review Guidance

- Do not assume setuid is part of the production Docker path.
- Treat any `--pidns` change as privilege-sensitive, even if the Docker path is
  unaffected.
- Keep privileged namespace tests opt-in; the default `make check` path should
  remain usable without elevated privileges.
- Run `tests/pidns-validation.sh` under root only when explicitly validating
  PID namespace behavior.
