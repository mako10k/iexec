#include "iexec_main.h"
#include "iexec_option.h"
#include "iexec_privilege.h"

int main(int argc, char **argv) {
  iexec_option_t ctx;
  iexec_drop_privilege();
  iexec_option_init(&ctx);
  iexec_option_parse(argc, argv, &ctx);
  iexec_print_warning(&ctx);
  iexec_main(argc, argv, &ctx);
}