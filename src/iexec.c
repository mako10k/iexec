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

enum { wait_never, wait_pid, wait_all, wait_forever };
enum { reap_never, reap_pid, reap_all };

/**
 * Parse signal name or number
 * @param sigspec signal name or number
 * @return signal number or -1 if invalid
 */
static int parse_sig(const char *sigspec) {
  if (sigspec == NULL || *sigspec == '\0') {
    return -1;
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

/**
 * Parse boolean value
 * @param boolspec boolean value
 * @return boolean value or -1 if invalid
 */
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

/**
 * Parse wait mode
 * @param waitspec wait mode
 * @return wait mode or -1 if invalid
 */
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
  if (strcasecmp(waitspec, "all") == 0 || strcasecmp(waitspec, "any") == 0) {
    return wait_all;
  }
  if (strcasecmp(waitspec, "forever") == 0) {
    return wait_forever;
  }
  return -1;
}

static int parse_reap(const char *reapspec) {
  if (reapspec == NULL || *reapspec == '\0') {
    return -1;
  }
  if (strcasecmp(reapspec, "never") == 0 || strcasecmp(reapspec, "no") == 0 ||
      strcasecmp(reapspec, "off") == 0) {
    return reap_never;
  }
  if (strcasecmp(reapspec, "invoked") == 0 ||
      strcasecmp(reapspec, "child") == 0 || strcasecmp(reapspec, "yes") == 0 ||
      strcasecmp(reapspec, "on") == 0) {
    return reap_pid;
  }
  if (strcasecmp(reapspec, "all") == 0 || strcasecmp(reapspec, "any") == 0) {
    return reap_all;
  }
  return -1;
}

int main(int argc, char **argv) {
  int opt;
  int opt_subreaper = 1;
  int opt_deathsig = 0;
  int opt_wait = -1;
  int opt_reap = -1;
  static struct option long_options[] = {
      {"subreaper", no_argument, NULL, 's'},
      {"deathsig", required_argument, NULL, 'd'},
      {"wait", required_argument, NULL, 'w'},
      {"reap", required_argument, NULL, 'r'},
      {"help", no_argument, NULL, 'h'},
      {NULL, 0, NULL, 0}};
  while ((opt = getopt_long(argc, argv, "+s:d:w:r:h", long_options, NULL)) !=
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

    case 'r':
      opt_reap = parse_reap(optarg);
      if (opt_reap == -1) {
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
      printf("  -r, --reap=<mode>               set reap mode          [same "
             "as --wait or child]\n");
      printf("        --reap=never|no             never reap\n");
      printf("        --reap=child|yes            reap child only\n");
      printf("        --reap=any                  reap any processes\n");
      printf("  -w, --wait=<mode>               set wait mode          [same "
             "as --reap or child]\n");
      printf("        --wait=never|no             never wait\n");
      printf("        --wait=child|yes            wait for child only\n");
      printf("        --wait=any                  wait for any processes\n");
      printf("        --wait=forever              wait forever\n");
      printf("  -h, --help                      display this help and exit\n");
      return EXIT_SUCCESS;

    default:
      fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
      return EXIT_FAILURE;
    }
  }

  int cmdind = optind;
  while (cmdind < argc && strchr(argv[cmdind], '=') != NULL) {
    cmdind++;
  }

  if (opt_reap == -1 && opt_wait == -1) {
    opt_reap = reap_pid;
    opt_wait = wait_pid;
  }

  if (opt_wait == -1) {
    opt_wait = opt_reap;
  }

  if (opt_reap == -1) {
    if (opt_wait == wait_forever) {
      opt_reap = reap_all;
    } else {
      opt_reap = opt_wait;
    }
  }

  if (opt_reap < opt_wait &&
      !(opt_reap == reap_all && opt_wait == wait_forever)) {
    fprintf(stderr, "reap mode must be same or wider than wait mode\n");
    return EXIT_FAILURE;
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

  pid_t pid_wait = -1;

  if (cmdind < argc) {
    pid_t pid_child = fork();
    if (pid_child == -1) {
      perror("fork");
      return EXIT_FAILURE;
    }
    if (pid_child == 0) {
      for (int i = optind; i < cmdind; i++) {
        putenv(argv[i]);
      }
      execvp(argv[cmdind], &argv[cmdind]);
      perror(argv[cmdind]);
      return 127;
    }

    switch (opt_reap) {
    case reap_never:
      return EXIT_SUCCESS;
    case reap_pid:
      pid_wait = pid_child;
      break;
    case reap_all:
      pid_wait = -1;
      break;
    default:
      abort();
    }
  } else {
    switch (opt_wait) {
    case wait_never:
      return EXIT_SUCCESS;
    case wait_pid:
      return EXIT_SUCCESS;
    case wait_all: {
      int ret = kill(-1, 0);
      if (ret == -1) {
        if (errno == ESRCH) {
          return EXIT_SUCCESS;
        }
        perror("kill");
        return EXIT_FAILURE;
      }
      break;
    };
    case wait_forever:
      break;
    }
  }

  while (1) {
    int status, status_child = 0;
    pid_t pid_ret = waitpid(pid_wait, &status, 0);
    if (pid_ret == -1) {
      if (errno == EINTR) {
        continue;
      }
      if (errno == ECHILD) {
        if (opt_wait == wait_forever) {
          sigset_t mask;
          sigemptyset(&mask);
          sigaddset(&mask, SIGCHLD);
          struct timespec timeout = {1, 0};
          if (sigtimedwait(&mask, NULL, &timeout) == -1) {
            if (errno == EAGAIN) {
              continue;
            }
            perror("sigtimedwait");
            return EXIT_FAILURE;
          }
          continue;
        }
        exit(status_child);
      }
      perror("waitpid");
      exit(EXIT_FAILURE);
    }
    if (pid_ret == pid_wait) {
      status_child = status;
      if (opt_wait != opt_reap) {
        if (opt_wait == wait_pid) {
          exit(status_child);
        }
        if (opt_wait == wait_all) {
          continue;
        }
      }
    }
  }
}