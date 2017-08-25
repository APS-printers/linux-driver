srcdir=.
top_srcdir=..

prefix=/usr
includedir=$(prefix)/include
libdir=$(prefix)/lib

INSTALL=/usr/bin/install

CFLAGS+=-g -Wall -Wextra -Wmissing-prototypes -Wstrict-prototypes -Wmissing-declarations -Wshadow -I$(top_srcdir)
LDFLAGS+=-L$(srcdir) -lusb-1.0

TARGETS=libaps.a getstatus testaps testdetect

all: $(TARGETS)

libaps.a: aps.o uri.o detect.o serial.o parallel.o usb.o models.o ethernet.o
	@echo "Building Libaps..."
	@$(AR) r $@ $^

%.o: %.c aps.h aps-private.h
	@echo "[CC] $<..."
	@$(CC) $(CFLAGS) -c -o $@ $<

getstatus: getstatus.c libaps.a
	@echo "Building $@..."
	@$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

testaps: testaps.c libaps.a
	@echo "Building $@..."
	@$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LDFLAGS)

testdetect: testdetect.c libaps.a
	@echo "Building $@..."
	@$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	$(RM) *.o *~ $(TARGETS)

install:
	mkdir -p $(DESTDIR)$(libdir)
	$(INSTALL) libaps.a $(DESTDIR)$(libdir)
	$(INSTALL) -d $(DESTDIR)$(includedir)/aps
	$(INSTALL) -m 644 models.def version.def aps.h version.h $(DESTDIR)$(includedir)/aps

uninstall:
	$(RM) $(DESTDIR)$(libdir)/libaps.a
	$(RM) -r $(DESTDIR)$(includedir)/aps

