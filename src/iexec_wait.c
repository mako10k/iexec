#include "iexec_wait.h"
#include "iexec_print.h"
#include "iexec_process.h"
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

static void iexec_wait_for_sigchld_or_timeout(void) {
  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGCHLD);
  struct timespec timeout = {1, 0};
  int sig = sigtimedwait(&mask, NULL, &timeout);
  if (sig == -1) {
    if (errno == EAGAIN) {
      return;
    }
    iexec_printf(IEXEC_PRINT_LEVEL_FATAL, "sigtimedwait: %s\n",
                 iexec_strerror(iexec_errno()));
    iexec_exit(IEXEC_EXIT_FAILURE);
  }
}

void iexec_wait_forever(void) {
  // just run as reaper if no command and running as init
  int status;
  while (1) {
    pid_t pid_reported = wait(&status);
    if (pid_reported == -1) {
      if (errno == EINTR) {
        continue;
      }
      if (errno != ECHILD) {
        iexec_printf(IEXEC_PRINT_LEVEL_FATAL, "waitpid: %s\n",
                     iexec_strerror(iexec_errno()));
        iexec_exit(IEXEC_EXIT_FAILURE);
      }
    }
    iexec_wait_for_sigchld_or_timeout();
  }
}

void iexec_wait_for_children(pid_t pid_child) {
  int status;
  int status_child = -1;
  while (1) {
    pid_t pid_reported = wait(&status);
    if (pid_reported == -1) {
      if (errno == EINTR) {
        continue;
      }
      if (errno == ECHILD) {
        if (status_child != -1) {
          iexec_exit(status_child);
        }
        iexec_printf(IEXEC_PRINT_LEVEL_FATAL, "No child process\n");
        iexec_exit(IEXEC_EXIT_FAILURE);
      }
      iexec_printf(IEXEC_PRINT_LEVEL_FATAL, "waitpid: %s\n",
                   iexec_strerror(iexec_errno()));
      iexec_exit(IEXEC_EXIT_FAILURE);
    }
    if (pid_reported == pid_child) {
      status_child = status;
    }
  }
}
