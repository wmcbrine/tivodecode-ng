AC_INIT(tivodecode-ng, 0.1)
AC_CONFIG_SRCDIR(tivodecode.cxx)
AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE
AC_CONFIG_HEADERS(tdconfig.h)
AC_PROG_CXX(g++)
AC_PROG_CPP
AC_AIX
AC_ISC_POSIX
AC_MINIX
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

#    PGAC_PROG_CC_CFLAGS_OPT([-Wdeclaration-after-statement])
#    PGAC_PROG_CC_CFLAGS_OPT([-Wendif-labels])
#    PGAC_PROG_CC_CFLAGS_OPT([-Werror-implicit-function-declaration])
  fi
#  PGAC_PROG_CC_CFLAGS_OPT([-fno-strict-aliasing])
fi

AC_SYS_LARGEFILE
AC_FUNC_FSEEKO
AC_TYPE_OFF_T
AC_CHECK_SIZEOF(off_t)
AC_CHECK_TYPES([size_t], [AC_CHECK_SIZEOF([size_t])])
AC_CHECK_TYPES([unsigned long], [AC_CHECK_SIZEOF([unsigned long])])
AC_CHECK_TYPES([unsigned long long], [AC_CHECK_SIZEOF([unsigned long long])])

AC_CHECK_HEADERS([getopt.h ctype.h], [], [], AC_INCLUDES_DEFAULT([]))
AC_SEARCH_LIBS(getopt_long, [getopt gnugetopt])

AC_CHECK_TYPES([struct option], [], [],
[#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif])

AC_CACHE_CHECK([for optreset], pgac_cv_var_int_optreset,
[AC_TRY_LINK([#include <unistd.h>],
  [extern int optreset; optreset = 1;],
  [pgac_cv_var_int_optreset=yes],
  [pgac_cv_var_int_optreset=no])])
if test x"$pgac_cv_var_int_optreset" = x"yes"; then
  AC_DEFINE(HAVE_INT_OPTRESET, 1, [Define to 1 if you have the global variable 'int optreset'.])
fi

if test x"$ac_cv_type_struct_option" = xyes ; then
  AC_REPLACE_FUNCS([getopt_long])
else
  AC_LIBOBJ(getopt_long)
fi

AC_OUTPUT(Makefile)

