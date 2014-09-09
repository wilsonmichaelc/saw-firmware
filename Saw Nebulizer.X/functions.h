#include "global.h"

void pic_setup(void);
void serputchar(const unsigned char c);
unsigned char sergetchar(void);
void serputstr(const unsigned char *line);
int sergetline(unsigned char *buf, int bsize);
int parse_setup_cmd(char *buf, CMD_DESRCIPTION *cmd_descr);
int echo_command(char *param1);
int remote_command(char *param1);
int enable1_command(char *param1);
int enable2_command(char *param1);
int pulse_command(char *param1);
int power_command(char *param1);
int ttlout_command(char *param1);
int status_command(char *param1);