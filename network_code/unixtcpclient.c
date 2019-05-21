



//UNIX 域套接字 TCP 客户端程序

//UNIX 域套接字用来实现本地进程之间的通信

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <sys/un.h>



#define BUF_LEN   128



//.tcpclient  socketName


int clientSocket = 0;

char mySocketName[50] = {0};

int main(int argc, char *argv[])
{
 	int iret = 0;

	if(argc != 2)
	{
		printf("parameter number error\n");
		printf("usage: ./tcpserver socketName\n");

		return 0;
	}

	strcpy(mySocketName, argv[1]);


	//创建socket
	clientSocket = socket(PF_UNIX,SOCK_STREAM,0);
	if(clientSocket == 0)
	{
		printf("client create socket error\n");
		return 0;
	}


	struct sockaddr_un serverAddr;
	int addr_len = sizeof(struct sockaddr_un);
	memset(&serverAddr,0, sizeof(struct sockaddr_un));

	serverAddr.sun_family = PF_UNIX;
	strcpy(serverAddr.sun_path,mySocketName);


	iret = connect(clientSocket, (struct sockaddr *)&serverAddr, addr_len);

	if(iret < 0)
	{
		printf("connect error\n");
		return 0;
	}

	printf("connect server ok\n");


	int dataLen = 0;
	char dataBuf[BUF_LEN];

	//客户端连接成功，开始发送和接收数据,服务器先接收数据
	while(1)
	{

		memset(dataBuf, 0, BUF_LEN);
		strcpy(dataBuf, "I am unix client");
		dataLen = send(clientSocket,dataBuf, strlen(dataBuf),0);

		printf("send return :%d\n", dataLen);

	
		memset(dataBuf, 0, BUF_LEN);
		dataLen = recv(clientSocket,dataBuf, BUF_LEN,0);

		printf("client receive data:%s  recv return:%d\n", dataBuf, dataLen);
		
		sleep(2);
	   
	}
	
}



