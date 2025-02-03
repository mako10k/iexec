bin_PROGRAMS = iexec

iexec_SOURCES = iexec.c
iexec_SOURCES += iexec_print.c
iexec_SOURCES += iexec_option.c
iexec_SOURCES += iexec_privilege.c
iexec_SOURCES += iexec_pidns.c
iexec_SOURCES += iexec_process.c
iexec_SOURCES += iexec_wait.c
iexec_SOURCES += iexec_main.c

noinst_HEADERS = iexec.h
noinst_HEADERS += iexec_print.h
noinst_HEADERS += iexec_option.h
noinst_HEADERS += iexec_privilege.h
noinst_HEADERS += iexec_pidns.h
noinst_HEADERS += iexec_process.h
noinst_HEADERS += iexec_wait.h
noinst_HEADERS += iexec_main.h

AM_LDFLAGS = -static -flto
AM_CFLAGS = -Wall -Wextra -Werror -Wpedantic -std=c99 -flto

install-exec-hook:
	chmod u+s "$(DESTDIR)$(bindir)/iexec"
