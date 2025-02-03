#pragma once

#include "iexec.h"
#include <stdio.h>
#include <sys/types.h>

typedef enum iexec_pidns_mode {
  IEXEC_PIDNS_MODE_INHERIT,
  IEXEC_PIDNS_MODE_NEW,
  IEXEC_PIDNS_MODE_ENTER_BY_PID,
  IEXEC_PIDNS_MODE_ENTER_BY_FILE,
  IEXEC_PIDNS_MODE_ENTER_BY_FD
} iexec_pidns_mode_t;

typedef struct iexec_option {
  int deathsig;
  iexec_pidns_mode_t pidns;
  pid_t pidns_pid;
  const char *pidns_filename;
  int pidns_fd;
  int envind;
} iexec_option_t;

void iexec_option_print_usage(FILE *stream);
void iexec_option_parse(int argc, char **argv, struct iexec_option *ctx);
void iexec_option_init(struct iexec_option *ctx);

int iexec_parse_command_index(int argc, char **argv);
