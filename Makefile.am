SUBDIRS = src

TESTS = tests/iexec-behavior.sh
TESTS += tests/docker-entrypoint.sh
TESTS += tests/pidns-validation.sh
EXTRA_DIST = $(TESTS)
EXTRA_DIST += LICENSE
EXTRA_DIST += docs/backlog.md
EXTRA_DIST += docs/ci.md
EXTRA_DIST += docs/docker.md
EXTRA_DIST += docs/install.md
EXTRA_DIST += docs/pidns-validation.md
EXTRA_DIST += docs/privilege.md
EXTRA_DIST += docs/release.md

AM_TESTS_ENVIRONMENT = IEXEC_TEST_BINARY='$(abs_top_builddir)/src/iexec';
