srcdir=.
top_srcdir=..

apsdir=$(top_srcdir)/aps

CFLAGS+=-g -Wall -Wextra -Wmissing-prototypes -Wstrict-prototypes -Wmissing-declarations -Wshadow -I$(top_srcdir) `cups-config --cflags`
LDFLAGS+=-L$(apsdir) `cups-config --image --libs --ldflags` -lusb-1.0

TARGETS=sample1 sample2 sample3 sample4 sample5 qrsample qrs wakeup

all: $(TARGETS)

sample1: sample1.c $(apsdir)/libaps.a
	@echo "Building $@..."
	@$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LDFLAGS)

sample2: sample2.c $(apsdir)/libaps.a
	@echo "Building $@..."
	@$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LDFLAGS)

sample3: sample3.c $(apsdir)/libaps.a
	@echo "Building $@..."
	@$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LDFLAGS)

sample4: sample4.c $(apsdir)/libaps.a
	@echo "Building $@..."
	@$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LDFLAGS)

sample5: sample5.c $(apsdir)/libaps.a
	@echo "Building $@..."
	@$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LDFLAGS)

wakeup: wakeup.c $(apsdir)/libaps.a
	@echo "Building $@..."
	@$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LDFLAGS)

qrsample: qrsample.c $(apsdir)/libaps.a
	@echo "Building $@..."
	@$(CC) $(CFLAGS) $(LDFLAGS) $^ -lqrencode -o $@ -g $(LDFLAGS)

qrs: qrs.c $(apsdir)/libaps.a
	@echo "Building $@..."
	@$(CC) $(CFLAGS) $(LDFLAGS) $^ -lqrencode -o $@ -g $(LDFLAGS)

clean:
	$(RM) *.o $(TARGETS)

install:

uninstall:

