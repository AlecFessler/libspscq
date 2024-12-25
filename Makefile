CC = gcc
CFLAGS = -std=c11 -c
PREFIX = /usr/local
LIBDIR = $(PREFIX)/lib
INCLUDEDIR = $(PREFIX)/include

spsc_queue.o: spsc_queue.h
	$(CC) $(CFLAGS) -x c spsc_queue.h -o spsc_queue.o

libspsc_queue.a: spsc_queue.o
	ar rcs libspsc_queue.a spsc_queue.o

install: libspsc_queue.a spsc_queue.h spsc_queue.hpp
	install -d $(LIBDIR)
	install -d $(INCLUDEDIR)
	install -m 644 libspsc_queue.a $(LIBDIR)
	install -m 644 spsc_queue.h $(INCLUDEDIR)
	install -m 644 spsc_queue.hpp $(INCLUDEDIR)

clean:
	rm -f *.o *.a

.PHONY: clean install
