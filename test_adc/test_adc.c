/*
/
/ Testing ADC 
/
/ (http://www.avrfreaks.net/index.php?name=PNphpBB2&file=viewtopic&t=56429)
/
/ (ATMega128)
/
*/

#include <io.h>
#include <avr/interrupt.h> 

int main (void)
{
    DDRE |= (1 << 2); // Set LED1 as output
    DDRG |= (1 << 0); // Set LED2 as output

    ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // Set ADC prescalar to 128 - 125KHz sample rate @ 16MHz

    ADMUX |= (1 << REFS0); // Set ADC reference to AVCC
    ADMUX |= (1 << ADLAR); // Left adjust ADC result to allow easy 8 bit reading

    // No MUX values needed to be changed to use ADC0

    ADCSRA |= (1 << ADFR);  // Set ADC to Free-Running Mode

    ADCSRA |= (1 << ADEN);  // Enable ADC

    ADCSRA |= (1 << ADIE);  // Enable ADC Interrupt
    sei();   // Enable Global Interrupts 

    ADCSRA |= (1 << ADSC);  // Start A2D Conversions 

    while(1)  // Loop Forever
    {
    }
} 

ISR(ADC_vect)
{
    if(ADCH < 128)
    {
        PORTE |= (1 << 2);  // Turn on LED1
        PORTG &= ~(1 << 0); // Turn off LED2
    }
    else
    {
        PORTE &= ~(1 << 2); // Turn off LED1
        PORTG |= (1 << 0);  // Turn on LED2
    }    
} 
