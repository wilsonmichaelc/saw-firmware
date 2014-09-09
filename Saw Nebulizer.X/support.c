#include <stdio.h>
#include <stdlib.h>
#include <htc.h>
#include <string.h>
#include <ctype.h>
#include "functions.h"

void pic_setup(void)
{
    /* Select 32 MHx clock */
    OSCCON = 0xF0;
    
    /* PORT A 2 & 7 are outputs */
    TRISA = ~0x84;

    /* PORT B 5-0 are outputs */
    TRISB = ~0x3F;
    
    /* PORT C 3-0 are outputs */
    TRISC = ~0x0F;
    
    /* PORT A 1-0 are analog inputs */
    ANSELA = 0x03;

	/* No analog inputs on PORT B */
	ANSELB = 0x00;

    /* Initialize the outputs */
    RF_ON_H = 0;
    ENABLE_1_H = 0;
    ENABLE_2_H = 0;
    PWM_H = 0;
    FAULT_1_LED_L = 1;
    OVER_TEMP_1_LED_L = 1;
    FAULT_2_LED_L = 1;
    OVER_TEMP_2_LED_L = 1;
	POWER_LEVEL_LED_L = 1;
    REMOTE_MODE_L = 1;
	TTL_OUT_L = 1;

    /* Setup A-D: right justified, FOSC/64, -VREF is VSS, +VREF is VDD */
    ADCON1 = 0x60;
    
    /* Enable A-D*/
    ADCON0 = 0x01;

    /* Options: weak pull-ups enabled, Timer0 uses FOSC/4, /32 prescaler */
    /* Timer 0 will provide interrupt every 1mS*/
    OPTION_REG = 0x04;

    /* Setup CCP2 as simple PWM using Timer 2, 125KHz */
    CCP2CON = 0x0C;
    CCPTMRS0 = 0x00;
    PR2 = 0x3F;
    T2CON = 0x04;
    CCPR2L = 0x00;

	/* Setup the UART for 9600 baud 8 bits */
	BRG16 = 1;
	BRGH = 1;
	SPBRGL = 0x40;
	SPBRGH = 0x03;
	SYNC = 0;
	SPEN = 1;
	CREN = 1;
	TXEN = 1;	

	/* Initialize the rotary encoder variables so the knob position always
	   starts at zero. */
	enc_A_2 = enc_A_1 = ENCODER_A_H;
	enc_B_2 = enc_B_1 = ENCODER_B_H;
	enc_position = 0;
	power_light_counter = 0;

	/* Enable the timer interrupt */
	TMR0IE = 1;
	GIE = 1;
}


void serputchar(const unsigned char c)
{
	while(TXIF == 0);
	TXREG = c;
}

unsigned char sergetchar(void)
{
	while(RCIF == 0);
	return RCREG;
}

void serputstr(const unsigned char *line)
{
	int len, i;

	len = strlen(line);
	for (i = 0; i < len; ++i) {
		serputchar(line[i]);
	}
}

int sergetline(unsigned char *buf, int bsize)
{
	int i;
	unsigned char c;
	
	i = 0;
	while(1) {
		c = sergetchar();
		c = tolower(c);						//Convert to lower case
		/* Handle backspace and delete the same */
  		if (c == 0x08 || c == 0x7f) {
        	if (i > 0) {
            	if (echo_enabled) serputstr("\b \b");
         		--i;
        	}
			continue;
		}
		if (c == 0x0A) continue;			//Ignore line feeds
		if (c == 0x0D) {					//Carriage return terminates
			if (echo_enabled) serputstr("\r\n");	
			break;
		}
		buf[i++] = c;
		if (echo_enabled) serputchar(c);
		if (i >= (bsize - 1)) return -1;	//Buffer overflow
	}
	buf[i] = 0;								//Terminate srting
	return 0;								//Good status
}

