PRG=readkeys
DEPS=common.o
CFLAGS=-Wall -O2 -DDEBUG
LDFLAGS=

all: $(PRG)

clean:
	rm -f *.o $(PRG)

$(PRG): $(DEPS)
	gcc $(CFLAGS) $(LDFLAGS) -o $@ $(PRG).c $(DEPS)

%.o: %.c
	gcc $(CFLAGS) $(LDFLAGS) -c $<
