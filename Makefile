CFLAGS=-Wall -D_FILE_OFFSET_BITS=64 -O3

TURING=TuringFast.o

OBJS=hexlib.o $(TURING) sha1.o tivo-parse.o happyfile.o turing_stream.o

all: tivodecode

clean:
	rm -f *.o $(OBJS) tivodecode.o tivodecode

tivodecode: tivodecode.o turing_stream.o $(OBJS)
	$(CC) -o $@ $^
