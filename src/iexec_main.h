#pragma once

#include "iexec.h"
#include "iexec_option.h"

void iexec_mainloop(int argc, char **argv, iexec_option_t *ctx)
    __attribute__((noreturn));
void iexec_print_warning(iexec_option_t *ctx);
void iexec_main(int argc, char **argv, iexec_option_t *ctx);
