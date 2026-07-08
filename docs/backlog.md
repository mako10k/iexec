# Backlog

This backlog preserves the project direction:

- Primary goal: Docker-style init/reaper for orphaned process cleanup.
- Secondary goal: non-Docker PID namespace validation for the same behavior.
- Non-goal: a general-purpose PID namespace administration tool.

## P0

- [x] Fix main child result propagation.
  `iexec` must exit with the main child's exit code when it exits normally, and
  should report signal termination using the conventional shell-compatible
  status.

- [x] Forward shutdown signals to the main child.
  Signals such as `SIGTERM`, `SIGINT`, `SIGHUP`, and `SIGQUIT` received by
  `iexec` should be forwarded so Docker stop and similar shutdown paths can
  terminate the wrapped process gracefully.

- [x] Add behavioral tests for the init/reaper contract.
  Cover normal exit status, non-zero exit status, signal termination, signal
  forwarding, orphaned child reaping, and non-PID 1 no-command behavior.

## P1

- [x] Document and test the Docker entrypoint use case.
  Include a minimal container example and expected behavior for shutdown,
  zombie reaping, and command result propagation.

- [x] Clarify privilege and install policy.
  The ordinary Docker init/reaper path should be understandable without setuid
  assumptions. PID namespace validation mode may need elevated privileges, but
  that risk surface should be documented separately.

- [x] Keep `--pidns` focused on non-Docker validation.
  Tests and documentation should make clear that `--pidns` is for validating
  init/reaper behavior outside Docker, not for repositioning the project as a
  general PID namespace manager.

## P2

- [x] Add project metadata.
  Add license information and release/install notes before treating the project
  as redistributable.

- [x] Consider CI after behavioral tests exist.
  CI should at least run the build and non-privileged tests; privileged
  namespace tests may need to remain opt-in.

- [x] Harden CLI regression coverage.
  Cover version and help output, `--deathsig` parsing, explicit inherited PID
  namespace mode, invalid option diagnostics, and leading environment
  assignments.

- [x] Prefer capability install over setuid fallback.
  Use `cap_sys_admin+ep` for explicit non-Docker PID namespace validation
  installs, keep setuid as a mutually exclusive fallback, and validate the
  install policy before release.

- [x] Add explicit PID namespace privilege and exec contract gates.
  Require runtime opt-in for privileged `--pidns` use, keep `seteuid(0)` limited
  to the setuid fallback, and verify uid, gid, supplementary groups, and
  capabilities before executing the wrapped command.
