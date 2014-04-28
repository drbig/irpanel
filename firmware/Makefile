PRG=firmware
DEPS=lcd.o rc5.o uartcli.o
MCU=attiny2313
CLOCK=8000000UL
CFLAGS=-Wall -O2 -mmcu=$(MCU) -DF_CPU=$(CLOCK)
LDFLAGS=
FUSES=-U lfuse:w:0xe4:m -U hfuse:w:0xdf:m -U efuse:w:0xff:m
ADFLAGS=-F -p t2313 -P /dev/ttyUSB0 -c avr910 -b 115200

all: $(PRG).hex
	avr-size -C --mcu=$(MCU) $(PRG).elf

burn: $(PRG).hex
	avrdude $(ADFLAGS) -U flash:w:$< $(FUSES)

clean:
	rm -f *.o *.elf *.hex *.lst

lst: $(PRG).lst

$(PRG).elf: $(DEPS)
	avr-gcc $(CFLAGS) $(LDFLAGS) -o $@ $(PRG).c $(DEPS)

%.o: %.c
	avr-gcc $(CFLAGS) $(LDFLAGS) -c $<

%.lst: %.elf
	avr-objdump -h -S $< > $@

%.hex: %.elf
	avr-objcopy -j .text -j .data -O ihex $< $@
