#include "iexec_print.h"
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static iexec_print_level_t verbose = IEXEC_PRINT_LEVEL_WARNING;

void iexec_printf(iexec_print_level_t level, const char *msg, ...) {
  if (verbose >= level) {
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
  }
}

void iexec_printf_increase_verbosity(void) {
  if (verbose < IEXEC_PRINT_LEVEL_DEBUG) {
    verbose++;
  }
}

void iexec_printf_decrease_verbosity(void) {
  if (verbose > IEXEC_PRINT_LEVEL_FATAL) {
    verbose--;
  }
}

int iexec_errno(void) { return errno; }
const char *iexec_strerror(int errnum) { return strerror(errnum); }