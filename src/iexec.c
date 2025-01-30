#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <unistd.h>

enum { wait_never, wait_pid, wait_pgid, wait_all, wait_forever };

static int parse_sig(const char *sigspec) {
  if (sigspec == NULL || *sigspec == '\0') {
    return -1;
  }
  {
    char *p;
    long signo = strtol(sigspec, &p, 10);
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

static int parse_bool(const char *boolspec) {
  if (boolspec == NULL || *boolspec == '\0') {
    return -1;
  }
  {
    char *p;
    long value = strtol(boolspec, &p, 10);
    if (boolspec != p && *p == '\0') {
      return value != 0;
    }
  }
  if (strcasecmp(boolspec, "true") == 0 || strcasecmp(boolspec, "yes") == 0 ||
      strcasecmp(boolspec, "on") == 0) {
    return 1;
  }
  if (strcasecmp(boolspec, "false") == 0 || strcasecmp(boolspec, "no") == 0 ||
      strcasecmp(boolspec, "off") == 0) {
    return 0;
  }
  return -1;
}

static int parse_wait(const char *waitspec) {
  if (waitspec == NULL || *waitspec == '\0') {
    return -1;
  }
  if (strcasecmp(waitspec, "never") == 0 || strcasecmp(waitspec, "no") == 0 ||
      strcasecmp(waitspec, "off") == 0) {
    return wait_never;
  }
  if (strcasecmp(waitspec, "invoked") == 0 ||
      strcasecmp(waitspec, "child") == 0 || strcasecmp(waitspec, "yes") == 0 ||
      strcasecmp(waitspec, "on") == 0) {
    return wait_pid;
  }
  if (strcasecmp(waitspec, "group") == 0 ||
      strcasecmp(waitspec, "pgroup") == 0 ||
      strcasecmp(waitspec, "childgroup") == 0 ||
      strcasecmp(waitspec, "childpgroup") == 0 ||
      strcasecmp(waitspec, "invokedgroup") == 0 ||
      strcasecmp(waitspec, "invokedpgroup") == 0) {
    return wait_pgid;
  }
  if (strcasecmp(waitspec, "all") == 0 || strcasecmp(waitspec, "any") == 0) {
    return wait_all;
  }
  if (strcasecmp(waitspec, "forever") == 0) {
    return wait_forever;
  }
  return -1;
}

int main(int argc, char **argv) {
  int opt;
  int opt_subreaper = 1;
  int opt_deathsig = 0;
  int opt_wait = wait_pid;
  static struct option long_options[] = {
      {"subreaper", no_argument, NULL, 's'},
      {"deathsig", required_argument, NULL, 'd'},
      {"wait", no_argument, NULL, 'w'},
      {"help", no_argument, NULL, 'h'},
      {NULL, 0, NULL, 0}};
  while ((opt = getopt_long(argc, argv, "+s:d:w:h", long_options, NULL)) !=
         -1) {
    switch (opt) {

    case 's':
      opt_subreaper = parse_bool(optarg);
      if (opt_subreaper == -1) {
        fprintf(stderr, "Invalid argument: %s\n", optarg);
        return EXIT_FAILURE;
      }
      break;

    case 'd':
      if (strcmp(optarg, "none") == 0) {
        opt_deathsig = 0;
        break;
      }
      opt_deathsig = parse_sig(optarg);
      if (opt_deathsig == -1) {
        fprintf(stderr, "Invalid signal: %s\n", optarg);
        return EXIT_FAILURE;
      }
      break;

    case 'w':
      opt_wait = parse_wait(optarg);
      if (opt_wait == -1) {
        fprintf(stderr, "Invalid argument: %s\n", optarg);
        return EXIT_FAILURE;
      }
      break;

    case 'h':
      printf("Usage: %s [OPTION] [ENV=VAL ...] [COMMAND [ARG ...]]\n", argv[0]);
      printf("Execute COMMAND with ARGs\n");
      printf("Options:\n");
      printf(
          "  -s, --subreaper=<bool>          set subreaper process  [true]\n");
      printf(
          "  -d, --deathsig=<num>|<signame>  set death signal       [none]\n");
      printf("  -w, --wait=never|no             never wait\n");
      printf("  -w, --wait=child|yes            wait for child only    "
             "[default]\n");
      printf(
          "  -w, --wait=pgroup               wait for child process group\n");
      printf("  -w, --wait=any                  wait for any processes\n");
      printf("  -w, --wait=forever              wait forever\n");
      printf("  -h, --help                      display this help and exit\n");
      return EXIT_SUCCESS;

    default:
      fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
      return EXIT_FAILURE;
    }
  }

  if (opt_subreaper) {
    if (prctl(PR_SET_CHILD_SUBREAPER, 1) == -1) {
      perror("error while setting PR_SET_CHILD_SUBREAPER");
      return EXIT_FAILURE;
    }
  }

  if (opt_deathsig) {
    if (prctl(PR_SET_PDEATHSIG, opt_deathsig) == -1) {
      perror("error while setting PR_SET_PDEATHSIG");
      return EXIT_FAILURE;
    }
  }

  int cmdind = optind;
  while (cmdind < argc && strchr(argv[cmdind], '=') != NULL) {
    cmdind++;
  }
  if (cmdind < argc) {
    pid_t pid = fork();
    if (pid == -1) {
      perror("fork");
      return EXIT_FAILURE;
    }
    if (pid == 0) {
      for (int i = optind; i < cmdind; i++) {
        putenv(argv[i]);
      }
      execvp(argv[cmdind], &argv[cmdind]);
      perror(argv[cmdind]);
      return 127;
    }
    pid_t wpid;

    switch (opt_wait) {
    case wait_never:
      return EXIT_SUCCESS;
    case wait_pid:
      wpid = pid;
      break;
    case wait_pgid:
      wpid = -pid;
      break;
    case wait_all:
      wpid = -1;
      break;
    case wait_forever:
      wpid = -1;
      break;
    }

    while (1) {
      int status, status_ret = 0;
      pid_t pid_ret = waitpid(wpid, &status, 0);
      if (pid_ret == -1) {
        if (errno == EINTR) {
          continue;
        }
        if (errno == ECHILD) {
          if (opt_wait == wait_forever) {
            if (getpid() != 1)
              break;
            sigset_t mask;
            sigemptyset(&mask);
            sigaddset(&mask, SIGCHLD);
            struct timespec timeout = {1, 0};
            if (sigtimedwait(&mask, NULL, &timeout) == -1) {
              perror("sigtimedwait");
              return EXIT_FAILURE;
            }
            continue;
          }
          return status_ret;
        }
        perror("waitpid");
        return EXIT_FAILURE;
      }
      if (pid_ret == pid) {
        status_ret = status;
      }
    }

  } else {
    switch (opt_wait) {
    case wait_never:
      return EXIT_SUCCESS;
    case wait_pid:
      return EXIT_SUCCESS;
    case wait_pgid:
      return EXIT_SUCCESS;
    case wait_all:
      break;
    case wait_forever:
      break;
    }
  }
  while (1) {
    int ret = pause();
    if (ret == -1 && errno == EINTR) {
      continue;
    }
    perror("pause");
    return EXIT_FAILURE;
  }
}