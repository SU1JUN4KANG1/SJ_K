
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
#include <sys/wait.h>

#include <string.h>
#include <fcntl.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>



/*
./udps serverIP  port
*/

#define BUF_LEN  128


char serverIP[50] = {0};
unsigned short serverPort = 0;
int serverSocket = 0; 

void sig_handler(int signo)
{
	printf("catch alrm signal\n");
}

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
	serverSocket = socket(AF_INET,SOCK_DGRAM,0);
	if(serverSocket == 0)
	{
		printf("create UDP socket error\n");
		return 0;
	}


	#if 0
	//设置serverSocket为非阻塞工作方式
	int flag = 0;
	flag = fcntl(serverSocket,F_GETFL,0);
	flag |= O_NONBLOCK;
	iret = fcntl(serverSocket,F_SETFL,flag);
	printf("fcntl return:%d\n",iret);
	#endif
	

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

	//接收UDP客户端请求

	int dataLen = 0;

	char dataBuf[BUF_LEN];

	char clientIP[50] = {0};
	unsigned short clientport = 0;

	struct sockaddr_in clientAddr;
	memset(&clientAddr,0, sizeof(struct sockaddr_in));


	#if 0

	//设置socket超时检测选项 
	struct timeval timeout;
	int optlen = sizeof(struct timeval);

	timeout.tv_sec = 2;
	timeout.tv_usec = 0;

	iret = setsockopt(serverSocket, SOL_SOCKET, SO_RCVTIMEO, &timeout,optlen);
	

	//select 函数实现超时检测
	struct timeval timeout;
	int maxfd = serverSocket;

	fd_set fd_read;

	#endif


	#if 0

	//使用信号检测网络超时机制
	struct sigaction action;

	sigaction(SIGALRM,NULL,&action); //获取默认的信号处理结构体
	action.sa_handler =  sig_handler; //忽略信号

	//默认重新启动系统调用
	action.sa_flags &= ~SA_RESTART; //清除重新启动系统调用标志
	sigaction(SIGALRM, &action, NULL);
	
	#endif
	

	while(1)
	{

	   #if 0
	   timeout.tv_sec = 2;
	   timeout.tv_usec = 0;
	   FD_ZERO(&fd_read);
	   FD_SET(serverSocket,&fd_read);

	   iret = select(maxfd+1,&fd_read,NULL,NULL, &timeout);
	   if(iret == 0)
	   {
	   		printf("udp select time out\n");
	   		continue;
	   }

	   #endif
	

	   memset(dataBuf, 0,BUF_LEN );



	  // alarm(2);
	   dataLen = recvfrom(serverSocket,dataBuf, BUF_LEN,0,
	                      (struct sockaddr *)&clientAddr, &addr_len);

	  /*
	   if(errno == EINTR)
	   {
	   		printf("time out errcode:%d\n", errno);
	   }
	   */

	   clientport = ntohs(clientAddr.sin_port);

	   strcpy(clientIP, inet_ntoa(clientAddr.sin_addr));

	   printf("recv len:%d  data: %s from ip  %s:%d\n",dataLen,dataBuf,clientIP, clientport);

     #if 0
	   memset(dataBuf, 0,BUF_LEN );
	   strcpy(dataBuf, "I am udp server");
	   addr_len = sizeof(struct sockaddr_in);
	   dataLen = sendto(serverSocket,dataBuf, strlen(dataBuf),0,
	                      (struct sockaddr *)&clientAddr, addr_len);

	   printf("send len:%d\n",dataLen );
	   
	 #endif

	 //sleep(1);
  
	}
}


