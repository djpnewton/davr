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
#include <stdbool.h>

/* Mode */
enum { MSTOP, MLINE_FOLLOW, MDANCE, MTRIMPOT };
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

enum { ADC_TRIMPOT, ADC_LINE_SENSOR };
volatile int g_adc_mode;
volatile int g_adc_chan;

void init_adc(int mode)
{
    ADCSRA |= _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0); // Set ADC prescalar to 128 - 125KHz sample rate @ 16MHz

    ADMUX |= _BV(REFS0); // Set ADC reference to AVCC
    ADMUX |= _BV(ADLAR); // Left adjust ADC result to allow easy 8 bit reading

	ADMUX &= ~_BV(MUX0) & ~_BV(MUX1) & ~_BV(MUX2);

	if (mode == ADC_TRIMPOT)
	{
	    // turn on trimpot
    	DDRF |= 2;
    	PORTF |= 2;
    	DDRA |= _BV(7);
    	PORTA &= ~_BV(7);

		 // do nothing to ADMUX use ADC0

	    ADCSRA |= _BV(ADFR);  // Set ADC to Free-Running Mode
	}
	else if (mode == ADC_LINE_SENSOR)
	{
		// turn on ADC pins
		DDRF = 0;
		PINF = 0;
		PINF |= _BV(0) | _BV(1) | _BV(2) | _BV(3);

		// do nothing to ADMUX, start with ADC0
		g_adc_chan = 0;

		ADCSRA &= ~_BV(ADFR); // Set ADC to Single Conversion Mode
	}

    ADCSRA |= _BV(ADEN);  // Enable ADC

    ADCSRA |= _BV(ADIE);  // Enable ADC Interrupt

    ADCSRA |= _BV(ADSC);  // Start A2D Conversions 

	g_adc_mode = mode;
}

volatile int g_status_led = 0;

void init_status_led()
{
    DDRE = _BV(PE3) | _BV(PE4) | _BV(PE5); // LED output at pins PE3-5
}

void init_button_port()
{
	// output PORTA.1
	DDRA = 255;
	//DDRA |= _BV(1);
	DDRA &= ~_BV(3);
	PORTA |= _BV(1);
	//PINA = 0;
}

void init_button_timer()
{
	init_button_port();

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
    //init_adc(ADC_LINE_SENSOR);
    init_status_led();
	init_button_timer();
	//init_piezo_timer();

    // turn off motors
    DDRG = 0xFF;
    PORTG = 0; 

    // Enable global interupts
    sei(); 

    while(1)  // Loop Forever
    {
        sleep_mode();
    }
}

enum { LEFT, RIGHT };
enum { STOP = 0, SLOW = 750, SLOWMED = 800, MED = 850, FAST = 1000 };
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

volatile bool g_line_left_last;
volatile bool g_line_left2;
volatile bool g_line_left1;
volatile bool g_line_right1;
volatile bool g_line_right2;
volatile bool g_line_right_last;

ISR(ADC_vect)
{
    const float adc_max = 255;
    const float adc_half = adc_max / 2.0;

	if (g_mode == MSTOP)
	{
		OCRA = 0;
		OCRC = 0;
	}
	else if (g_mode == MTRIMPOT)
	{
	    int adc = ADCH;

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
	else if (g_mode == MLINE_FOLLOW)
	{
		PORTG = 5; // motors forward
		int adc_comp = adc_max - 20;

		if (g_adc_chan == 3)
		{
			if (ADCH > adc_comp)
			{
                g_line_left2 = true;
				g_line_left_last = true;
				g_line_right_last = false;
			}
            else
                g_line_left2 = false;
		}
		else if (g_adc_chan == 2)
		{
			if (ADCH > adc_comp)
			{
                g_line_left1 = true;
				g_line_left_last = true;
				g_line_right_last = false;
			}
            else
                g_line_left1 = false;
		}
		else if (g_adc_chan == 1)
		{
			if (ADCH > adc_comp)
			{
                g_line_right1 = true;
				g_line_left_last = 0;
				g_line_right_last = true;
			}
            else
                g_line_right1 = false;
		}
		else if (g_adc_chan == 0)
		{
			if (ADCH > adc_comp)
			{
                g_line_right2 = true;
				g_line_left_last = false;
				g_line_right_last = true;
			}	
            else
                g_line_right2 = false;
		}

		if (g_line_left2)
		{
			motor_speed(LEFT, SLOW);
			motor_speed(RIGHT, MED);
		}
		else if (g_line_left1 && g_line_right1)
		{
			motor_speed(LEFT, MED);
			motor_speed(RIGHT, MED);			
		}
		if (g_line_left1)
		{
			motor_speed(LEFT, SLOWMED);
			motor_speed(RIGHT, MED);
		}		
		else if (g_line_right1)
		{
			motor_speed(LEFT, MED);
			motor_speed(RIGHT, SLOWMED);
		}
		else if (g_line_right2)
		{
			motor_speed(LEFT, MED);
			motor_speed(RIGHT, SLOW);
		}
		else if (g_line_left_last)
		{
			motor_speed(LEFT, STOP);
			motor_speed(RIGHT, MED);
		}
		else if (g_line_right_last)
		{
			motor_speed(LEFT, MED);
			motor_speed(RIGHT, STOP);
		}		
		else
		{
			motor_speed(LEFT, MED);
			motor_speed(RIGHT, MED);
		}
	}

	if (g_adc_mode == ADC_LINE_SENSOR)
	{
		g_adc_chan++;
		if (g_adc_chan > 3)
			g_adc_chan = 0;

		ADMUX &= ~_BV(MUX0) & ~_BV(MUX1) & ~_BV(MUX2);
		switch (g_adc_chan)
		{
			case 0:
				break;
			case 1:
				ADMUX |= _BV(MUX0);
				break;
			case 2:
				ADMUX |= _BV(MUX1);
				break;
			case 3:
				ADMUX |= _BV(MUX1) | _BV(MUX0);
				break;
		}

		ADCSRA |= _BV(ADSC);  // Start A2D Conversion
	}
}

ISR(TIMER1_OVF_vect)
{

}

volatile uint16_t g_dance_cnt = 0;
enum { D1 = 0, D2 = 100, D3 = 200, D4 = 250, D5 = 450, DSTOP = 600 };
volatile int g_mode_switch = 0;

ISR(TIMER0_COMP_vect)
{
	// if button is pressed change g_mode
	if (!(PINA & _BV(3)))
		g_mode_switch = 1;
	else if (g_mode_switch)
	{
		g_mode++;
		if (g_mode >= MTRIMPOT) // skip trimpot for now
			g_mode = MSTOP;
		else if (g_mode == MLINE_FOLLOW)
			init_adc(ADC_LINE_SENSOR);
		else if (g_mode == MDANCE)
		{
			PORTG = 5; // motors forward
			g_dance_cnt = 0; // reset dance counter
		}
		else if (g_mode == MTRIMPOT)
			init_adc(ADC_TRIMPOT);
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

    // status led
    if (g_status_led % 10 == 0)
    {
        if (PORTE == _BV(PE5))
           PORTE = _BV(PE3); // PE3 on, PE4 ground, PE5 off
        else
            PORTE = _BV(PE5); // PE3 off, PE4 ground, PE5 on
    }
    g_status_led++;
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
