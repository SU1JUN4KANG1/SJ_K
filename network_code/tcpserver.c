
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>  
#include <unistd.h>

#include <string.h>


/*TCP 服务器程序

./tcpserver serverip  port

*/

#define BUF_LEN  128


char serverIP[50] = {0};
unsigned short serverPort = 0;
int serverSocket = 0; //服务socket

int connSocket = 0; //连接socket

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

	//创建socket
	serverSocket = socket(AF_INET,SOCK_STREAM,0);
	if(serverSocket == 0)
	{
		printf("create socket error\n");
		return 0;
	}

	struct sockaddr_in serverAddr;
	int addr_len = sizeof(struct sockaddr_in);
	memset(&serverAddr,0, sizeof(struct sockaddr_in));

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(serverPort);
	serverAddr.sin_addr.s_addr = inet_addr(serverIP);
	

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

	struct sockaddr_in clientAddr;
	memset(&clientAddr,0, sizeof(struct sockaddr_in));
	

	char clientIP[50] = {0};
	unsigned short clientPort = 0;

	connSocket = accept(serverSocket,(struct sockaddr *)&clientAddr, &addr_len);

	clientPort = ntohs(clientAddr.sin_port);
	strcpy(clientIP,inet_ntoa(clientAddr.sin_addr));

	printf("new client connect:IP: %s   Port:%d\n",clientIP,clientPort);

	

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
		strcpy(dataBuf, "I am Server");
		dataLen = send(connSocket,dataBuf, strlen(dataBuf),0);


	   
	}

	return 0;

}
