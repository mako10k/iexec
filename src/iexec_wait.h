#pragma once

#include "iexec.h"
#include <sys/types.h>

void iexec_wait_forever(void) __attribute__((noreturn));
void iexec_wait_for_sigchld_or_timeout(void);
void iexec_wait_for_children(pid_t pid_child) __attribute__((noreturn));
