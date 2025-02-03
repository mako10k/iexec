#pragma once

#include "iexec.h"

typedef enum ielevel {
  IEXEC_PRINT_LEVEL_FATAL,
  IEXEC_PRINT_LEVEL_ERROR,
  IEXEC_PRINT_LEVEL_WARNING,
  IEXEC_PRINT_LEVEL_INFORMATION,
  IEXEC_PRINT_LEVEL_DEBUG
} iexec_print_level_t;

/**
 * Print message
 * @param level print level
 * @param msg message
 * @param ... arguments
 */
void iexec_printf(iexec_print_level_t level, const char *msg, ...)
    __attribute((format(printf, 2, 3)));

void iexec_printf_increase_verbosity(void);
void iexec_printf_decrease_verbosity(void);

int iexec_errno(void);
const char *iexec_strerror(int errnum);