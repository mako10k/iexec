#include "iexec_privilege.h"
#include "iexec_print.h"
#include "iexec_process.h"
#include <errno.h>
#include <linux/capability.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

static gid_t *iexec_initial_groups = NULL;
static int iexec_initial_group_count = -1;

static int iexec_compare_gid(const void *lhs, const void *rhs) {
  gid_t a = *(const gid_t *)lhs;
  gid_t b = *(const gid_t *)rhs;
  return (a > b) - (a < b);
}

static void iexec_get_capabilities(struct __user_cap_data_struct *data) {
  struct __user_cap_header_struct header;

  memset(&header, 0, sizeof(header));
  memset(data, 0, sizeof(struct __user_cap_data_struct) *
                      _LINUX_CAPABILITY_U32S_3);
  header.version = _LINUX_CAPABILITY_VERSION_3;

  if (syscall(SYS_capget, &header, data) == -1) {
    iexec_printf(IEXEC_PRINT_LEVEL_FATAL, "capget: %s\n",
                 iexec_strerror(iexec_errno()));
    iexec_exit(IEXEC_EXIT_FAILURE);
  }
}

static int iexec_has_cap_sys_admin(void) {
  struct __user_cap_data_struct data[_LINUX_CAPABILITY_U32S_3];
  int index = CAP_SYS_ADMIN / 32;
  unsigned int mask = 1U << (CAP_SYS_ADMIN % 32);

  iexec_get_capabilities(data);
  return (data[index].effective & mask) != 0;
}

static int iexec_has_any_capability(void) {
  struct __user_cap_data_struct data[_LINUX_CAPABILITY_U32S_3];

  iexec_get_capabilities(data);
  for (int i = 0; i < _LINUX_CAPABILITY_U32S_3; i++) {
    if (data[i].effective != 0 || data[i].permitted != 0 ||
        data[i].inheritable != 0) {
      return 1;
    }
  }
  return 0;
}

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

void iexec_privilege_snapshot(void) {
  iexec_initial_group_count = getgroups(0, NULL);
  if (iexec_initial_group_count == -1) {
    iexec_printf(IEXEC_PRINT_LEVEL_FATAL, "getgroups: %s\n",
                 iexec_strerror(iexec_errno()));
    iexec_exit(IEXEC_EXIT_FAILURE);
  }
  if (iexec_initial_group_count == 0) {
    return;
  }

  iexec_initial_groups =
      malloc(sizeof(gid_t) * (size_t)iexec_initial_group_count);
  if (iexec_initial_groups == NULL) {
    iexec_printf(IEXEC_PRINT_LEVEL_FATAL, "malloc: %s\n",
                 iexec_strerror(iexec_errno()));
    iexec_exit(IEXEC_EXIT_FAILURE);
  }
  if (getgroups(iexec_initial_group_count, iexec_initial_groups) == -1) {
    iexec_printf(IEXEC_PRINT_LEVEL_FATAL, "getgroups: %s\n",
                 iexec_strerror(iexec_errno()));
    iexec_exit(IEXEC_EXIT_FAILURE);
  }
  qsort(iexec_initial_groups, (size_t)iexec_initial_group_count,
        sizeof(gid_t), iexec_compare_gid);
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

void iexec_prepare_pidns_privilege(void) {
  if (geteuid() == 0) {
    return;
  }
  if (iexec_has_cap_sys_admin()) {
    return;
  }
#ifdef IEXEC_INSTALL_PRIVILEGE_SETUID
  if (seteuid(0) == -1) {
    iexec_printf(IEXEC_PRINT_LEVEL_ERROR, "seteuid(0): %s\n",
                 iexec_strerror(iexec_errno()));
    iexec_exit(IEXEC_EXIT_FAILURE);
  }
  if (geteuid() != 0) {
    iexec_printf(IEXEC_PRINT_LEVEL_ERROR,
                 "seteuid(0) did not produce effective uid 0\n");
    iexec_exit(IEXEC_EXIT_FAILURE);
  }
  return;
#else
  iexec_printf(IEXEC_PRINT_LEVEL_ERROR,
               "PID namespace setup requires root or effective CAP_SYS_ADMIN\n");
#ifdef IEXEC_INSTALL_PRIVILEGE_CAP
  iexec_printf(IEXEC_PRINT_LEVEL_ERROR,
               "capability install is missing effective CAP_SYS_ADMIN\n");
#endif
  iexec_exit(IEXEC_EXIT_FAILURE);
#endif
}

static void iexec_assert_uids_are_stable(void) {
  uid_t ruid;
  uid_t euid;
  uid_t suid;

  if (getresuid(&ruid, &euid, &suid) == -1) {
    iexec_printf(IEXEC_PRINT_LEVEL_FATAL, "getresuid: %s\n",
                 iexec_strerror(iexec_errno()));
    iexec_exit(IEXEC_EXIT_FAILURE);
  }
  if (ruid != euid || ruid != suid) {
    iexec_printf(IEXEC_PRINT_LEVEL_FATAL,
                 "refusing to exec with mismatched uid state "
                 "(ruid:%ld euid:%ld suid:%ld)\n",
                 (long)ruid, (long)euid, (long)suid);
    iexec_exit(IEXEC_EXIT_FAILURE);
  }
}

static void iexec_assert_gids_are_stable(void) {
  gid_t rgid;
  gid_t egid;
  gid_t sgid;

  if (getresgid(&rgid, &egid, &sgid) == -1) {
    iexec_printf(IEXEC_PRINT_LEVEL_FATAL, "getresgid: %s\n",
                 iexec_strerror(iexec_errno()));
    iexec_exit(IEXEC_EXIT_FAILURE);
  }
  if (rgid != egid || rgid != sgid) {
    iexec_printf(IEXEC_PRINT_LEVEL_FATAL,
                 "refusing to exec with mismatched gid state "
                 "(rgid:%ld egid:%ld sgid:%ld)\n",
                 (long)rgid, (long)egid, (long)sgid);
    iexec_exit(IEXEC_EXIT_FAILURE);
  }
}

static void iexec_assert_groups_unchanged(void) {
  int count = getgroups(0, NULL);
  if (count == -1) {
    iexec_printf(IEXEC_PRINT_LEVEL_FATAL, "getgroups: %s\n",
                 iexec_strerror(iexec_errno()));
    iexec_exit(IEXEC_EXIT_FAILURE);
  }
  if (count != iexec_initial_group_count) {
    iexec_printf(IEXEC_PRINT_LEVEL_FATAL,
                 "refusing to exec with changed supplementary groups\n");
    iexec_exit(IEXEC_EXIT_FAILURE);
  }
  if (count == 0) {
    return;
  }

  gid_t *groups = malloc(sizeof(gid_t) * (size_t)count);
  if (groups == NULL) {
    iexec_printf(IEXEC_PRINT_LEVEL_FATAL, "malloc: %s\n",
                 iexec_strerror(iexec_errno()));
    iexec_exit(IEXEC_EXIT_FAILURE);
  }
  if (getgroups(count, groups) == -1) {
    iexec_printf(IEXEC_PRINT_LEVEL_FATAL, "getgroups: %s\n",
                 iexec_strerror(iexec_errno()));
    free(groups);
    iexec_exit(IEXEC_EXIT_FAILURE);
  }
  qsort(groups, (size_t)count, sizeof(gid_t), iexec_compare_gid);
  if (memcmp(groups, iexec_initial_groups, sizeof(gid_t) * (size_t)count) !=
      0) {
    iexec_printf(IEXEC_PRINT_LEVEL_FATAL,
                 "refusing to exec with changed supplementary groups\n");
    free(groups);
    iexec_exit(IEXEC_EXIT_FAILURE);
  }
  free(groups);
}

void iexec_assert_exec_privilege_contract(void) {
  iexec_assert_uids_are_stable();
  iexec_assert_gids_are_stable();
  iexec_assert_groups_unchanged();

  if (getuid() != 0 && iexec_has_any_capability()) {
    iexec_printf(IEXEC_PRINT_LEVEL_FATAL,
                 "refusing to exec with capabilities still present\n");
    iexec_exit(IEXEC_EXIT_FAILURE);
  }
}
