#include <stdio.h>
#include <stdlib.h>
#include <htc.h>
#include <math.h>
#include <string.h>
#include "functions.h"
#include "global.h"

/* Constants for square root interplotation below.  The AD_a values are scaled by 2^16. */
const static unsigned int AD_x[] = {16, 32, 48, 64, 80, 96, 112, 127};
const static unsigned long int AD_a[] = {372184, 154164, 118294, 99726, 87861, 79432, 73045, 68127};
const static unsigned char AD_b[] = {0, 53, 71, 84, 96, 106, 116, 124};

/* Globals local to this module */
short int power_light_counter;

/* A-D val for FET temp=343K.  Value decreases with temperature rise. */
#define FET_TEMP_LIMIT 431

#define PULSE_REP_PERIOD 100
#define PULSE_ON_PERIOD 20
#define ENCODER_MAX 127
#define POWER_SLEW_STEP 20

/* Interrupt period is 1.02 mS */
void interrupt tc_int(void)
{
	static bit overtemp_1 = 0, overtemp_2 = 0;
	static unsigned char fet_temp_timer=0;
	static unsigned short int fault_timer_1=0, fault_timer_2=0, pulse_timer=0;
	static unsigned long int fet_temp_1=0, fet_temp_2=0;
	unsigned short int AD_val, period, slewed_period, pwm;
	unsigned char i;
	

	if (TMR0IF) {
		TMR0IF = 0;

		if (!remote_enabled) {
			enc_A_2 = enc_A_1;
			enc_A_1 = ENCODER_A_H;
			enc_B_2 = enc_B_1;
			enc_B_1 = ENCODER_B_H;

			if (enc_A_1 && !enc_A_2) {			//Positive edge on A
				if (!enc_B_1) {
					++enc_position;
				} else {
					--enc_position;
				}
			} else if (!enc_A_1 && enc_A_2) {	//Negative edge on A
				if (!enc_B_1) {
					--enc_position;
				} else {
					++enc_position;
				}			
			} else if (enc_B_1 && !enc_B_2) {	//Positive edge on B
				if (!enc_A_1) {
					--enc_position;
				} else {
					++enc_position;
				}
			} else if (!enc_B_1 && enc_B_2) {	//Negative edge on B
				if (!enc_A_1) {
					++enc_position;
				} else {
					--enc_position;
				}
			}

			if (enc_position < 0) enc_position = 0;
			if (enc_position > ENCODER_MAX) enc_position = ENCODER_MAX;

			/* Output power is proportional to the square root of the amplifier supply
		   	   voltage, which is proportional to the PWM duty cycle.  Compute a piece-wise linear
		   	   approximation to the square root of the encode position from the power level knob, and
		   	   scale the output to the 0x00-0xFF range of the the PWM. */
			for (i=0; i<8; ++i) {
				if (enc_position <= AD_x[i]) {
					period = (unsigned char)((enc_position * AD_a[i]) >> 16) + AD_b[i];
					break;
				}
			}
		} else {
			period = remote_power_level;
		}

		if (period > 256) period = 256;  //Guard against overflow

		/* The power level light blinks with an ~1/2 sec period and a duty cycle proportional
		   to the power level. */
		++power_light_counter;
		if (power_light_counter > 512) power_light_counter = 0;

		if (power_light_counter < (period << 1)) {
			POWER_LEVEL_LED_L = 0;
		} else {
			POWER_LEVEL_LED_L = 1;
		}

		/* If either of the outputs are in a fault condition then set the output power to
			zero.  Otherwise, if both outputs are inactive, the voltage produced by the PWM
			will float up to maximum.  This may then cause another immediate fault when
			the fault timer expires. */
		if ((fault_timer_1 > 0) || (fault_timer_2 > 0)) {
			period = 0;
		}
			
		/* Limit the slew rate when we turn on the power for the RF amplifiers.  Otherwise
		   the abrupt rise may trigger the output over voltage detector. */
		if (period < slewed_period) {
			slewed_period = period;
		} else if (period >= (slewed_period + POWER_SLEW_STEP)) {
			slewed_period += POWER_SLEW_STEP;
		} else if (period < (slewed_period + POWER_SLEW_STEP)) {
			slewed_period = period;
		}

		pwm = (unsigned short)(0x100 - slewed_period);
		CCPR2L = (unsigned char)(pwm >> 2);
		CCP2CON = (unsigned char)((CCP2CON & 0xCF) | ((pwm & 0x03) << 4));
				
		/* Start A-D conversion for FET 1 temperature and wait for done */
		ADCON0 = 0x03;
		while(nDONE);
		ADCON0 = 0x03;
		while(nDONE);
		AD_val = ((unsigned short)ADRESH << 2) +  ((unsigned short)ADRESL >> 6);
		fet_temp_1 += AD_val;	//Accumulate sum for average

		/* Start A-D conversion for FET 2 temperature and wait for done */
		ADCON0 = 0x07;
		while(nDONE);
		ADCON0 = 0x07;
		while(nDONE);
		AD_val = ((unsigned short)ADRESH << 2) +  ((unsigned short)ADRESL >> 6);
		fet_temp_2 += AD_val;	//Accumulate sum for average
	
		++fet_temp_timer;
		if (!fet_temp_timer) {
			if ((fet_temp_1 >> 8) < FET_TEMP_LIMIT) {
				overtemp_1 = 1;
			} else {
				overtemp_1 = 0;			
			}

			if ((fet_temp_2 >> 8) < FET_TEMP_LIMIT) {
				overtemp_2 = 1;
			} else {
				overtemp_2 = 0;			
			}
			fet_temp_1 = fet_temp_2 = 0;		
		}

		/* If the FAULT 1 input is asserted then start a timer.  Otherwise, count
		   the timer down until it reached zero. */
		if (FAULT_1_H) {
			fault_timer_1 = 512;
		} else {
			if (fault_timer_1) --fault_timer_1;
		}

		if (FAULT_2_H) {
			fault_timer_2 = 512;
		} else {
			if (fault_timer_2) --fault_timer_2;
		}

		/* Timer for pulsed mode operation */
		if (pulse_timer < PULSE_REP_PERIOD) {
			++pulse_timer;
		} else {
			pulse_timer = 0;
		}
		
		/* Control the LEDs */
		OVER_TEMP_1_LED_L = !overtemp_1;	
		OVER_TEMP_2_LED_L = !overtemp_2;
		FAULT_1_LED_L = !(fault_timer_1 > 0);
		FAULT_2_LED_L = !(fault_timer_2 > 0);

		if (remote_enabled == 1) {
			REMOTE_MODE_L = 0;

			/* RF ON output control */
			RF_ON_H = !pulse_enabled || (pulse_enabled && (pulse_timer < PULSE_ON_PERIOD));

			/* Amplfier enables */
			ENABLE_1_H = !(fault_timer_1 > 0) && !overtemp_1 & channel1_enabled;
			ENABLE_2_H = !(fault_timer_2 > 0) && !overtemp_2 && channel2_enabled;

		} else {
			REMOTE_MODE_L = 1;

			/* RF ON output control */
			RF_ON_H = CONT_PULSE_H || (!CONT_PULSE_H && (pulse_timer < PULSE_ON_PERIOD));
	
			/* Amplfier enables */
			ENABLE_1_H = !(fault_timer_1 > 0) && !overtemp_1;
			ENABLE_2_H = !(fault_timer_2 > 0) && !overtemp_2 && !SINGLE_DUAL_H;

		}
		
	}
}
