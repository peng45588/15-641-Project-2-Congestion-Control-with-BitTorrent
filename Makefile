# Some variables
CC 		= gcc
CFLAGS		= -g -Wall -DDEBUG
LDFLAGS		= -lm
TESTDEFS	= -DTESTING			# comment this out to disable debugging code
OBJS		= bt_parse.o spiffy.o debug.o input_buffer.o chunk.o sha.o packet.o connection.o job.o peer.o lib.o control.o
MK_CHUNK_OBJS   = make_chunks.o chunk.o sha.o

BINS            = peer make-chunks
TESTBINS        = test_debug test_input_buffer

# Implicit .o target
.c.o:
	$(CC) $(TESTDEFS) -c $(CFLAGS) $<

# Explit build and testing targets

all: ${BINS} ${TESTBINS}
#gcc -g -Wall -DDEBUG peer.c bt_parse.c spiffy.c debug.c input_buffer.c chunk.c sha.c job.c packet.c connection.c -o peer
#${BINS} ${TESTBINS}

run: peer_run
	./peer_run

test: peer_test
	./peer_test

peer: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

make-chunks: $(MK_CHUNK_OBJS)
	$(CC) $(CFLAGS) $(MK_CHUNK_OBJS) -o $@ $(LDFLAGS)

clean:
	rm -f *.o $(BINS) $(TESTBINS)

bt_parse.c: bt_parse.h

# The debugging utility code

debug-text.h: debug.h
	./debugparse.pl < debug.h > debug-text.h

test_debug.o: debug.c debug-text.h
	${CC} debug.c ${INCLUDES} ${CFLAGS} -c -D_TEST_DEBUG_ -o $@

test_input_buffer:  test_input_buffer.o input_buffer.o

runA: 
	echo "GET ../tmp/B.chunks newB.tar" | ./peer -p ../tmp/nodes.map -c ../tmp/A.haschunks -f ../tmp/C.masterchunks -m 4 -i 3 -d 10
runB: 
	./peer -p ../tmp/nodes.map -c ../tmp/B.haschunks -f ../tmp/C.masterchunks -m 4 -i 5 -d 10
runC: 
	./peer -p ../tmp/nodes.map -c ../tmp/B.haschunks -f ../tmp/C.masterchunks -m 4 -i 6 -d 10

handin:
	tar cf ../pengchex_project_2.tar ../15-441-project-2