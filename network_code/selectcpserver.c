


/*
 多路IO复用技术实现TCP SERVER
*/


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <arpa/inet.h>


#define BUF_LEN  	128
#define MAXFD_NUM  	100  

int dataLen = 0;
char dataBuf[BUF_LEN];


char serverIP[50] = {0};
unsigned short serverPort = 0;
int serverSocket = 0; //服务socket

int connSocket = 0; //连接socket

int main(int argc, char *argv[])
{

	int iret = 0;
	int i = 0;

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

	
	//绑定socket
	struct sockaddr_in serverAddr;
	int addr_len = sizeof(struct sockaddr_in);
	memset(&serverAddr,0, sizeof(struct sockaddr_in));

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(serverPort);
	serverAddr.sin_addr.s_addr = inet_addr(serverIP);
	
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


	struct timeval timeout = {0};
	fd_set fd_read;		 //定义读描述符集合
	fd_set fd_backup;	 //定义备份描述符集合
	int fd_all[MAXFD_NUM]={0};  //保存所有的socket描述符
	
	int max_fd = serverSocket; 	//定义最大描述符

	fd_all[0] = serverSocket;
	FD_ZERO(&fd_read);
	FD_ZERO(&fd_backup);
	
	FD_SET(serverSocket, &fd_read);
	FD_SET(serverSocket, &fd_backup);

	while(1)
	{
			//设置超时时间
		timeout.tv_sec = 10;
		timeout.tv_usec = 0;

		fd_read = fd_backup;

		iret = select(max_fd+1, &fd_read,NULL,NULL,&timeout);
		//当select 返回时，fd_read只保留了有数据可读的描述符

		if(iret < 0)
		{
			//select 函数出错
			printf("select error\n");
			break;
		}

		if(iret == 0)
		{
			printf("select timeout\n");
		}

		//iret > 0 表示有文件描述符有数据可读

		//检测监听描述符是否有数据可读
		if(FD_ISSET(serverSocket, &fd_read))
		{
			//监听描述符有数据可读,表示有新的客户端建立连接请求
			connSocket = accept(serverSocket, (struct sockaddr *)&clientAddr,&addr_len);
			if(connSocket < 0)
			{
			   	printf("accept error\n");
				return 0;
			}

			clientPort = ntohs(clientAddr.sin_port);
			strcpy(clientIP,inet_ntoa(clientAddr.sin_addr));

			printf("new client connect:IP: %s   Port:%d\n",clientIP,clientPort);


			//将新的连接socket加入到fd_all和备份描述符集合

			for(i = 0; i< MAXFD_NUM;i++)
			{
				if(fd_all[i] == 0)
				{
					fd_all[i] = connSocket;
					break;
				}
			}

			FD_SET(connSocket, &fd_backup);

			//更新最大描述符
			if(connSocket > max_fd)
			{
				max_fd = connSocket;
			}
			
		}


		//检测客户端连接描述符是否有数据可读
		for(i = 1;i < MAXFD_NUM; i++)
		{
			if(FD_ISSET(fd_all[i], &fd_read))
			{
				//fd_all[i] 有数据可读

				memset(dataBuf, 0, BUF_LEN);
				dataLen = recv(fd_all[i],dataBuf, BUF_LEN,0);

				printf("server receive data:%s  recv return:%d\n", dataBuf, dataLen);
				if(dataLen <= 0)
				{  //dataLen =0 表示对方关闭了connSocket,或读取socket出错
					close(fd_all[i]);
					FD_CLR(fd_all[i],&fd_backup);
					fd_all[i] = 0;
					
					break;
				}

				memset(dataBuf, 0, BUF_LEN);
				strcpy(dataBuf, "I am Server");
				dataLen = send(fd_all[i],dataBuf, strlen(dataBuf),0);
			}
		}
		

		
	}
	
	return 0;

}




