#pragma once

#include "iexec.h"
#include "iexec_option.h"

/**
 * @brief Main loop of iexec
 *
 * @param argc Argument count
 * @param argv Argument vector
 * @param ctx iexec_option_t context
 */
void iexec_mainloop(int argc, char **argv, iexec_option_t *ctx)
    __attribute__((noreturn));

/**
 * @brief Print warning message
 *
 * @param ctx iexec_option_t context
 */
void iexec_print_warning(iexec_option_t *ctx);

/**
 * @brief Main function of iexec
 *
 * @param argc Argument count
 * @param argv Argument vector
 * @param ctx iexec_option_t context
 */
void iexec_main(int argc, char **argv, iexec_option_t *ctx);
