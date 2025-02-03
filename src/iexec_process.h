#pragma once

#include "iexec.h"
#include <sys/types.h>

enum {
  IEXEC_EXIT_SUCCESS = 0,
  IEXEC_EXIT_FAILURE = 1,
  IEXEC_EXIT_NOCMD = 127,
};

void iexec_prctl_set_child_subreaper(void);
void iexec_prctl_set_pdeathsig(int signum);
pid_t iexec_fork(void);
char *iexec_getenv(const char *name);
void iexec_put_envs(int argc, char **argv);
void iexec_execvp(const char *file, char *const argv[])
    __attribute__((noreturn));
void iexec_exit(int status) __attribute__((noreturn));
pid_t iexec_getpid(void);
void iexec_abort(void) __attribute__((noreturn));