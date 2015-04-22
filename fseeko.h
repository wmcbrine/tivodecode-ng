#ifndef FSEEKO_H_
# define FSEEKO_H_
#ifdef __cplusplus
extern "C" {
#endif
# if !defined(HAVE_FSEEKO) && (defined(__bsdi__) || defined(__NetBSD__) || defined(WIN32))

#  ifdef WIN32
#   define OFF_T_TYPE __int64
#  else
#   define OFF_T_TYPE off_t
#  endif

extern int fseeko(FILE *stream, OFF_T_TYPE offset, int whence);

#  define HAVE_FSEEKO 1

# endif

#ifdef __cplusplus
}
#endif
#endif // FSEEKO_H_
