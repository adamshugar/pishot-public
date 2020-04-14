#include "interrupts.h"
#include "keyboard.h"
#include "printf.h"
#include "shell.h"
#include "uart.h"

void main(void) 
{
    interrupts_init();
    uart_init();
    keyboard_init(KEYBOARD_CLOCK, KEYBOARD_DATA);
    interrupts_global_enable();
    
    shell_init(printf);

    shell_run();
}
