bin_PROGRAMS = tivodecode tdcat
lib_LIBRARIES = libtivodecode.a
pkginclude_HEADERS = tivo_parse.hxx turing_stream.hxx Turing.hxx
nodist_pkginclude_HEADERS = tdconfig.h
libtivodecode_a_SOURCES=hexlib.cxx md5.cxx sha1.cxx TuringFast.cxx happyfile.cxx cli_common.cxx tivo_parse.cxx turing_stream.cxx TuringBoxes.hxx hexlib.hxx md5.hxx sha1.hxx happyfile.hxx cli_common.hxx
tivodecode_SOURCES=tivodecode.cxx tivo_decoder_base.cxx tivo_decoder_ts.cxx tivo_decoder_ts_pkt.cxx tivo_decoder_ts_stream.cxx tivo_decoder_ps.cxx tivo_decoder_mpeg_parser.cxx getopt_long.h happyfile.hxx tivo_decoder_base.hxx tivo_decoder_ts.hxx tivo_decoder_ps.hxx tivo_decoder_mpeg_parser.hxx
tivodecode_LDADD=$(LIBOBJS) -L. -ltivodecode
tivodecode_DEPENDENCIES=$(LIBOBJS) libtivodecode.a
tdcat_SOURCES=tdcat.cxx getopt_long.h
tdcat_LDADD=$(LIBOBJS) -L. -ltivodecode
tdcat_DEPENDENCIES=$(LIBOBJS) libtivodecode.a
EXTRA_DIST=tdconfig.h.win32

clean-local:
	-rm -rf *.xml *.ts
