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
 * Wait for SIGCHLD or timeout
 * @return 0 if success, -1 if error
 */
static int wait_sigchld(void) {
  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGCHLD);
  struct timespec timeout = {1, 0};
  int sig = sigtimedwait(&mask, NULL, &timeout);
  if (sig == -1) {
    if (errno == EAGAIN) {
      return 0;
    }
    perror("sigtimedwait");
    return -1;
  }
  return 0;
}

int main(int argc, char **argv) {
  pid_t pid_self = getpid();
  int opt;
  int opt_deathsig = SIGHUP;
  static struct option long_options[] = {
      {"deathsig", required_argument, NULL, 'd'},
      {"help", no_argument, NULL, 'h'},
      {NULL, 0, NULL, 0}};
  while ((opt = getopt_long(argc, argv, "+d:h", long_options, NULL)) != -1) {
    switch (opt) {

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

    case 'h':
      printf("Usage: %s [OPTION] [ENV=VAL ...] [COMMAND [ARG ...]]\n", argv[0]);
      printf("Execute COMMAND with ARGs\n");
      printf("Options:\n");
      printf(
          "  -d, --deathsig=<num>|<signame>  set death signal       [none]\n");
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

  if (pid_self != 1) {
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

  pid_t pid_child = -1;
  if (cmdind < argc) {
    pid_child = fork();
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
  } else if (pid_self != 1) {
    fprintf(stderr, "No command specified\n");
    fprintf(stderr,
            "Command needs for non-init process (init pid:1, this pid:%d)\n",
            pid_self);
    return EXIT_FAILURE;
  } else {
    // just run as reaper if no command and running as init
    int status;
    while (1) {
      pid_t pid_reported = wait(&status);
      if (pid_reported == -1) {
        if (errno == EINTR) {
          continue;
        }
        if (errno == ECHILD) {
          break;
        }
        perror("waitpid");
        return EXIT_FAILURE;
      }
      int ret = wait_sigchld();
      if (ret == -1) {
        return EXIT_FAILURE;
      }
    }
  }

  int status;
  int exit_status = -1;
  while (1) {
    pid_t pid_reported = 0;

    // check child process in the same process group first
    pid_reported = waitpid(0, &status, WNOHANG);

    if (pid_reported == 0) {
      // otherwise, check the whole child process
      pid_reported = wait(&status);
    }

    if (pid_reported == -1) {

      if (errno == EINTR) {
        continue;
      }

      if (errno == ECHILD) {

        if (pid_child > 0) {

          if (exit_status == -1) {
            fprintf(stderr, "Child process %d exited abnormally\n", pid_child);
            return EXIT_FAILURE;
          }

          return exit_status;
        }

        int ret = wait_sigchld();
        if (ret == -1) {
          return EXIT_FAILURE;
        }

        continue;
      }

      perror("waitpid");
      return EXIT_FAILURE;
    }

    if (pid_reported == pid_child) {
      exit_status = status;
    }
  }
}