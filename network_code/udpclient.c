

/*
UDP 服务器程序

*/


#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>  
#include <unistd.h>

#include <string.h>


/*
./udps serverIP  port
*/

#define BUF_LEN  128


char serverIP[50] = {0};
unsigned short serverPort = 0;
int clientSocket = 0; 

int main(int argc, char *argv[])
{

	int iret = 0;

	if(argc != 3)
	{
		printf("parameter number error\n");
		printf("usage: ./tcpserver serverIP port\n");

		return 0;
	}

	strcpy(serverIP, argv[1]);
	serverPort = atoi(argv[2]);

	//创建UDP socket
	clientSocket = socket(AF_INET,SOCK_DGRAM,0);
	if(clientSocket == 0)
	{
		printf("create UDP socket error\n");
		return 0;
	}


	struct sockaddr_in serverAddr;
	int addr_len = sizeof(struct sockaddr_in);
	memset(&serverAddr,0, sizeof(struct sockaddr_in));

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(serverPort);
	serverAddr.sin_addr.s_addr = inet_addr(serverIP);
	
	//接收UDP客户端请求

	int dataLen = 0;

	char dataBuf[BUF_LEN];
	
	while(1)
	{

	   memset(dataBuf, 0,BUF_LEN );
	   strcpy(dataBuf, "I am udp client");
	   addr_len = sizeof(struct sockaddr_in);
	   dataLen = sendto(clientSocket,dataBuf, strlen(dataBuf),0,
	                      (struct sockaddr *)&serverAddr, addr_len);

	   printf("send len:%d\n",dataLen );


	#if 0
	   memset(dataBuf, 0,BUF_LEN );
	   dataLen = recvfrom(clientSocket,dataBuf, BUF_LEN,0,
	                      (struct sockaddr *)&serverAddr, &addr_len);

	   serverPort = ntohs(serverAddr.sin_port);

	   strcpy(serverIP, inet_ntoa(serverAddr.sin_addr));

	   printf("recv %d  data: %s from ip  %s:%d\n",dataLen,dataBuf,serverIP, serverPort);

    #endif
    
	   sleep(2);
	  
  
	}
}



