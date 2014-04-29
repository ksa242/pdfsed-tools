CROSS_COMPILE ?=
CC ?= gcc
CFLAGS ?= -O2
CFLAGS_REQ = -std=c99 -D_POSIX_C_SOURCE=200101L -Wall `pkg-config --cflags --libs libxml-2.0` -lhpdf -lm

all: pdfsed-conv pdfsed-run

%.o: %.h %.c
	$(CROSS_COMPILE)$(CC) $(CFLAGS) $(CFLAGS_REQ) -c $(@:.o=.c)

.PHONY: pdfsed-conv
pdfsed-conv: common.o hocr.o djvused.o txt.o
	$(CROSS_COMPILE)$(CC) $(CFLAGS) $(CFLAGS_REQ) -o pdfsed-conv $^ pdfsed-conv.c

.PHONY: pdfsed-run
pdfsed-run: common.o hocr.o djvused.o pdfsed.o
	$(CROSS_COMPILE)$(CC) $(CFLAGS) $(CFLAGS_REQ) -o pdfsed-run $^ pdfsed-run.c

.PHONY: clean
clean:
	-rm *.o
