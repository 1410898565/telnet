#ifndef _S_TELNET_H_
#define _S_TELNET_H_


#define S_TELNET_TAG		"test$ "

typedef struct
{
	int 	socketFd;
}s_telnetMngr;

typedef void (*telnet_cmd_func)(int socketfd, unsigned char* pdata, int len);
typedef struct
{
	telnet_cmd_func func;
    char telnet_cmd[32]; //max len is 11 bytes
}s_telnet_Cmd;

void s_telnet_Task(void *pArg);
int s_telnet_Init(void);

#endif
