# Change APPLICATION target as necessary
APPLICATION = clock.bin

INCLUDE = ../system/include
LIBSYS = ../system/libsys.a # Library for bare-metal interfacing with Raspberry Pi
GPIOEXTRA = ../system/libpi.a # Include private instructor module for GPIO interrupts and pull-up/down resistors
BUILD = ../build/

CFLAGS = -I $(INCLUDE) -g -Wall -Og -std=c99 -ffreestanding 
CFLAGS += -mapcs-frame -fno-omit-frame-pointer -mpoke-function-name -Wpointer-arith
LDFLAGS = -nostdlib -T $(BUILD)memmap -L../system
LDLIBS  = -l:$(LIBSYS) -l:$(GPIOEXTRA) -lgcc

all: $(APPLICATION)
	rm -f *.o *.elf

%.bin: %.elf
	arm-none-eabi-objcopy $< -O binary $@

%.elf: %.o
	arm-none-eabi-gcc $(LDFLAGS) $< $(LDLIBS) -o $@

%.o: %.c
	arm-none-eabi-gcc $(CFLAGS) -c $< -o $@

%.o: %.s
	arm-none-eabi-as $(ASFLAGS) $< -o $@

%.list: %.o
	arm-none-eabi-objdump --no-show-raw-insn -d $< > $@

install: $(APPLICATION)
	rm -f *.o *.elf
	$(BUILD)rpi-install.py -p $<

clean:
	rm -f *.o *.bin *.elf *.list *.a *~

.PHONY: all clean install

.PRECIOUS: %.elf %.o %.a

# empty recipe used to disable built-in rules for native build
%:%.c
%:%.o
