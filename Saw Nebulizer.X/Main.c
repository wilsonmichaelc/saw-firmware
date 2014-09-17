/* 
 * File:   Main.c
 * Author: gladden
 *
 * Created on April 10, 2012, 7:53 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <htc.h>
#include <math.h>
#include <string.h>
#include "functions.h"
#include "global.h"

/* Global varibles for communication with interrupt routine */
bit echo_enabled = 0;
bit remote_enabled = 0;
bit channel1_enabled = 0;
bit channel2_enabled = 0;
bit pulse_enabled = 0;
unsigned char remote_power_level = 0;
short int power_light_counter = 0;

/* Global variables used to maintain rotary encoder position */
bit enc_A_1, enc_A_2, enc_B_1, enc_B_2;
int enc_position;

/* Processor configuration register */
__CONFIG(FCMEN_OFF  & IESO_OFF & CLKOUTEN_OFF &  BOREN_ON & CPD_OFF & CP_OFF &
         MCLRE_OFF & PWRTE_ON & WDTE_OFF & FOSC_INTOSC);
__CONFIG(LVP_ON &  BORV_LO & STVREN_OFF & PLLEN_OFF & WRT_OFF );

/* Entry point */
int main(int argc, char** argv) {
	int status;
	long int i, j;
	unsigned char cmdbuf[CMDBUFSIZE], temp[20];
	CMD_DESRCIPTION cmd_descr;

/* Setup the processor */
	pic_setup();

/* Delay to allow clock to stabilize before attempting to use serial port */
	for (i = 0; i < 10000; ++i) {
		j = i * i;
	}

	serputstr("\rSAW Nebulizer RF Driver - " VERSION "\r\n");

/* Loop and wait for commands. All the actual hardware control takes place in the
   interrupt routine. */	
	while(1) {
		status = sergetline(cmdbuf, sizeof(cmdbuf));
		if (status != 0) {
			if (!echo_enabled) {
				serputstr("ERR4\r");			//Buffer overflow
			} else {
				serputstr("ERR4 command line buffer overflow\r\n");
			}	
			continue;
		}
		
		if (strlen(cmdbuf) == 0) {			//Empty command
			serputstr("OK\r");
			if (echo_enabled) serputstr("\n");
			continue;
		}
		
		/* Parse the command */
		status = parse_setup_cmd(cmdbuf, &cmd_descr);
		if (status != 0) {
			if (!echo_enabled) {
				serputstr("ERR1\r");			//Bad command verb
			} else {
				serputstr("ERR1 bad command verb\r\n");
			}	
			continue;
		}

		/* Dispatch the command */
		status = (*cmd_descr.cmdptr)(cmd_descr.arg1);
		if (status == -1) {
			if (!echo_enabled) {
				serputstr("ERR2\r");			//Parameter missing
			} else {
				serputstr("ERR2 parameter missing\r\n");
			}	
			continue;

		} else if (status == -2) {
			if (!echo_enabled) {
				serputstr("ERR3\r");			//Bad parameter value
			} else {
				serputstr("ERR3 bad parameter value\r\n");
			}	
			continue;

		} else if (status == -3) {
			continue;						//Suppress reply

		} else {
			serputstr("OK\r");				//Success
			if (echo_enabled) serputstr("\n");
			continue;
		}

	}

    return (EXIT_SUCCESS);					//No way to get here
}

/* Command dispatch list.  These are the recognized server commands. */
const CMD_LIST_ITEM commands[] = {
	{"echo", echo_command},
	{"rem", remote_command},
	{"enab1", enable1_command},
	{"enab2", enable2_command},
	{"pulse", pulse_command},
	{"pwr", power_command},
	{"ttlout", ttlout_command},
	{"status", status_command},
        {"errors", errors_command}
};

/* Function to parse command and parameter. If succesful the function
   copies a pointer to the function that implements the parsed command,
   and copies the parameter (if included), into the structure passed by
   reference as argument "cmd_descr".  The function returns "0" to indicate
   success or "-1" to indicate failure. */
int parse_setup_cmd(char *buf, CMD_DESRCIPTION *cmd_descr)
{
	char cbuf[CMDBUFSIZE];
	char *tokptr;
	int num_cmds, i;

/*	Clear the argument string. */
	cmd_descr->arg1[0] = 0;

/*	The first token in the string is the command */
	tokptr = strtok(buf, " ");			//Space is the only delimiter
	if (tokptr == NULL) {
 		return -1;						//Empty command line
	} else {
		strcpy(cbuf, tokptr);
	}

/*	Check for the first argument (remainder of the command string) */
	tokptr = strtok(NULL, "~");
	if (tokptr != NULL) {
		strcpy(cmd_descr->arg1, tokptr);
	} else {
       cmd_descr->arg1[0] = 0;
    }

/*	Attempt to look up the command */
	num_cmds = sizeof(commands) / sizeof(CMD_LIST_ITEM);
	for (i = 0; i < num_cmds; ++i) {
		if (strcmp(cbuf, commands[i].cmdname) == 0) {
			cmd_descr->cmdptr = commands[i].cmdptr;
			return 0;					//Success
		}
	}

	return -2;							//Command not found
}

int echo_command(char *param1)
{
	int val;

	if (strlen(param1) == 0) return -1;			//Missing parameter
	val = atoi(param1);
	if (val < 0 || val > 1) return -2;			//Bad parameter value
	echo_enabled = val;
	return 0;
}

int remote_command(char *param1)
{
	int val;

	if (strlen(param1) == 0) return -1;			//Missing parameter
	val = atoi(param1);
	if (val < 0 || val > 1) return -2;			//Bad parameter value
	remote_enabled = val;
	return 0;
}	

int enable1_command(char *param1)
{
	int val;

	if (strlen(param1) == 0) return -1;			//Missing parameter
	val = atoi(param1);
	if (val < 0 || val > 1) return -2;			//Bad parameter value
	channel1_enabled = val;
	return 0;
}

int enable2_command(char *param1)
{
	int val;

	if (strlen(param1) == 0) return -1;			//Missing parameter
	val = atoi(param1);
	if (val < 0 || val > 1) return -2;			//Bad parameter value
	channel2_enabled = val;
	return 0;
}

int pulse_command(char *param1)
{
	int val;

	if (strlen(param1) == 0) return -1;			//Missing parameter
	val = atoi(param1);
	if (val < 0 || val > 1) return -2;			//Bad parameter value
	pulse_enabled = val;
	return 0;
}

int power_command(char *param1)
{
	int val;
	long int temp;

	if (strlen(param1) == 0) return -1;			//Missing parameter
	val = atoi(param1);
	if (val < 0 || val > 100) return -2;		//Bad parameter value
	
	/* Scale value to 0-255 range. */
	temp = (long int)val * 167116L + 32768L;				//Multiply by (2^16 * 2.55)
	remote_power_level = temp >> 16;			//Divide by 2^16
	return 0;
}

int ttlout_command(char *param1)
{
	int val;

	if (strlen(param1) == 0) return -1;			//Missing parameter
	val = atoi(param1);
	if (val < 0 || val > 1) return -2;			//Bad parameter value
	TTL_OUT_L = ~val;
	return 0;
}


int status_command(char *param1)
{
	char buf[10];
	long int temp;

	serputstr("OK ");
	sprintf(buf, "REM=%d ", remote_enabled);
	serputstr(buf);
	
	sprintf(buf, "EN1=%d ", channel1_enabled);
	serputstr(buf);

	sprintf(buf, "EN2=%d ", channel2_enabled);
	serputstr(buf);

	sprintf(buf, "PUL=%d ", pulse_enabled);
	serputstr(buf);

	temp = (long int)remote_power_level * 25700L + 32768L;
	temp = temp >> 16;
	sprintf(buf, "PWR=%d ", (int)temp);
	serputstr(buf);

	sprintf(buf, "FL1=%d ", ~FAULT_1_LED_L & 0x0001);
	serputstr(buf);

	sprintf(buf, "FL2=%d ", ~FAULT_2_LED_L & 0x0001);
	serputstr(buf);

	sprintf(buf, "OT1=%d ", ~OVER_TEMP_1_LED_L & 0x0001);
	serputstr(buf);

	sprintf(buf, "OT2=%d", ~OVER_TEMP_2_LED_L & 0x0001);
	serputstr(buf);

	serputstr("\r");
	if (echo_enabled) serputstr("\n");

	return -3;		//Not an error, but suppresses "OK" from command dispatcher
	
}

int errors_command(char *param1)
{
	char buf[10];
	serputstr("ERR ");

	sprintf(buf, "FL1=%d ", ~FAULT_1_LED_L & 0x0001);
	serputstr(buf);

	sprintf(buf, "FL2=%d ", ~FAULT_2_LED_L & 0x0001);
	serputstr(buf);

	sprintf(buf, "OT1=%d ", ~OVER_TEMP_1_LED_L & 0x0001);
	serputstr(buf);

	sprintf(buf, "OT2=%d", ~OVER_TEMP_2_LED_L & 0x0001);
	serputstr(buf);

	serputstr("\r");
	if (echo_enabled) serputstr("\n");

	return -3;		//Not an error, but suppresses "OK" from command dispatcher

}