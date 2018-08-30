AC_INIT(tivodecode-ng, 0.2)
AC_CONFIG_SRCDIR(tivodecode.cxx)
AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE
AC_CONFIG_HEADERS(tdconfig.h)
AC_PROG_CXX(g++)
AC_PROG_CPP
AC_C_BIGENDIAN
AC_PROG_RANLIB
AC_PROG_INSTALL

if test x"$ac_cv_c_compiler_gnu" = x"yes"; then
# ICC pretends to be GCC but it's lying; it doesn't support these options.
# So we have to check if "GCC" is really ICC.
AC_TRY_COMPILE([], [@%:@ifndef __INTEL_COMPILER
choke me
@%:@endif], [ICC=[yes]], [ICC=[no]])

  if test "$ICC" = no; then
    CFLAGS="$CFLAGS -Wall -Wpointer-arith -Winline"
  fi
fi

AC_SYS_LARGEFILE
AC_TYPE_OFF_T
AC_CHECK_SIZEOF(off_t)

AC_OUTPUT(Makefile)

