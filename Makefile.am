bin_PROGRAMS = tivodecode tdcat
lib_LIBRARIES = libtivodecode.a
pkginclude_HEADERS = tivo_parse.hxx turing_stream.h Turing.h
nodist_pkginclude_HEADERS = tdconfig.h
libtivodecode_a_SOURCES=hexlib.c TuringFast.c sha1.c md5.c tivo_parse.cxx turing_stream.c QUTsbox.h TuringMultab.h TuringSbox.h hexlib.h sha1.h md5.h
tivodecode_SOURCES=happyfile.c tivo_decode.cxx tivo_decoder_base.cxx tivo_decoder_ts.cxx tivo_decoder_ts_pkt.cxx tivo_decoder_ts_stream.cxx tivo_decoder_ps.cxx tivo_decoder_mpeg_parser.cxx happyfile.h getopt_long.h fseeko.h tivo_decoder_base.hxx tivo_decoder_ts.hxx tivo_decoder_ps.hxx tivo_decoder_mpeg_parser.hxx
tivodecode_LDADD=$(LIBOBJS) -L. -ltivodecode
tivodecode_DEPENDENCIES=$(LIBOBJS) libtivodecode.a
tdcat_SOURCES=tdcat.cxx happyfile.c happyfile.h getopt_long.h
tdcat_LDADD=$(LIBOBJS) -L. -ltivodecode
tdcat_DEPENDENCIES=$(LIBOBJS) libtivodecode.a
EXTRA_DIST=tdconfig.h.win32

clean-local:
	-rm -rf *.xml *.ts
