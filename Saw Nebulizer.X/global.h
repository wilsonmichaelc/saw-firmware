#ifndef __GLOBAL__

#define VERSION "1.0"

/* I/O pin definitions */
#define FAULT_1_H RA3
#define FAULT_2_H RA4
#define SINGLE_DUAL_H RA5
#define CONT_PULSE_H RA6
#define TTL_OUT_L RA7
#define RF_ON_H RC2
#define ENABLE_1_H RC0
#define ENABLE_2_H RC3
#define PWM_H RC1
#define ENCODER_A_H RC4
#define ENCODER_B_H RC5
#define FAULT_1_LED_L RB0
#define OVER_TEMP_1_LED_L RB1
#define FAULT_2_LED_L RB2
#define OVER_TEMP_2_LED_L RB3
#define POWER_LEVEL_LED_L RB4
#define REMOTE_MODE_L RB5

/* Size of buffer for commands from server */
#define	CMDBUFSIZE	10

typedef struct command {
	char arg1[CMDBUFSIZE];
	int (*cmdptr)(char *);
} CMD_DESRCIPTION;

typedef struct cmdlist {
	const char *cmdname;
	int (*cmdptr)(char *);
} CMD_LIST_ITEM;


/* Global varibles for communication with interrupt routine */
extern bit echo_enabled;
extern bit remote_enabled;
extern bit channel1_enabled;
extern bit channel2_enabled;
extern bit pulse_enabled;
extern unsigned char remote_power_level;
extern short int power_light_counter;

/* Global variables used to maintain rotary encoder position */
extern bit enc_A_1, enc_A_2, enc_B_1, enc_B_2;
extern int enc_position;

#define __GLOBAL__
#endif