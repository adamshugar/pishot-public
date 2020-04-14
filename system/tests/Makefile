# Change TEST target as necessary
TEST = test_strings_printf.bin

INCLUDE = ../include
LIBSYS = ../libsys.a # Library for bare-metal interfacing with Raspberry Pi
GPIOEXTRA = ../libpi.a
BUILD = ../../build/

CFLAGS = -I $(INCLUDE) -g -Wall -Og -std=c99 -ffreestanding 
CFLAGS += -mapcs-frame -fno-omit-frame-pointer -mpoke-function-name -Wpointer-arith
LDFLAGS = -nostdlib -T $(BUILD)/memmap -L../
LDLIBS  = -l:$(LIBSYS) -l:$(GPIOEXTRA) -lgcc

all: $(TEST)
	rm -f *.o *.elf

test: $(TEST)
	rm -f *.o *.elf
	$(BUILD)rpi-install.py -p $<

%.bin: %.elf
	arm-none-eabi-objcopy $< -O binary $@

%.elf: %.o
	arm-none-eabi-gcc $(LDFLAGS) $< $(LDLIBS) -o $@

%.o: %.c
	arm-none-eabi-gcc $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o *.bin *.elf *.list *.a *~

.PHONY: all clean test

.PRECIOUS: %.elf %.o %.a

# empty recipe used to disable built-in rules for native build
%:%.c
%:%.o
