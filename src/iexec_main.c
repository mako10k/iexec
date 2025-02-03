#include "iexec_main.h"
#include "iexec_pidns.h"
#include "iexec_print.h"
#include "iexec_privilege.h"
#include "iexec_process.h"
#include "iexec_wait.h"

void iexec_mainloop(int argc, char **argv, iexec_option_t *ctx) {
  pid_t pid_self = iexec_getpid();

  if (pid_self != 1) {
    iexec_prctl_set_child_subreaper();
  }

  if (ctx->deathsig) {
    iexec_prctl_set_pdeathsig(ctx->deathsig);
  }

  int cmdind = iexec_parse_command_index(argc, argv);

  pid_t pid_child = -1;
  if (cmdind < argc) {
    pid_child = iexec_fork();
    if (pid_child == 0) {
      iexec_put_envs(cmdind, argv);
      iexec_execvp(argv[cmdind], argv + cmdind);
    }
  } else if (pid_self != 1) {
    iexec_printf(IEXEC_PRINT_LEVEL_ERROR, "No command specified\n");
    iexec_printf(
        IEXEC_PRINT_LEVEL_ERROR,
        "Command needs for non-init process (init pid:1, this pid:%d)\n",
        pid_self);
    iexec_exit(IEXEC_EXIT_FAILURE);
  } else {
    iexec_wait_forever();
  }

  iexec_wait_for_children(pid_child);
}

void iexec_print_warning(iexec_option_t *ctx) {
  if (ctx->pidns != IEXEC_PIDNS_MODE_INHERIT) {
    pid_t pid_self = iexec_getpid();
    if (pid_self == 1) {
      iexec_printf(IEXEC_PRINT_LEVEL_WARNING,
                   "Warning: running as init process\n");
      iexec_printf(IEXEC_PRINT_LEVEL_WARNING,
                   "Warning: -p or --pidns shuld not be used with init\n");
    }
    if (ctx->pidns == IEXEC_PIDNS_MODE_NEW) {
      iexec_printf(IEXEC_PRINT_LEVEL_WARNING,
                   "Warning: running as init process in new PID namespace\n");
    }
  }
}

void iexec_main(int argc, char **argv, iexec_option_t *ctx) {
  argc -= ctx->envind;
  argv += ctx->envind;
  switch (ctx->pidns) {
  case IEXEC_PIDNS_MODE_INHERIT:
    iexec_drop_privilege_permanently();
    iexec_mainloop(argc, argv, ctx);

  case IEXEC_PIDNS_MODE_NEW:
    iexec_pidns_new();
    break;

  case IEXEC_PIDNS_MODE_ENTER_BY_PID:
    iexec_pidns_enter_by_pid(ctx->pidns_pid);
    break;

  case IEXEC_PIDNS_MODE_ENTER_BY_FILE:
    iexec_pidns_enter_by_file(ctx->pidns_filename);
    break;

  case IEXEC_PIDNS_MODE_ENTER_BY_FD:
    iexec_pidns_enter_by_fd(ctx->pidns_fd);
    break;

  default:
    iexec_abort();
  }

  pid_t pid_child = iexec_fork();
  if (pid_child == 0) {

    if (ctx->pidns != IEXEC_PIDNS_MODE_INHERIT) {
      iexec_pidns_prepare();
    } else {
      iexec_drop_privilege_permanently();
    }

    iexec_mainloop(argc, argv, ctx);
  }

  iexec_drop_privilege_permanently();
  if (ctx->pidns == IEXEC_PIDNS_MODE_NEW) {
    const char *shell = iexec_getenv("SHELL");
    extern char *program_invocation_name;
    iexec_printf(
        IEXEC_PRINT_LEVEL_WARNING,
        "Warning: to enter this new PID namespace, use %s --pidns=pid:%d %s\n",
        program_invocation_name, pid_child, shell ? shell : "/bin/sh");
  }

  iexec_wait_for_children(pid_child);
}
