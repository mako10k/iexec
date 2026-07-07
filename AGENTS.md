# Repository Guidelines

## Project Structure & Module Organization

This is a small Autotools-based C project for `iexec`, a Docker-style
init/reaper wrapper. Source files live in `src/`; `src/Makefile.am` defines the
binary and compilation flags. Behavioral tests live in `tests/`. Project
direction and operational notes live in `docs/`, especially `docs/backlog.md`,
`docs/docker.md`, `docs/privilege.md`, and `docs/pidns-validation.md`.

## Build, Test, and Development Commands

- `autoreconf -fi`: regenerate Autotools files from `configure.ac` and
  `Makefile.am`.
- `./configure`: configure the local build with default non-setuid install
  behavior.
- `./configure --enable-setuid-install`: opt in to setuid install only for
  deliberate non-Docker PID namespace validation.
- `make V=1`: build `src/iexec` with verbose compiler output.
- `make check V=1`: run the non-privileged test suite. Docker tests skip unless
  explicitly enabled.
- `IEXEC_TEST_DOCKER=1 make check`: run the Docker entrypoint integration test
  on a Docker-capable host.
- `sudo -n env IEXEC_TEST_PIDNS=1 IEXEC_TEST_BINARY="$PWD/src/iexec" tests/pidns-validation.sh`:
  run privileged PID namespace validation explicitly.
- `make distcheck V=1`: verify source distribution completeness.

## Coding Style & Naming Conventions

Use C99 and keep code compatible with the current warning policy in
`src/Makefile.am`: `-Wall -Wextra -Werror -Wpedantic`. Indent C code with two
spaces, matching existing files. Public project functions use the `iexec_`
prefix. Keep headers paired with implementation files where practical, for
example `iexec_wait.c` and `iexec_wait.h`.

## Testing Guidelines

Tests are POSIX shell scripts wired through Automake `TESTS` in `Makefile.am`.
Name new tests descriptively under `tests/`, such as `tests/iexec-behavior.sh`.
Default tests must not require root, setuid, Docker, or network access.
Privileged PID namespace and Docker checks should remain opt-in.

## Commit & Pull Request Guidelines

History uses short imperative commit subjects, for example `Fix main child
status propagation` and `Add default CI workflow`. Keep commits focused on one
logical change. Pull requests should describe the behavior change, list
verification commands, and call out any privilege, setuid, Docker, or namespace
impact.

## Security & Configuration Notes

The ordinary Docker init/reaper path must not require setuid. Treat `--pidns`
and `--enable-setuid-install` as privilege-sensitive validation features.
Remote `gh` or `git` operations in this repository should use:

```sh
secdat exec --inject secret:only=GH_TOKEN -- gh ...
secdat exec --inject secret:only=GH_TOKEN -- git ...
```
