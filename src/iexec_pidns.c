#include "iexec_pidns.h"
#include "iexec_print.h"
#include "iexec_privilege.h"
#include "iexec_process.h"
#include <fcntl.h>
#include <limits.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <unistd.h>

void iexec_pidns_new(void) {
  iexec_raise_privilege();
  int ret = unshare(CLONE_NEWPID);
  if (ret == -1) {
    iexec_printf(IEXEC_PRINT_LEVEL_FATAL,
                 "unshare while creating new PID namespace: %s\n",
                 iexec_strerror(iexec_errno()));
    iexec_exit(IEXEC_EXIT_FAILURE);
  }
  iexec_drop_privilege();
}

void iexec_pidns_enter_by_fd_internal(int fd) {
  int ret = setns(fd, CLONE_NEWPID);
  if (ret == -1) {
    iexec_printf(IEXEC_PRINT_LEVEL_ERROR,
                 "setns while entering PID namespace: %s\n",
                 iexec_strerror(iexec_errno()));
    iexec_exit(IEXEC_EXIT_FAILURE);
  }
}

void iexec_pidns_enter_by_fd(int fd) {
  iexec_raise_privilege();
  iexec_pidns_enter_by_fd_internal(fd);
  iexec_drop_privilege();
  close(fd);
}

void iexec_pidns_enter_by_file(const char *path) {
  iexec_printf(IEXEC_PRINT_LEVEL_INFORMATION,
               "Entering PID namespace by file=%s\n", path);
  iexec_raise_privilege();
  int fd = open(path, O_RDONLY);
  if (fd == -1) {
    iexec_printf(IEXEC_PRINT_LEVEL_ERROR,
                 "open while entering PID namespace: %s\n",
                 iexec_strerror(iexec_errno()));
    iexec_exit(IEXEC_EXIT_FAILURE);
  }
  iexec_pidns_enter_by_fd_internal(fd);
  iexec_drop_privilege();
  close(fd);
}

void iexec_pidns_enter_by_pid(pid_t pid) {
  iexec_printf(IEXEC_PRINT_LEVEL_INFORMATION,
               "Entering PID namespace by PID=%d\n", pid);
  char path[PATH_MAX];
  snprintf(path, sizeof(path), "/proc/%d/ns/pid", pid);
  iexec_pidns_enter_by_file(path);
}

void iexec_pidns_prepare(void) {
  iexec_printf(IEXEC_PRINT_LEVEL_INFORMATION,
               "Preparing PID namespace (pid:%d)\n", getpid());
  iexec_raise_privilege();
  if (unshare(CLONE_NEWNS) == -1) {
    iexec_printf(IEXEC_PRINT_LEVEL_FATAL,
                 "unshare while preparing PID namespace: %s\n",
                 iexec_strerror(iexec_errno()));
    iexec_exit(IEXEC_EXIT_FAILURE);
  }
  int ret = mount("none", "/proc", NULL, MS_REC | MS_PRIVATE, NULL);
  if (ret == -1) {
    iexec_printf(IEXEC_PRINT_LEVEL_FATAL,
                 "mount while preparing PID namespace: %s\n",
                 iexec_strerror(iexec_errno()));
    iexec_exit(IEXEC_EXIT_FAILURE);
  }
  ret = mount("proc", "/proc", "proc", 0, NULL);
  if (ret == -1) {
    iexec_printf(IEXEC_PRINT_LEVEL_FATAL,
                 "mount while preparing PID namespace: %s\n",
                 iexec_strerror(iexec_errno()));
    iexec_exit(IEXEC_EXIT_FAILURE);
  }
  iexec_drop_privilege_permanently();
}
