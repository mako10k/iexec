#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.71])
AC_INIT([iexec], [0.0.1], [mako10k@mk10.org])
AC_CONFIG_SRCDIR([src/iexec.c])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([foreign])
AC_USE_SYSTEM_EXTENSIONS

AC_ARG_ENABLE([cap-install],
  [AS_HELP_STRING([--enable-cap-install],
    [install iexec with cap_sys_admin for non-Docker PID namespace validation])],
  [],
  [enable_cap_install=no])
AS_CASE([$enable_cap_install],
  [yes], [],
  [no], [],
  [AC_MSG_ERROR([--enable-cap-install must be yes or no])])

AC_ARG_ENABLE([setuid-install],
  [AS_HELP_STRING([--enable-setuid-install],
    [install iexec with the setuid bit as an explicit fallback for PID namespace validation])],
  [],
  [enable_setuid_install=no])
AS_CASE([$enable_setuid_install],
  [yes], [],
  [no], [],
  [AC_MSG_ERROR([--enable-setuid-install must be yes or no])])

AS_IF([test "x$enable_cap_install" = "xyes" &&
       test "x$enable_setuid_install" = "xyes"],
  [AC_MSG_ERROR([--enable-cap-install and --enable-setuid-install are mutually exclusive])])

AC_PATH_PROG([SETCAP], [setcap], [], [$PATH:/usr/sbin:/sbin])
AS_IF([test "x$enable_cap_install" = "xyes" && test "x$SETCAP" = "x"],
  [AC_MSG_ERROR([setcap is required for --enable-cap-install])])

AM_CONDITIONAL([ENABLE_CAP_INSTALL],
  [test "x$enable_cap_install" = "xyes"])
AM_CONDITIONAL([ENABLE_SETUID_INSTALL],
  [test "x$enable_setuid_install" = "xyes"])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_PID_T

# Checks for library functions.
AC_FUNC_FORK
AC_CHECK_FUNCS([putenv strchr strtol])

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
