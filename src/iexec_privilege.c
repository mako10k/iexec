#include "iexec_privilege.h"
#include "iexec_print.h"
#include "iexec_process.h"
#include <errno.h>
#include <linux/capability.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

static void iexec_drop_capabilities(void) {
  struct __user_cap_header_struct header;
  struct __user_cap_data_struct data[_LINUX_CAPABILITY_U32S_3];

  memset(&header, 0, sizeof(header));
  memset(data, 0, sizeof(data));
  header.version = _LINUX_CAPABILITY_VERSION_3;

  if (syscall(SYS_capset, &header, data) == -1) {
    iexec_printf(IEXEC_PRINT_LEVEL_FATAL, "capset: %s\n",
                 iexec_strerror(iexec_errno()));
    iexec_exit(IEXEC_EXIT_FAILURE);
  }
}

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
    iexec_drop_capabilities();
  }
}

void iexec_raise_privilege(void) {
  if (geteuid() == 0) {
    return;
  }
  int ret = seteuid(0);
  if (ret == -1) {
    if (iexec_errno() == EPERM) {
      return;
    }
    iexec_printf(IEXEC_PRINT_LEVEL_ERROR, "Cannot promote privilege: %s\n",
                 iexec_strerror(iexec_errno()));
    iexec_exit(IEXEC_EXIT_FAILURE);
  }
}
