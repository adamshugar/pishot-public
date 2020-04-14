# Makefile to generate library for Raspberry Pi ground-up library
# (Student code written by Adam Shugar winter 2020 @ Stanford for CS107e)

# NOTE: gpioextra module must be included from instructor-provided library
# because it contains private modules (solutions to course assignments)

# Student modules
MODULES = countdown.o backtrace.o cmd_list.o console.o fb.o gl.o gpio.o gprof.o
MODULES += keyboard.o malloc.o printf.o shell.o start.o strings.o timer.o utils.o

# Instructor modules
MODULES += cstart.o font.o interrupts_asm.o interrupts.o
MODULES += mailbox.o pi.o ps2.o rand.o ringbuffer.o uart.o

CFLAGS = -I./include -g -Wall -Og -std=c99 -ffreestanding 
CFLAGS += -mapcs-frame -fno-omit-frame-pointer -mpoke-function-name -Wpointer-arith

vpath %.c ./src_instructor ./src_student
vpath %.s ./src_instructor ./src_student

all: libsys.a

libsys.a: $(MODULES)
	arm-none-eabi-ar -r ./libsys.a $(MODULES)
	rm -f $(MODULES)

%.o: %.c
	arm-none-eabi-gcc $(CFLAGS) -c $< -o $@

%.o: %.s
	arm-none-eabi-as $(ASFLAGS) $< -o $@

%.list: %.o
	arm-none-eabi-objdump --no-show-raw-insn -d $< > $@

clean:
	rm -f *.o *.bin *.elf *.list *~ libsys.a

.PHONY: all clean

.PRECIOUS: %.elf %.o %.a

# empty recipe used to disable built-in rules for native build
%:%.c
%:%.o
