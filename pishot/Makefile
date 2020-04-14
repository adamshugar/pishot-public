PISHOT = main.bin
TEST = test_countdown.bin

# We include countdown.c and countdown.h (named for
# clarity as _countdown.c and _countdown.h) in the pishot
# directory because they contain a nontrivial portion
# of the work that went into pishot. However, they are
# technically part of the system library (gprof needs
# them in order to work). So the master copies exist
# in the system directory- they are built and included
# in the project via the static system library from there,
# and hence not included in MODULES.
MODULES = sonic.o sonic_rb.o motor.o hoop.o object_vector.o
INCLUDE = ./include
LIBINCLUDE = ../system/include
LIBSYS = ../system/libsys.a # Library for bare-metal interfacing with Raspberry Pi
GPIOEXTRA = ../system/libpi.a
BUILD = ../build/

# TODO: Change -Og to -Ofast once finished
CFLAGS = -I$(LIBINCLUDE) -I$(INCLUDE) -g -Wall -Og -std=c99 -ffreestanding 
CFLAGS += -mapcs-frame -fno-omit-frame-pointer -mpoke-function-name -Wpointer-arith
LDFLAGS = -nostdlib -T $(BUILD)memmap -L../system
LDLIBS  = -l:$(LIBSYS) -l:$(GPIOEXTRA) -lgcc

all: $(PISHOT) $(MODULES)
	rm -f *.o *.elf *~

%.bin: %.elf
	arm-none-eabi-objcopy $< -O binary $@

%.elf: %.o $(MODULES)
	arm-none-eabi-gcc $(LDFLAGS) $^ $(LDLIBS) -o $@

%.o: ./src/%.c
	arm-none-eabi-gcc $(CFLAGS) -c $< -o $@

%.o: ./tests/%.c
	arm-none-eabi-gcc $(CFLAGS) -c $< -o $@

%.list: %.elf
	arm-none-eabi-objdump --no-show-raw-insn -d $< > $@

test: $(TEST)
	rm -f *.o *.elf *~
	$(BUILD)rpi-install.py -p $<

install: $(PISHOT)
	rm -f *.o *.elf *~
	$(BUILD)rpi-install.py -p $<

clean:
	rm -f *.o *.bin *.elf *.list *.a *~

.PHONY: all clean install test

.PRECIOUS: %.elf %.o %.a

# empty recipe used to disable built-in rules for native build
%:%.c
%:%.o.
