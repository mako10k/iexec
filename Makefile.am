SUBDIRS = src

TESTS = tests/iexec-behavior.sh
TESTS += tests/docker-entrypoint.sh
EXTRA_DIST = $(TESTS)

AM_TESTS_ENVIRONMENT = IEXEC_TEST_BINARY='$(abs_top_builddir)/src/iexec';
