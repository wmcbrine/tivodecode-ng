#-----------------------------------------------
# tivodecode-ng Makefile for GCC/GNU make et al.
# This file will be overwritten by configure.
#-----------------------------------------------

# General options. POST is for any post-processing that needs doing.

ifeq ($(DEBUG),Y)
	OPTS = -g -Wall -Wextra -pedantic
else
	OPTS = -O2 -Wall -pedantic -flto
	POST = strip $@
endif

# PREFIX is the base directory under which to install the binary and man
# page; generally either /usr/local or /usr (or perhaps /opt...).

PREFIX = /usr/local

#--------------------------------------------------------------
# RM is the Delete command ("rm" or "del", as appropriate), and SEP is the
# separator for multi-statement lines... some systems require ";", while
# others need "&&".

ifeq ($(OS),Windows_NT)
	RM = del
	SEP = &&
	E = .exe
	LFLAGS = -static
else
	RM = rm -f
	SEP = ;
	E =
	LFLAGS =
endif

O = o

.SUFFIXES: .cxx
.PHONY: clean dep install

all:	tivodecode$(E) tdcat$(E)

OBJS1 = TuringFast.$(O) cli_common.$(O) getopt_td.$(O) happyfile.$(O) \
hexlib.$(O) md5.$(O) sha1.$(O) turing_stream.$(O) tivo_parse.$(O)

OBJS2 = tivo_decoder_base.$(O) tivo_decoder_mpeg_parser.$(O) \
tivo_decoder_ps.$(O) tivo_decoder_ts.$(O) tivo_decoder_ts_pkt.$(O) \
tivo_decoder_ts_stream.$(O)

OBJS3 = tivodecode.$(O) tdcat.$(O)

$(OBJS1) $(OBJS2) $(OBJS3): %.o: %.cxx
	$(CXX) $(OPTS) -c $<

tivodecode$(E):	tivodecode.$(O) $(OBJS1) $(OBJS2)
	$(CXX) $(LFLAGS) -o tivodecode$(E) tivodecode.$(O) $(OBJS1) $(OBJS2)
	$(POST)

tdcat$(E):	tdcat.$(O) $(OBJS1)
	$(CXX) $(LFLAGS) -o tdcat$(E) tdcat.$(O) $(OBJS1)
	$(POST)

dep:
	$(CXX) -MM *.cxx | sed s/"\.o"/"\.\$$(O)"/ > depend

clean:
	$(RM) *.$(O)
	$(RM) tivodecode$(E)
	$(RM) tdcat$(E)

install::
	install -c -s tivodecode $(PREFIX)/bin
	install -c -s tdcat $(PREFIX)/bin

include depend
