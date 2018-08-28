AC_INIT(tivodecode-ng, 0.1)
AC_CONFIG_SRCDIR(tivodecode.cxx)
AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE
AC_CONFIG_HEADERS(tdconfig.h)
AC_PROG_CXX(g++)
AC_PROG_CPP
AC_ISC_POSIX
AC_HEADER_STDC
AC_C_BIGENDIAN
AC_C_INLINE
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
AC_FUNC_FSEEKO
AC_TYPE_OFF_T
AC_CHECK_SIZEOF(off_t)
AC_CHECK_TYPES([size_t], [AC_CHECK_SIZEOF([size_t])])
AC_CHECK_TYPES([unsigned long], [AC_CHECK_SIZEOF([unsigned long])])
AC_CHECK_TYPES([unsigned long long], [AC_CHECK_SIZEOF([unsigned long long])])

AC_OUTPUT(Makefile)

