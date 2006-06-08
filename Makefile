CFLAGS=-Wall -D_FILE_OFFSET_BITS=64 -O3

OBJDIR=objects.dir
OBJS=hexlib.o TuringFast.o sha1.o tivo-parse.o happyfile.o turing_stream.o tivodecode.o

REALOBJS=$(patsubst %.o,$(OBJDIR)/%.o,$(OBJS))

all: tivodecode

.PHONY: clean
clean:
	rm -rf $(OBJDIR)

.PHONY: prep
prep:
	mkdir -p $(OBJDIR)

.PHONY: tivodecode
tivodecode: prep $(OBJDIR)/tivodecode

$(OBJDIR)/tivodecode: $(REALOBJS)
	$(CC) -o $@ $^

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJDIR):
	mkdir $(OBJDIR)
