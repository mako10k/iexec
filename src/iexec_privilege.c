#include "iexec_privilege.h"
#include "iexec_print.h"
#include "iexec_process.h"
#include <unistd.h>

void iexec_drop_privilege(void) {
  uid_t uid = getuid();
  if (uid != 0) {
    int ret = seteuid(uid);
    if (ret == -1) {
      iexec_printf(IEXEC_PRINT_LEVEL_FATAL, "seteuid: %s\n",
                   iexec_strerror(iexec_errno()));
      iexec_exit(IEXEC_EXIT_FAILURE);
    }
  }
}

void iexec_drop_privilege_permanently(void) {
  uid_t uid = getuid();
  if (uid != 0) {
    int ret = setuid(uid);
    if (ret == -1) {
      iexec_printf(IEXEC_PRINT_LEVEL_FATAL, "setuid: %s\n",
                   iexec_strerror(iexec_errno()));
      iexec_exit(IEXEC_EXIT_FAILURE);
    }
  }
}

void iexec_raise_privilege(void) {
  int ret = seteuid(0);
  if (ret == -1) {
    iexec_printf(IEXEC_PRINT_LEVEL_ERROR, "Cannot promote privilege: %s\n",
                 iexec_strerror(iexec_errno()));
    iexec_exit(IEXEC_EXIT_FAILURE);
  }
}
