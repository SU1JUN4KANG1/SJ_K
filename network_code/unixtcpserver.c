

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>  
#include <unistd.h>

#include <string.h>

#include <sys/un.h>



/*UNIX TCP 服务器程序

./tcpserver mySocketName

*/

#define BUF_LEN  128


char serSocketName[50] = {0};

int serverSocket = 0; //服务socket
int connSocket = 0; //连接socket

int main(int argc, char *argv[])
{

	int iret = 0;

	if(argc != 2)
	{
		printf("parameter number error\n");
		printf("usage: ./tcpserver mySocketName\n");

		return 0;
	}

	strcpy(serSocketName, argv[1]);
	

	//创建socket
	serverSocket = socket(PF_UNIX,SOCK_STREAM,0);
	if(serverSocket == 0)
	{
		printf("create socket error\n");
		return 0;
	}

	struct sockaddr_un serverAddr;
	int addr_len = sizeof(struct sockaddr_un);
	memset(&serverAddr,0, sizeof(struct sockaddr_un));

	serverAddr.sun_family = PF_UNIX;
	strcpy(serverAddr.sun_path ,serSocketName);

	

	//绑定socket
	iret = bind(serverSocket,(struct sockaddr *)&serverAddr,addr_len);
	if(iret < 0)
	{
		printf("bind error\n");
		return 0;
	}

	//监听socket
	iret = listen(serverSocket, 20);
	if(iret < 0)
	{
		printf("listen error\n");
		return 0;
	}

	printf("server is listening!!!\n");

	
	connSocket = accept(serverSocket,NULL, NULL);


	if(connSocket < 0)
	{
	   	printf("accept error\n");
		return 0;
	}

	int dataLen = 0;


	char dataBuf[BUF_LEN];

	//客户端连接成功，开始发送和接收数据,服务器先接收数据
	while(1)
	{
		memset(dataBuf, 0, BUF_LEN);
		dataLen = recv(connSocket,dataBuf, BUF_LEN,0);

		printf("server receive data:%s  recv return:%d\n", dataBuf, dataLen);

		memset(dataBuf, 0, BUF_LEN);
		strcpy(dataBuf, "I am unix Server");
		dataLen = send(connSocket,dataBuf, strlen(dataBuf),0);


	   
	}

	return 0;

}

