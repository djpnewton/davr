/*
/
/ ADC & PWM
/
/ (http://www.avrfreaks.net/index.php?name=PNphpBB2&file=viewtopic&t=56429)
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
    sei ();


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
    sei();   // Enable Global Interrupts 

    ADCSRA |= (1 << ADSC);  // Start A2D Conversions 
}

int main(void)
{
    init_pwm();
    init_adc();


    while(1)  // Loop Forever
    {
        sleep_mode();
    }
} 

ISR(ADC_vect)
{
    int adc = ADCH;
    const int adc_max = 255;
    const float adc_half = adc_max / 2.0;

    if (adc <= adc_half)
    {
        OCRA = (adc / adc_half) * TIMER1_TOP;
        OCRC = TIMER1_TOP;
    }
    else
    {
        OCRC = ((adc_half - adc - adc_half) / adc_half) * TIMER1_TOP;
        OCRA = TIMER1_TOP;
    }


    //OCRA = (adc_val / 255.0) * TIMER1_TOP;
    //OCRC = (adc_val / 255.0) * TIMER1_TOP;
}

ISR (TIMER1_OVF_vect)           /* Note [2] */
{
/*
    static uint16_t pwm;        
    static uint8_t direction;
	static int counter = 0;

	if (PINA == 255)
	{
		//if (counter % 2 == 0)
		{
			switch (direction)          
			{
			    case UP:
			        if (++pwm == TIMER1_TOP)
			            direction = DOWN;
			        break;

			    case DOWN:
			        if (--pwm == 0)
			            direction = UP;
			        break;
			}

			OCR = pwm;                  
		}
		counter++;
	}
	else
		OCR = TIMER1_TOP;
        */
    //uint16_t pwm = (adc_val / 255.0) * TIMER1_TOP;
    //OCR = pwm;
}
