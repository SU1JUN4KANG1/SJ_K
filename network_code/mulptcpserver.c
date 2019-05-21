

/*
	多进程实现TCP并发服务器
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>  
#include <unistd.h>

#include <string.h>


#define BUF_LEN  128

int dataLen = 0;
char dataBuf[BUF_LEN];

 
char serverIP[50] = {0};  //服务器IP
unsigned short serverPort = 0;  //服务器端口号

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
	int cli_pid = 0;

	while(1)
	{
		//主进程处理新的客户端连接建立请求
		connSocket = accept(serverSocket,(struct sockaddr *)&clientAddr, &addr_len);

		clientPort = ntohs(clientAddr.sin_port);
		strcpy(clientIP,inet_ntoa(clientAddr.sin_addr));

		printf("new client connect:IP: %s   Port:%d\n",clientIP,clientPort);

		if(connSocket < 0)
		{
		   	printf("accept error\n");
			return 0;
		}

		cli_pid = fork(); //创建子进程，用来负责和客户端进行数据通信

		if(cli_pid == 0)
		{
			//子进程负责与客户端进行数据通信
			close(serverSocket); //关闭监听socket

			while(1)
			{
				memset(dataBuf, 0, BUF_LEN);
				dataLen = recv(connSocket,dataBuf, BUF_LEN,0);

				printf("server receive data:%s  recv return:%d\n", dataBuf, dataLen);
				if(dataLen <= 0)
				{  //dataLen =0 表示对方关闭了connSocket
					break;
				}

				memset(dataBuf, 0, BUF_LEN);
				strcpy(dataBuf, "I am Server");
				dataLen = send(connSocket,dataBuf, strlen(dataBuf),0);
			}

			exit(0);
			
		}

		//父进程，关闭连接socket,继续处理客户端连接建立请求
		close(connSocket);	
	}

	return 0;
}

