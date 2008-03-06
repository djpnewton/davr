/*
/
/ Testing LED flashing 
/
/ (ATMega128)
/
*/

#define F_CPU 16000000UL  // 16 MHz


#include <io.h>
#include <util/delay.h>

void delay(int msec)
{
    int i;
    for (i = 0; i < msec; i++)
        _delay_ms(1);
}

int main()
{
    while (1)
    {
        PORTA = 0;
        delay(10);
        PORTA = 255;
    }
    return 0;
}
