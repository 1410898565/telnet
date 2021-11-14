#include <stdio.h>  
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h> 

#include "s_telnet.h"

s_telnetMngr	gs_telnetMngr;

//////////////////////////////////////////////////////////////////////////////////////
void s_telnet_send(int conSocketFd, uint8_t *pSendData, int senLen)
{
	int reseult = -1;

	if(pSendData != NULL || senLen !=0)
	{
		//先发tag
		reseult = send(conSocketFd, S_TELNET_TAG, strlen(S_TELNET_TAG), 0);
		if(reseult<0)
		{
			printf("send error:%d\r\n", reseult);  
			return;
		}

		//再发数据
		reseult = send(conSocketFd, pSendData, senLen, 0);
		if(reseult<0)
		{
			printf("send error:%d\r\n", reseult);  
			return;
		} 
		//再发tag
		reseult = send(conSocketFd, S_TELNET_TAG, strlen(S_TELNET_TAG), 0);
		if(reseult<0)
		{
			printf("send error:%d\r\n", reseult);  
			return;
		}
	}
	else
	{	
		//发个tag
		reseult = send(conSocketFd, S_TELNET_TAG, strlen(S_TELNET_TAG), 0);
		if(reseult<0)
		{
			printf("send error:%d\r\n", reseult);  
			return;
		}
	}
}

void s_telnet_cmd_help(int socketfd,unsigned char* pdata, int len)
{
	int sendLen = 0;
	char reply[256] = {0};

	strncpy(reply+sendLen, "\r\n------------esp32 nvt cmds-------------\r\n", strlen("\r\n------------esp32 nvt cmds-------------\r\n"));
	sendLen += strlen("\r\n------------esp32 nvt cmds-------------\r\n");
	
	strncpy(reply+sendLen, "enableLog : start to receive log\r\n", strlen("enableLog : start to receive log\r\n"));
	sendLen += strlen("enableLog : start to receive log\r\n");
	
	strncpy(reply+sendLen, "disableLog: stop receiving log\r\n", strlen("disableLog: stop receiving log\r\n"));
	sendLen += strlen("disableLog: stop receiving log\r\n");
	
	strncpy(reply+sendLen, "\r\nTips: We only support these orders now\r\n", strlen("Tips: We only support these orders now\r\n"));
	sendLen += strlen("Tips: We only support these orders now\r\n");

	strncpy(reply+sendLen, "\r\n---------------------------------------\r\n", strlen("\r\n---------------------------------------\r\n"));
	sendLen += strlen("\r\n---------------------------------------\r\n");

	s_telnet_send(socketfd, reply, sendLen);
}

void s_telnet_cmd_enableLog(int socketfd,unsigned char* pdata, int len)
{
	int sendLen = 0;
	char reply[64] = {0};

	sendLen = snprintf(reply, sizeof(reply)-1, "enableLog ok\r\n", pdata);
	s_telnet_send(socketfd, reply, sendLen);
}

void s_telnet_cmd_disableLog(int socketfd, unsigned char* pdata, int len)
{
	int sendLen = 0;
	char reply[64] = {0};

	sendLen = snprintf(reply, sizeof(reply)-1, "disableLog ok\r\n", pdata);
	s_telnet_send(socketfd, reply, sendLen);
}

void s_telnet_cmd_notFound(int socketfd,unsigned char* pdata, int len)
{
	int sendLen = 0;
	char reply[64] = {0};

	if(strlen(pdata) > 0)			
	{
		sendLen = snprintf(reply, sizeof(reply)-1, "%s: not found\r\n", pdata);
		s_telnet_send(socketfd, reply, sendLen);
	}
	else
	{
		s_telnet_send(socketfd, NULL, 0);
	}
}

s_telnet_Cmd s_telnet_CmdSets[]=
{
	{s_telnet_cmd_help,"help"},
	{s_telnet_cmd_enableLog,"enableLog"},
	{s_telnet_cmd_disableLog,"disableLog"}
};

int s_telnet_process(int conSocketFd, uint8_t *pRcvData, int rcvLen)
{
	int isFound = 0;
	int ni = 0;
	char inputCmd[32+1] = {0};
	
	//计算命令个数
	int s_telnet_cmd_num = sizeof(s_telnet_CmdSets)/sizeof(s_telnet_CmdSets[0]);

	//提取命令
	for(ni=0;ni<(sizeof(inputCmd)-1) && ni<rcvLen; ni++)
	{
		if(pRcvData[ni] != '\r' && pRcvData[ni] != '\n')
			inputCmd[ni] = pRcvData[ni];
	}

	//查找命令对应的处理
	for(ni=0; ni<s_telnet_cmd_num; ni++)
	{
		if(strncmp(inputCmd, s_telnet_CmdSets[ni].telnet_cmd,strlen(s_telnet_CmdSets[ni].telnet_cmd))==0)
		{
			isFound = 1;
			break;
		}
	}

	//是否找到
	if(isFound == 0)
	{
		//没找到对应命令则回复命令不支持
		s_telnet_cmd_notFound(conSocketFd, inputCmd, rcvLen);
	}
	else
	{
		//否则调用命令执行函数
		s_telnet_CmdSets[ni].func(conSocketFd, pRcvData, rcvLen);
	}
	
	return 0;
}

void s_telnet_Task(void *pArg)
{
	int rcvLen		= -1;
	int conSocketFd = -1;

	int nOption = 1;


	uint8_t rcvBuf[256] = {0}; 
	struct sockaddr_in ser_addr;//服务器IP地址
	struct sockaddr_in client_addr;//客户机IP地址
	socklen_t length 	= sizeof(struct sockaddr_in);

	if(gs_telnetMngr.socketFd<0)
	{
		printf("socketFd error\r\n");
		return;
	}

	setsockopt(gs_telnetMngr.socketFd, SOL_SOCKET, TCP_NODELAY, (void*)&nOption, sizeof(nOption));

		//设定要绑定的地址
	bzero(&ser_addr,sizeof(struct sockaddr_in));
	ser_addr.sin_family 		= AF_INET;			//ipv4
	ser_addr.sin_port 			= htons(18181);		//telnet默认端口23
	ser_addr.sin_addr.s_addr	= htonl(INADDR_ANY);//设置为任何IP都可以通信
	
	//绑定地址:套接字、绑定地址的结构体，地址长度
	bind(gs_telnetMngr.socketFd,(struct sockaddr*)&ser_addr,sizeof(ser_addr));
	
	//监听窗口
	listen(gs_telnetMngr.socketFd,1);//第二个参数为允许连接的客户机数目
	

	//等待连接
	while(gs_telnetMngr.socketFd>0)
	{
		memset(&client_addr, 0, sizeof(client_addr));
		conSocketFd = accept(gs_telnetMngr.socketFd,(struct sockaddr*)&client_addr,&length);
		if(conSocketFd<0)
		{
			printf("accept new connection error\r\n");
			continue;
		}
		printf("New connection:%d ,Cient IP: %s:%d\n",conSocketFd, inet_ntoa(client_addr.sin_addr),htons(client_addr.sin_port));

		usleep(100*1000);
		
		//先发tag
		rcvLen = send(conSocketFd, S_TELNET_TAG, strlen(S_TELNET_TAG), 0);

		//等待接收处理数据
		while((rcvLen=recv(conSocketFd,rcvBuf,sizeof(rcvBuf)-1,0))>0)  
		{
			//数据处理
			s_telnet_process(conSocketFd,rcvBuf, rcvLen);
		} 

		printf("close connection:%d\r\n",conSocketFd);
		//结束连接	
		close(conSocketFd);
	}

	if(gs_telnetMngr.socketFd>0)
		close(gs_telnetMngr.socketFd);
}


int s_telnet_Init(void)
{
	memset(&gs_telnetMngr, 0, sizeof(gs_telnetMngr));

	//创建套接字
	gs_telnetMngr.socketFd = socket(AF_INET, SOCK_STREAM,0);//创建TCP套接字
	if(gs_telnetMngr.socketFd == -1) 
	{
		printf("Create socket failed\n");
		return -1;
	}

	return 0;
}

