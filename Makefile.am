bin_PROGRAMS = tivodecode tdcat
lib_LIBRARIES = libtivodecode.a
pkginclude_HEADERS = tivodecoder.h tivo-parse.h turing_stream.h Turing.h
nodist_pkginclude_HEADERS = tdconfig.h
libtivodecode_a_SOURCES=hexlib.c TuringFast.c sha1.c md5.c tivo-parse.c turing_stream.c tivodecoder.c QUTsbox.h TuringMultab.h TuringSbox.h hexlib.h sha1.h md5.h
tivodecode_SOURCES=cli_common.c happyfile.c tivodecode.c happyfile.h getopt_long.h fseeko.h cli_common.h
tivodecode_LDADD=$(LIBOBJS) -L. -ltivodecode
tivodecode_DEPENDENCIES=$(LIBOBJS) libtivodecode.a
tdcat_SOURCES=cli_common.c happyfile.c tdcat.c happyfile.h getopt_long.h fseeko.h cli_common.h
tdcat_LDADD=$(LIBOBJS) -L. -ltivodecode
tdcat_DEPENDENCIES=$(LIBOBJS) libtivodecode.a
EXTRA_DIST=libtivodecode.vcproj tdconfig.h.win32 tivodecode.rules tivodecode.sln tivodecode.vcproj

