PREFIX = /usr/local
LIBDIR = $(PREFIX)/lib
INCLUDEDIR = $(PREFIX)/include

install:
	install -d $(INCLUDEDIR)
	install -m 644 spsc_queue.h $(INCLUDEDIR)
	install -m 644 spsc_queue.hpp $(INCLUDEDIR)

clean:
	rm -f *.o *.a

.PHONY: clean install
