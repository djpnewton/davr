/*
/
/ Larry the Robot
/
/ (ATMega128)
/
*/

#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

/* Mode */
enum { MSTOP, MTRIMPOT, MDANCE };
volatile int g_mode = MSTOP;

void beep(int freq, int len);

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

void init_button_timer()
{
	// output PORTA.1
	DDRA = 255;
	//DDRA |= _BV(1);
	DDRA &= ~_BV(3);
	PORTA |= _BV(1);
	//PINA = 0;

	TCCR0 |= _BV(WGM01); // configure timer 0 for CTC mode
	TIMSK |= _BV(OCIE0);
	TIFR |= _BV(OCF0);
	OCR0 = 255; 
	TCCR0 |= _BV(CS02) | _BV(CS01) | _BV(CS00); // start timer at Fcpu / 1024
}

void init_piezo_timer()
{
	// output PORTF.5 and PORTF.3
	DDRF |= _BV(5) | _BV(3);
	PORTF |= _BV(5) | _BV(3);

	TCCR2 |= _BV(WGM21); // configure timer 2 for CTC mode
	TIMSK |= _BV(OCIE2);
	TIFR |= _BV(OCF2);
	OCR2 = 255; 
	TCCR2 |= _BV(CS22) | _BV(CS20); // start timer at Fcpu / 1024
}

int main(void)
 {
    init_pwm();
    init_adc();
    init_status_led();
	init_button_timer();
	//init_piezo_timer();

    // turn off motors
    DDRG = 0xFF;
    PORTG = 0; 

     // turn on trimpot
    DDRF |= 2;
    PORTF |= 2;
    DDRA |= _BV(7);
    PORTA &= ~_BV(7);

    // Enable global interupts
    sei(); 

    while(1)  // Loop Forever
    {
        sleep_mode();
    }
}

ISR(ADC_vect)
{

	if (g_mode == MSTOP)
	{
		OCRA = 0;
		OCRC = 0;
	}
	else if (g_mode == MTRIMPOT)
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
}

ISR(TIMER1_OVF_vect)
{

}

enum { LEFT, RIGHT };
enum { STOP = 0, SLOW = 700, MED = 800, FAST = 1000 };
volatile uint16_t g_dance_cnt = 0;
enum { D1 = 0, D2 = 100, D3 = 200, D4 = 250, D5 = 450, DSTOP = 600 };
volatile int g_mode_switch = 0;

void motor_speed(int motor, int speed)
{
	switch (motor)
	{
		case LEFT:
			OCRA = speed;
			return;
		case RIGHT:
			OCRC = speed;
			return;
	}
}


ISR(TIMER0_COMP_vect)
{
	// if button is pressed change gMode
	if (PINA & _BV(3))
		g_mode_switch = 1;
	else if (g_mode_switch)
	{
		g_mode++;
		if (g_mode > MDANCE)
			g_mode = MSTOP;
		else if (g_mode == MDANCE)
		{
			PORTG = 5; // motors forward
			g_dance_cnt = 0; // reset dance counter
		}
		g_mode_switch = 0;

		beep(100 + g_mode * 10, 250);
	}

	// reset flag so interupt gets called again
	TIFR |= _BV(OCF0);

	// dance routine
	if (g_mode == MDANCE)
	{
		switch (g_dance_cnt)
		{
			case D1:
				motor_speed(LEFT, MED);
				motor_speed(RIGHT, MED);
				break;
			case D2:
				motor_speed(LEFT, STOP);
				motor_speed(RIGHT, MED);
				break;
			case D3:
				motor_speed(LEFT, STOP);
				motor_speed(RIGHT, STOP);
				break;
			case D4:
				motor_speed(LEFT, FAST);
				motor_speed(RIGHT, MED);
				break;
			case D5:
				motor_speed(LEFT, FAST);
				motor_speed(RIGHT, STOP);
				break;
		}
		if (g_dance_cnt >= DSTOP)
		{
			motor_speed(LEFT, STOP);
			motor_speed(RIGHT, STOP);
		}
		g_dance_cnt++;
	}
}

volatile int g_beep_count;

void beep(int freq, int len)
{
	//OCR2 = freq;
	//g_beep_count = len;
}

ISR(TIMER2_COMP_vect)
{
	// reset flag so interupt gets called again
	TIFR |= _BV(OCF2);

	if (g_beep_count > 0)
	{
		// toggle beeper pin
		PORTF &= ~_BV(5);

		g_beep_count--;
	}
	else if (g_beep_count == 0)
		PORTF |= _BV(5);
}
