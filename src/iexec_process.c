#include "iexec_process.h"
#include "iexec_print.h"
#include <assert.h>
#include <stdlib.h>
#include <sys/prctl.h>
#include <unistd.h>

void iexec_prctl_set_child_subreaper(void) {
  int ret = prctl(PR_SET_CHILD_SUBREAPER, 1);
  if (ret == -1) {
    iexec_printf(IEXEC_PRINT_LEVEL_FATAL,
                 "prctl(PR_SET_CHILD_SUBREAPER): %s\n", iexec_strerror(iexec_errno()));
    iexec_exit(IEXEC_EXIT_FAILURE);
  }
}

void iexec_prctl_set_pdeathsig(int signum) {
  int ret = prctl(PR_SET_PDEATHSIG, signum);
  if (ret == -1) {
    iexec_printf(IEXEC_PRINT_LEVEL_FATAL, "prctl(PR_SET_PDEATHSIG): %s\n", iexec_strerror(iexec_errno()));
    iexec_exit(IEXEC_EXIT_FAILURE);
  }
}

pid_t iexec_fork(void) {
  pid_t pid = fork();
  if (pid == -1) {
    iexec_printf(IEXEC_PRINT_LEVEL_FATAL, "fork: %s\n", iexec_strerror(iexec_errno()));
    iexec_exit(IEXEC_EXIT_FAILURE);
  }
  return pid;
}

char *iexec_getenv(const char *name) { return getenv(name); }

void iexec_put_envs(int argc, char **argv) {
  for (int i = 0; i < argc; i++) {
    putenv(argv[i]);
  }
}

void iexec_execvp(const char *file, char *const argv[]) {
  int ret = execvp(file, argv);
  assert(ret == -1);
  iexec_printf(IEXEC_PRINT_LEVEL_INFORMATION, "execvp: %s\n", iexec_strerror(iexec_errno()));
  iexec_exit(IEXEC_EXIT_NOCMD);
}

pid_t iexec_getpid(void) { return getpid(); }
void iexec_exit(int status) { exit(status); }
void iexec_abort(void) { abort(); }