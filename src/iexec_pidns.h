#pragma once

#include "iexec.h"
#include <sys/types.h>

void iexec_pidns_new(void);
void iexec_pidns_enter_by_fd_internal(int fd);
void iexec_pidns_enter_by_fd(int fd);
void iexec_pidns_enter_by_file(const char *path);
void iexec_pidns_enter_by_pid(pid_t pid);
void iexec_pidns_prepare(void);
