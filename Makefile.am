bin_PROGRAMS = tivodecode 
lib_LIBRARIES = libtivodecode.a
pkginclude_HEADERS = tivo_parse.hxx turing_stream.h Turing.h
nodist_pkginclude_HEADERS = tdconfig.h
libtivodecode_a_SOURCES=hexlib.c TuringFast.c sha1.c md5.c tivo_parse.cxx turing_stream.c QUTsbox.h TuringMultab.h TuringSbox.h hexlib.h sha1.h md5.h
tivodecode_SOURCES=happyfile.c tivo_decode.cxx tivo_decoder_base.cxx tivo_decoder_ts.cxx tivo_decoder_ts_pkt.cxx tivo_decoder_ts_stream.cxx tivo_decoder_ps.cxx happyfile.h getopt_long.h fseeko.h tivo_decoder_base.hxx tivo_decoder_ts.hxx tivo_decoder_ps.hxx
tivodecode_LDADD=$(LIBOBJS) -L. -ltivodecode
tivodecode_DEPENDENCIES=$(LIBOBJS) libtivodecode.a
EXTRA_DIST=libtivodecode.vcproj libtdcommon.vcproj tdcat.vcproj tdconfig.h.win32 tivodecode.rules tivodecode.sln tivodecode.vcproj


clean-local:
	-rm -rf *.xml *.ts
