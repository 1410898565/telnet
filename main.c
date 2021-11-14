#include <stdio.h>  
#include "s_telnet.h"


int main(void)
{
	s_telnet_Init();

	s_telnet_Task(NULL);

	return 0;
}

