#!/bin/sh

set -u

source_root=$(CDPATH= cd "$(dirname "$0")/.." && pwd) || exit 99
MAKE=${MAKE:-make}

fail() {
  echo "FAIL: $*" >&2
  exit 1
}

tmpdir=$(mktemp -d "${TMPDIR:-/tmp}/iexec-install-policy.XXXXXX") || exit 99

cleanup() {
  rm -rf "$tmpdir"
}

trap cleanup EXIT HUP INT TERM

srcdir=$tmpdir/source
mkdir "$srcdir" || exit 99
(
  cd "$source_root" &&
    tar \
      --exclude=.git \
      --exclude=autom4te.cache \
      --exclude=config.h \
      --exclude=config.log \
      --exclude=config.status \
      --exclude=Makefile \
      --exclude=stamp-h1 \
      --exclude=src/.deps \
      --exclude=src/Makefile \
      --exclude=src/iexec \
      --exclude='*.o' \
      --exclude='iexec-*.tar.gz' \
      -cf - .
) | (cd "$srcdir" && tar -xf -) || fail "could not copy source tree"
(cd "$srcdir" && autoreconf -fi) || fail "autoreconf failed"

run_install() {
  name=$1
  shift
  builddir=$tmpdir/$name-build
  prefix=$tmpdir/$name-prefix
  mkdir "$builddir" "$prefix" || exit 99

  (
    cd "$builddir" &&
      "$srcdir/configure" --prefix="$prefix" "$@" >/dev/null &&
      "$MAKE" >/dev/null &&
      "$MAKE" install >/dev/null
  ) || fail "$name install failed"

  installed_bin=$prefix/bin/iexec
  if [ ! -x "$installed_bin" ]; then
    fail "$name install did not create an executable iexec"
  fi
}

file_caps() {
  if command -v getcap >/dev/null 2>&1; then
    getcap "$1"
  fi
}

assert_no_file_caps() {
  caps=$(file_caps "$1")
  if [ -n "$caps" ]; then
    fail "expected no file capabilities on $1, got: $caps"
  fi
}

run_install default
if [ -u "$installed_bin" ]; then
  fail "default install unexpectedly set the setuid bit"
fi
assert_no_file_caps "$installed_bin"

if (cd "$tmpdir" &&
    "$srcdir/configure" --enable-cap-install --enable-setuid-install \
      >/dev/null 2>&1); then
  fail "configure accepted both --enable-cap-install and --enable-setuid-install"
fi

if [ "$(id -u)" -ne 0 ]; then
  echo "SKIP: capability and setuid install checks require root"
  exit 77
fi

if ! command -v setcap >/dev/null 2>&1 ||
   ! command -v getcap >/dev/null 2>&1; then
  echo "SKIP: setcap/getcap are required for capability install checks"
  exit 77
fi

run_install capability --enable-cap-install
if [ -u "$installed_bin" ]; then
  fail "capability install unexpectedly set the setuid bit"
fi
caps=$(file_caps "$installed_bin")
case "$caps" in
  *cap_sys_admin*=*ep*) ;;
  *) fail "capability install did not set cap_sys_admin+ep: $caps" ;;
esac

run_install setuid --enable-setuid-install
if [ ! -u "$installed_bin" ]; then
  fail "setuid fallback install did not set the setuid bit"
fi
assert_no_file_caps "$installed_bin"
