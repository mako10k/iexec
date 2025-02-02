#include <stdarg.h>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

struct iexec_option {
  int deathsig;
  enum {
    PIDNS_INHERIT,
    PIDNS_NEW,
    PIDNS_ENTER_BY_PID,
    PIDNS_ENTER_BY_FILE,
    PIDNS_ENTER_BY_FD
  } pidns;
  pid_t pidns_pid;
  const char *pidns_filename;
  int pidns_fd;
};

static enum ielevel {
  iefatal,
  ieerror,
  iewarn,
  ieinfo,
  iedebug
} verbose = iewarn;

static void ielog(enum ielevel level, const char *msg, ...) {
  if (verbose < level) {
    return;
  }
  va_list ap;
  va_start(ap, msg);
  vfprintf(stderr, msg, ap);
  va_end(ap);
}

/**
 * Parse signal name or number
 * @param sigspec signal name or number
 * @return signal number or -1 if invalid
 */
static int parse_optval_signal(const char *sigspec) {
  if (sigspec == NULL || *sigspec == '\0') {
    return -1;
  }
  if (strcasecmp(optarg, "NONE") == 0) {
    return 0;
  }
  {
    char *p;
    long signo = strtol(sigspec, &p, 0);
    if (sigspec != p && *p == '\0') {
      if (0 < signo && signo < NSIG) {
        return signo;
      }
      return -1;
    }
  }
  if (strncasecmp(sigspec, "SIG", 3) == 0) {
    sigspec += 3;
  }
  for (int signo = 1; signo < NSIG; signo++) {
    const char *signame = sigabbrev_np(signo);
    if (signame == NULL) {
      continue;
    }
    if (strcasecmp(sigspec, signame) == 0) {
      return signo;
    }
  }
  return -1;
}

static int parse_optval_pidns(const char *pidns, struct iexec_option *ctx) {
  if (pidns == NULL || *pidns == '\0') {
    ctx->pidns = PIDNS_NEW;
    return 0;
  }
  if (strcasecmp(pidns, "inherit") == 0) {
    ctx->pidns = PIDNS_INHERIT;
    return 0;
  }
  if (strcasecmp(pidns, "new") == 0) {
    ctx->pidns = PIDNS_NEW;
    return 0;
  }
  if (strncasecmp(pidns, "pid:", 4) == 0) {
    char *p;
    long pid = strtol(pidns + 4, &p, 0);
    if (pidns + 4 != p && *p == '\0') {
      ctx->pidns = PIDNS_ENTER_BY_PID;
      ctx->pidns_pid = pid;
      return 0;
    }
    return -1;
  }
  if (strcasecmp(pidns, "file:") == 0) {
    ctx->pidns = PIDNS_ENTER_BY_FILE;
    ctx->pidns_filename = pidns + 5;
    return 0;
  }
  if (strcasecmp(pidns, "fd:") == 0) {
    char *p;
    long fd = strtol(pidns + 3, &p, 0);
    if (pidns + 3 != p && *p == '\0') {
      ctx->pidns = PIDNS_ENTER_BY_FD;
      ctx->pidns_fd = fd;
      return 0;
    }
    return -1;
  }
  char *p;
  long pid = strtol(pidns, &p, 0);
  if (pidns != p && *p == '\0') {
    ctx->pidns = PIDNS_ENTER_BY_PID;
    ctx->pidns_pid = pid;
    return 0;
  }
  ctx->pidns = PIDNS_ENTER_BY_FILE;
  ctx->pidns_filename = pidns;
  return 0;
}

static void wait_for_sigchld_or_timeout(void) {
  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGCHLD);
  struct timespec timeout = {1, 0};
  int sig = sigtimedwait(&mask, NULL, &timeout);
  if (sig == -1) {
    if (errno == EAGAIN) {
      return;
    }
    ielog(iefatal, "sigtimedwait: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
}

static void wait_forever(void) __attribute__((noreturn));

static void wait_forever(void) {
  // just run as reaper if no command and running as init
  int status;
  while (1) {
    pid_t pid_reported = wait(&status);
    if (pid_reported == -1) {
      if (errno == EINTR) {
        continue;
      }
      if (errno != ECHILD) {
        ielog(iefatal, "waitpid: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
      }
    }
    wait_for_sigchld_or_timeout();
  }
}

static void wait_for_children(pid_t pid_child) __attribute__((noreturn));

static void wait_for_children(pid_t pid_child) {
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
          exit(status_child);
        }
        ielog(iefatal, "No child process\n");
        exit(EXIT_FAILURE);
      }
      ielog(iefatal, "waitpid: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
    }
    if (pid_reported == pid_child) {
      status_child = status;
    }
  }
}

static void exec_command(int argc, char **argv, struct iexec_option *ctx)
    __attribute__((noreturn));

static void exec_command(int argc, char **argv, struct iexec_option *ctx) {
  pid_t pid_self = getpid();

  if (pid_self != 1) {
    if (prctl(PR_SET_CHILD_SUBREAPER, 1) == -1) {
      ielog(ieerror, "error while setting PR_SET_CHILD_SUBREAPER: %s\n",
            strerror(errno));
      exit(EXIT_FAILURE);
    }
  }

  if (ctx->deathsig) {
    if (prctl(PR_SET_PDEATHSIG, ctx->deathsig) == -1) {
      ielog(ieerror, "error while setting PR_SET_PDEATHSIG: %s\n",
            strerror(errno));
      exit(EXIT_FAILURE);
    }
  }

  int cmdind = 0;
  while (cmdind < argc && strchr(argv[cmdind], '=') != NULL) {
    cmdind++;
  }

  pid_t pid_child = -1;
  if (cmdind < argc) {
    pid_child = fork();
    if (pid_child == -1) {
      ielog(iefatal, "fork: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
    }
    if (pid_child == 0) {
      for (int i = optind; i < cmdind; i++) {
        putenv(argv[i]);
      }
      execvp(argv[cmdind], &argv[cmdind]);
      ielog(ieinfo, "execvp: %s\n", strerror(errno));
      exit(127);
    }
  } else if (pid_self != 1) {
    ielog(ieerror, "No command specified\n");
    ielog(ieerror,
          "Command needs for non-init process (init pid:1, this pid:%d)\n",
          pid_self);
    exit(EXIT_FAILURE);
  } else {
    wait_forever();
  }

  wait_for_children(pid_child);
}

static void drop_privilege(void) {
  uid_t uid = getuid();
  if (uid != 0) {
    int ret = seteuid(uid);
    if (ret == -1) {
      ielog(iefatal, "seteuid: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
    }
  }
}

static void drop_privilege_permanently(void) {
  uid_t uid = getuid();
  if (uid != 0) {
    int ret = setuid(uid);
    if (ret == -1) {
      ielog(iefatal, "setuid: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
    }
  }
}

static void raise_privilege(void) {
  int ret = seteuid(0);
  if (ret == -1) {
    ielog(ieerror, "Cannot promote privilege: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
}

static void print_usage(FILE *stream) {
  fprintf(stream, "Usage: %s [OPTION]... [COMMAND] [ARG]...\n",
          program_invocation_name);
  fprintf(stream, "Run COMMAND with PID namespace\n");
  fprintf(stream, "\n");
  fprintf(stream, "Options:\n");
  fprintf(stream, "  -k, --deathsig=SIGNAME|SIGNO  set parent death signal\n");
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

static void parse_options(int argc, char **argv, struct iexec_option *ctx) {
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
      ctx->deathsig = parse_optval_signal(optarg);
      if (ctx->deathsig == -1) {
        fprintf(stderr, "Invalid signal: %s\n", optarg);
        exit(EXIT_FAILURE);
      }
      break;

    case 'p':
      if (parse_optval_pidns(optarg, ctx) == -1) {
        fprintf(stderr, "Invalid pidns: %s\n", optarg);
        exit(EXIT_FAILURE);
      }
      break;

    case 'v':
      if (verbose < iedebug) {
        verbose++;
      }
      break;

    case 'q':
      if (verbose > iefatal) {
        verbose--;
      }
      break;

    case 'V':
      printf("%s\n", PACKAGE_STRING);
      exit(EXIT_SUCCESS);

    case 'h':
      print_usage(stdout);
      exit(EXIT_SUCCESS);

    default:
      fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
      exit(EXIT_FAILURE);
    }
  }
}

static void init_options(struct iexec_option *ctx) {
  ctx->deathsig = SIGHUP;
  ctx->pidns = PIDNS_INHERIT;
  ctx->pidns_pid = 0;
  ctx->pidns_filename = NULL;
  ctx->pidns_fd = 0;
}

static void pidns_new(void) {
  raise_privilege();
  int ret = unshare(CLONE_NEWPID);
  if (ret == -1) {
    ielog(iefatal, "unshare while creating new PID namespace: %s\n",
          strerror(errno));
    exit(EXIT_FAILURE);
  }
  drop_privilege();
}

static void pidns_enter_by_fd_internal(int fd) {
  int ret = setns(fd, CLONE_NEWPID);
  if (ret == -1) {
    ielog(ieerror, "setns while entering PID namespace: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
}

static void pidns_enter_by_fd(int fd) {
  raise_privilege();
  pidns_enter_by_fd_internal(fd);
  drop_privilege();
  close(fd);
}

static void pidns_enter_by_file(const char *path) {
  ielog(ieinfo, "Entering PID namespace by file=%s\n", path);
  raise_privilege();
  int fd = open(path, O_RDONLY);
  if (fd == -1) {
    ielog(ieerror, "open while entering PID namespace: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
  pidns_enter_by_fd_internal(fd);
  drop_privilege();
  close(fd);
}

static void pidns_enter_by_pid(pid_t pid) {
  ielog(ieinfo, "Entering PID namespace by PID=%d\n", pid);
  char path[PATH_MAX];
  snprintf(path, sizeof(path), "/proc/%d/ns/pid", pid);
  pidns_enter_by_file(path);
}

static void pidns_prepare(void) {
  ielog(ieinfo, "Preparing PID namespace (pid:%d)\n", getpid());
  raise_privilege();
  if (unshare(CLONE_NEWNS) == -1) {
    ielog(iefatal, "unshare while preparing PID namespace: %s\n",
          strerror(errno));
    exit(EXIT_FAILURE);
  }
  int ret = mount("none", "/proc", NULL, MS_REC | MS_PRIVATE, NULL);
  if (ret == -1) {
    ielog(iefatal, "mount while preparing PID namespace: %s\n",
          strerror(errno));
    exit(EXIT_FAILURE);
  }
  ret = mount("proc", "/proc", "proc", 0, NULL);
  if (ret == -1) {
    ielog(iefatal, "mount while preparing PID namespace: %s\n",
          strerror(errno));
    exit(EXIT_FAILURE);
  }
  drop_privilege_permanently();
}

static void print_warning(struct iexec_option *ctx) {
  if (ctx->pidns != PIDNS_INHERIT) {
    pid_t pid_self = getpid();
    if (pid_self == 1) {
      ielog(iewarn, "Warning: running as init process\n");
      ielog(iewarn, "Warning: -p or --pidns shuld not be used with init\n");
    }
    if (ctx->pidns == PIDNS_NEW) {
      ielog(iewarn, "Warning: running as init process in new PID namespace\n");
    }
  }
}

static void run_iexec(int argc, char **argv, struct iexec_option *ctx) {
  switch (ctx->pidns) {
  case PIDNS_INHERIT:
    drop_privilege_permanently();
    return exec_command(argc, argv, ctx);

  case PIDNS_NEW:
    pidns_new();
    break;

  case PIDNS_ENTER_BY_PID:
    pidns_enter_by_pid(ctx->pidns_pid);
    break;

  case PIDNS_ENTER_BY_FILE:
    pidns_enter_by_file(ctx->pidns_filename);
    break;

  case PIDNS_ENTER_BY_FD:
    pidns_enter_by_fd(ctx->pidns_fd);
    break;

  default:
    abort();
  }

  pid_t pid_child = fork();
  if (pid_child == -1) {
    ielog(iefatal, "fork: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
  if (pid_child == 0) {

    if (ctx->pidns != PIDNS_INHERIT) {
      pidns_prepare();
    } else {
      drop_privilege_permanently();
    }

    return exec_command(argc, argv, ctx);
  }

  drop_privilege_permanently();
  if (ctx->pidns == PIDNS_NEW) {
    ielog(
        iewarn,
        "Warning: to enter this new PID namespace, use %s --pidns=pid:%d %s\n",
        program_invocation_name, pid_child, getenv("SHELL") ?: "/bin/sh");
  }

  wait_for_children(pid_child);
}

int main(int argc, char **argv) {
  struct iexec_option ctx;
  drop_privilege();
  init_options(&ctx);
  parse_options(argc, argv, &ctx);
  print_warning(&ctx);
  run_iexec(argc - optind, argv + optind, &ctx);
}