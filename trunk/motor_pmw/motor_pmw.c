/*
/
/ Motor PMW
/
/ (ATMega128)
/
*/

#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>


/* Timer defines */

#define TIMER1_PWM_INIT _BV(WGM10) | _BV(WGM11) | _BV(COM1A1) | _BV(COM1C1)
#define TIMER1_CLOCKSOURCE _BV(CS10) /* full clock */
#define OC1 PB5
#define OC3 PB7
#define DDROC DDRB
#define OCRA OCR1A
#define OCRC OCR1C
#define TIMER1_TOP 1023	/* 10-bit PWM */

void init_pwm()
{
    /* Timer 1 is 10-bit PWM */
    TCCR1A = TIMER1_PWM_INIT;
    /*
     * Start timer 1.
     *
     * NB: TCCR1A and TCCR1B could actually be the same register, so
     * take care to not clobber it.
     */
    TCCR1B |= TIMER1_CLOCKSOURCE;
    /*
     * Run any device-dependent timer 1 setup hook if present.
     */
#if defined(TIMER1_SETUP_HOOK)
    TIMER1_SETUP_HOOK();
#endif

    /* Set PWM value to 0. */
    OCRA = 0;
    OCRC = 0;

    /* Enable OC1 and OC2 as output. */
    DDROC = _BV(OC1) | _BV(OC3);

    /* Enable timer 1 overflow interrupt. */
    TIMSK = _BV (TOIE1);

	// set up switch input
	DDRA = 0x00;
}

void init_adc()
{
    ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // Set ADC prescalar to 128 - 125KHz sample rate @ 16MHz

    ADMUX |= (1 << REFS0); // Set ADC reference to AVCC
    ADMUX |= (1 << ADLAR); // Left adjust ADC result to allow easy 8 bit reading

    // No MUX values needed to be changed to use ADC0

    ADCSRA |= (1 << ADFR);  // Set ADC to Free-Running Mode

    ADCSRA |= (1 << ADEN);  // Enable ADC

    ADCSRA |= (1 << ADIE);  // Enable ADC Interrupt

    ADCSRA |= (1 << ADSC);  // Start A2D Conversions 
}

void init_status_led()
{
    DDRE = _BV(PE3) | _BV(PE4) | _BV(PE5); // LED output at pins PE3-5
    PORTE = _BV(PE3); // PE3 on, PE4 ground, PE5 off
    TCCR3B = _BV(WGM12); // Configure timer 3 for CTC mode
    TCCR3A = _BV(COM3A0) | _BV(COM3C0); // CTC mode, toggle on compare match
    OCR3A = 65200; // set CTC compare value to 1Hz at 16Mhz clock, with a prescaler of 64
    TCCR3B |= /*_BV(CS30) | _BV(CS31) |*/ _BV(CS32); // prescaler Fcpu / 64
}

int main(void)
 {
    init_pwm();
    init_adc();
    init_status_led();

    // turn off motors
    DDRG = 0xFF;
    PORTG = 0; 

     // turn on trimpot
    DDRF = 2;
    PORTF = 2;
    DDRA = 255;
    PORTA = 0;

    // Enable global interupts
    sei(); 

    while(1)  // Loop Forever
    {
        sleep_mode();
    }
}

ISR(ADC_vect)
{
    int adc = ADCH;
    const float adc_max = 255;
    const float adc_half = adc_max / 2.0;

    if (adc <= adc_half)
    {
        PORTG = 5; // motors forward

        OCRA = ((adc_half - adc) / adc_half) * (TIMER1_TOP);
        OCRC = ((adc_half - adc) / adc_half) * (TIMER1_TOP);
    }
    else
    {
        PORTG = 10; // motors backward

        OCRA = ((adc - adc_half) / adc_half) * TIMER1_TOP;
        OCRC = ((adc - adc_half) / adc_half) * TIMER1_TOP;
    }
}

ISR (TIMER1_OVF_vect)
{

}
