#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <unistd.h>

enum {
  wait_never = 0,
  wait_invoked = 1,
  wait_invokedgroup = 2,
  wait_all = 3,
  wait_forever = 4,
};

static struct sigspec {
  const char *name;
  int signo;
} sigspecs[] = {
    {"HUP", SIGHUP},   {"INT", SIGINT},       {"QUIT", SIGQUIT},
    {"ILL", SIGILL},   {"TRAP", SIGTRAP},     {"ABRT", SIGABRT},
    {"IOT", SIGIOT},   {"BUS", SIGBUS},       {"FPE", SIGFPE},
    {"KILL", SIGKILL}, {"USR1", SIGUSR1},     {"SEGV", SIGSEGV},
    {"USR2", SIGUSR2}, {"PIPE", SIGPIPE},     {"ALRM", SIGALRM},
    {"TERM", SIGTERM}, {"STKFLT", SIGSTKFLT}, {"CHLD", SIGCHLD},
    {"CONT", SIGCONT}, {"STOP", SIGSTOP},     {"TSTP", SIGTSTP},
    {"TTIN", SIGTTIN}, {"TTOU", SIGTTOU},     {"URG", SIGURG},
    {"XCPU", SIGXCPU}, {"XFSZ", SIGXFSZ},     {"VTALRM", SIGVTALRM},
    {"PROF", SIGPROF}, {"WINCH", SIGWINCH},   {"IO", SIGIO},
    {"PWR", SIGPWR},   {"SYS", SIGSYS},       {NULL, 0},
};

int parse_sig(const char *sigspec) {
  if (sigspec == NULL || *sigspec == '\0') {
    return -1;
  }
  char *p;
  long signo = strtol(sigspec, &p, 10);
  if (sigspec != p && *p == '\0') {
    for (int i = 0; sigspecs[i].name != NULL; i++) {
      if (sigspecs[i].signo == signo) {
        return signo;
      }
    }
    return -1;
  }
  if (strncmp(sigspec, "SIG", 3) == 0) {
    sigspec += 3;
  }
  for (int i = 0; sigspecs[i].name != NULL; i++) {
    if (strcmp(sigspec, sigspecs[i].name) == 0) {
      return sigspecs[i].signo;
    }
  }
  return -1;
}

int main(int argc, char **argv) {
  int opt;
  int opt_subreaper = 1;
  int opt_deathsig = 0;
  int opt_wait = wait_invoked;
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
      if (strcmp(optarg, "true") == 0) {
        opt_subreaper = 1;
      } else if (strcmp(optarg, "false") == 0) {
        opt_subreaper = 0;
      } else {
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
      if (strcmp(optarg, "never") == 0) {
        opt_wait = wait_never;
      } else if (strcmp(optarg, "invoked") == 0) {
        opt_wait = wait_invoked;
      } else if (strcmp(optarg, "invokedgroup") == 0) {
        opt_wait = wait_invokedgroup;
      } else if (strcmp(optarg, "all") == 0) {
        opt_wait = wait_all;
      } else if (strcmp(optarg, "forever") == 0) {
        opt_wait = wait_forever;
      } else {
        fprintf(stderr, "Invalid argument: %s\n", optarg);
        return EXIT_FAILURE;
      }
      break;

    case 'h':
      printf("Usage: %s [OPTION] [ENV=VAL ...] [COMMAND [ARG ...]]\n", argv[0]);
      printf("Execute COMMAND with ARGs\n");
      printf("Options:\n");
      printf("  -s, --subreaper={true,false}       set as subreaper  [true]\n");
      printf("  -d, --deathsig={<num>|<signame>}   set death signal  [none]\n");
      printf("  -w, --wait={never|invoked|invokedgroup|all|foreaver}");
      printf(
          "                                     wait for children [invoked]\n");
      printf("  -h, --help                         display this help and "
             "exit\n");
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
    case wait_invoked:
      wpid = pid;
      break;
    case wait_invokedgroup:
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
            break;
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
    case wait_invoked:
      return EXIT_SUCCESS;
    case wait_invokedgroup:
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