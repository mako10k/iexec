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

/**
 * Parse signal name or number
 * @param sigspec signal name or number
 * @return signal number or -1 if invalid
 */
int iexec_option_parse_signal(const char *sigspec);

/**
 * Parse PID namespace mode
 * @param pidns PID namespace mode
 * @param ctx context
 * @return 0 if success, -1 if invalid
 */
int iexec_option_parse_pidns_mode(const char *pidns, iexec_option_t *ctx);

void iexec_option_print_usage(FILE *stream);
void iexec_option_parse(int argc, char **argv, struct iexec_option *ctx);
void iexec_option_init(struct iexec_option *ctx);

int iexec_parse_command_index(int argc, char **argv);
