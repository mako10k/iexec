#include "iexec_option.h"
#include "iexec_print.h"
#include "iexec_process.h"
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

/**
 * Parse signal name or number
 * @param sigspec signal name or number
 * @return signal number or -1 if invalid
 */
int iexec_option_parse_signal(const char *sigspec) {
  if (sigspec == NULL || *sigspec == '\0') {
    return -1;
  }
  if (strcasecmp(optarg, "NONE") == 0) {
    return 0;
  }
  {
    char *p;
    long signum = strtol(sigspec, &p, 0);
    if (sigspec != p && *p == '\0') {
      if (0 < signum && signum < NSIG) {
        return signum;
      }
      return -1;
    }
  }
  if (strncasecmp(sigspec, "SIG", 3) == 0) {
    sigspec += 3;
  }
  for (int signum = 1; signum < NSIG; signum++) {
    const char *signame = sigabbrev_np(signum);
    if (signame == NULL) {
      continue;
    }
    if (strcasecmp(sigspec, signame) == 0) {
      return signum;
    }
  }
  return -1;
}

int iexec_option_parse_pidns_mode(const char *pidns, iexec_option_t *ctx) {
  if (pidns == NULL || *pidns == '\0') {
    ctx->pidns = IEXEC_PIDNS_MODE_NEW;
    return 0;
  }
  if (strcasecmp(pidns, "inherit") == 0) {
    ctx->pidns = IEXEC_PIDNS_MODE_INHERIT;
    return 0;
  }
  if (strcasecmp(pidns, "new") == 0) {
    ctx->pidns = IEXEC_PIDNS_MODE_NEW;
    return 0;
  }
  if (strncasecmp(pidns, "pid:", 4) == 0) {
    char *p;
    long pid = strtol(pidns + 4, &p, 0);
    if (pidns + 4 != p && *p == '\0') {
      ctx->pidns = IEXEC_PIDNS_MODE_ENTER_BY_PID;
      ctx->pidns_pid = pid;
      return 0;
    }
    return -1;
  }
  if (strcasecmp(pidns, "file:") == 0) {
    ctx->pidns = IEXEC_PIDNS_MODE_ENTER_BY_FILE;
    ctx->pidns_filename = pidns + 5;
    return 0;
  }
  if (strcasecmp(pidns, "fd:") == 0) {
    char *p;
    long fd = strtol(pidns + 3, &p, 0);
    if (pidns + 3 != p && *p == '\0') {
      ctx->pidns = IEXEC_PIDNS_MODE_ENTER_BY_FD;
      ctx->pidns_fd = fd;
      return 0;
    }
    return -1;
  }
  char *p;
  long pid = strtol(pidns, &p, 0);
  if (pidns != p && *p == '\0') {
    ctx->pidns = IEXEC_PIDNS_MODE_ENTER_BY_PID;
    ctx->pidns_pid = pid;
    return 0;
  }
  ctx->pidns = IEXEC_PIDNS_MODE_ENTER_BY_FILE;
  ctx->pidns_filename = pidns;
  return 0;
}

void iexec_option_print_usage(FILE *stream) {
  fprintf(stream, "Usage: %s [OPTION]... [COMMAND] [ARG]...\n",
          program_invocation_name);
  fprintf(stream, "Run COMMAND with PID namespace\n");
  fprintf(stream, "\n");
  fprintf(stream, "Options:\n");
  fprintf(stream, "  -k, --deathsig=SIGNAME|SIGNUM set parent death signal\n");
  fprintf(stream, "  -p, --pidns[=MODE]            set PID namespace\n");
  fprintf(stream, "      MODE can be:\n");
  fprintf(stream, "        inherit                   inherit PID namespace\n");
  fprintf(stream, "        new                       create new PID namespace "
                  "   (default when \"=MODE\" is omitted)\n");
  fprintf(stream, "        pid:PID                   enter PID namespace by "
                  "PID  (\"pid:\" can omit)\n");
  fprintf(stream, "        file:PATH                 enter PID namespace by "
                  "file (\"file:\" can omit)\n");
  fprintf(stream, "        fd:FD                     enter PID namespace by "
                  "file descriptor\n");
  fprintf(stream, "  -v, --verbose                 verbose mode\n");
  fprintf(stream, "  -q, --quiet                   quiet mode\n");
  fprintf(stream, "  -V, --version                 display version and exit\n");
  fprintf(stream,
          "  -h, --help                    display this help and exit\n");
}

void iexec_option_parse(int argc, char **argv, struct iexec_option *ctx) {
  int opt;
  static struct option long_options[] = {
      {"deathsig", required_argument, NULL, 'k'},
      {"pidns", optional_argument, NULL, 'p'},
      {"verbose", no_argument, NULL, 'v'},
      {"quiet", no_argument, NULL, 'q'},
      {"version", no_argument, NULL, 'V'},
      {"help", no_argument, NULL, 'h'},
      {NULL, 0, NULL, 0}};
  while ((opt = getopt_long(argc, argv, "+k:p::vqVh", long_options, NULL)) !=
         -1) {
    switch (opt) {

    case 'k':
      ctx->deathsig = iexec_option_parse_signal(optarg);
      if (ctx->deathsig == -1) {
        fprintf(stderr, "Invalid signal: %s\n", optarg);
        iexec_exit(IEXEC_EXIT_FAILURE);
      }
      break;

    case 'p':
      if (iexec_option_parse_pidns_mode(optarg, ctx) == -1) {
        fprintf(stderr, "Invalid pidns: %s\n", optarg);
        iexec_exit(IEXEC_EXIT_FAILURE);
      }
      break;

    case 'v':
      iexec_printf_increase_verbosity();
      break;

    case 'q':
      iexec_printf_decrease_verbosity();
      break;

    case 'V':
      printf("%s\n", PACKAGE_STRING);
      iexec_exit(IEXEC_EXIT_SUCCESS);

    case 'h':
      iexec_option_print_usage(stdout);
      iexec_exit(IEXEC_EXIT_SUCCESS);

    default:
      fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
      iexec_exit(IEXEC_EXIT_FAILURE);
    }
  }
  ctx->envind = optind;
}

void iexec_option_init(struct iexec_option *ctx) {
  ctx->deathsig = SIGHUP;
  ctx->pidns = IEXEC_PIDNS_MODE_INHERIT;
  ctx->pidns_pid = 0;
  ctx->pidns_filename = NULL;
  ctx->pidns_fd = 0;
  ctx->envind = 0;
}

int iexec_parse_command_index(int argc, char **argv) {
  for (int i = 0; i < argc; i++) {
    if (strchr(argv[i], '=') == NULL) {
      return i;
    }
  }
  return argc;
}
