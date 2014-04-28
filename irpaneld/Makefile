PRG=irpaneld
DEPS=cli.o common.o serial.o
CFLAGS=-Wall -O2
LDFLAGS=

.PHONY: all clean debug

all: $(PRG)

clean:
	rm -f *.o $(PRG)

debug: CFLAGS+= -DDEBUG
debug: all

$(PRG): $(DEPS)
	gcc $(CFLAGS) $(LDFLAGS) -o $@ $(PRG).c $(DEPS)

%.o: %.c
	gcc $(CFLAGS) $(LDFLAGS) -c $<
