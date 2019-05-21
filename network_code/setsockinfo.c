

/*

   socket属性相关函数使用

   TCP 客户端程序
   
*/



#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>




#define BUF_LEN   128



//.tcpclient  ServerIP  Port


int clientSocket = 0;

char serverIP[50] = {0};
unsigned short serverPort = 0;


void show_sockaddr(struct sockaddr_in *addr)
{
	unsigned short portNo = 0;
	char IPAddr[50]={0};
	
	portNo = ntohs(addr->sin_port);

	strcpy(IPAddr, inet_ntoa(addr->sin_addr));

	printf("ipaddr: %s  port:%d\n",IPAddr, portNo);
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


   //gethostname :获取主机名称

	char  hostName[50]={0};

	iret = gethostname(hostName, 50);

	printf("iret:%d  hostname:%s\n", iret,hostName);
	

	strcpy(serverIP, argv[1]);
	serverPort = atoi(argv[2]);

	//创建socket
	clientSocket = socket(AF_INET,SOCK_STREAM,0);
	if(clientSocket == 0)
	{
		printf("client create socket error\n");
		return 0;
	}


	struct sockaddr_in serverAddr;
	int addr_len = sizeof(struct sockaddr_in);
	memset(&serverAddr,0, sizeof(struct sockaddr_in));

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(serverPort);
	serverAddr.sin_addr.s_addr = inet_addr(serverIP);


	struct sockaddr_in peerAddr;
	iret = getsockname(clientSocket, (struct sockaddr *)&peerAddr, &addr_len);
	show_sockaddr(&peerAddr);
	

	iret = connect(clientSocket, (struct sockaddr *)&serverAddr, addr_len);

	if(iret < 0)
	{
		printf("connect error\n");
		return 0;
	}

	printf("connect server ok\n");

    //getsockname, 获取与套接字相关的本机协议地址
	struct sockaddr_in  localAddr;

	iret = getsockname(clientSocket, (struct sockaddr *)&localAddr, &addr_len);

	show_sockaddr(&localAddr);


	iret = getpeername(clientSocket, (struct sockaddr *)&peerAddr, &addr_len);
	show_sockaddr(&peerAddr);


/*
  gethostbyname 根据域名或主机名获取主机地址结构相关信息

  struct hostent * gethostbyname(const char * hostname);   //返回：若成功则为非空指针，若出错则为NULL且设置h_errno

  struct hostent
  {
	char 	*h_name; 		//official (canonical ) name of host
    char 	**h_aliases;	 //pointer to array of pointers to alias names
    int		 h_addrtype; 	//host address type:AF_INET
    int		 h_length; 		//length of address: 4
    char 	**h_addr_list; 	//ptr to array of ptrs with IPv4 addrs
  };

*/


	struct hostent *phost=NULL;
	 
	phost = gethostbyname("www.hqyj.com");

	printf("official name: %s  addr_type:%d  addr_len:%d\n", 
			phost->h_name, phost->h_addrtype, phost->h_length);

	//输出所有主机别名
	int i = 0;
	printf("host aliases:\n");
	while(phost->h_aliases[i])
	{
		printf("%s\n",phost->h_aliases[i]);
		i++;
	}

	//输出地址列表
	i = 0;
	printf("host ipaddr:\n");

	char hostIP[50] ={0};
	while(phost->h_addr_list[i])
	{
		 memset(hostIP,0,50);
		 inet_ntop(AF_INET, (void *)phost->h_addr_list[i], hostIP, 16);
		 printf("ipaddr:%s\n", hostIP);
		i++;
	}


	int reuse = 0;
	int optlen = 0;
	
	//获取重用本地地址和端口号选项
	iret = getsockopt(clientSocket,SOL_SOCKET,SO_REUSEADDR,&reuse,&optlen);

	printf("optval:%d  optlen:%d\n",reuse, optlen);

	//设置重用本地地址和端口号选项
	optlen=sizeof(int);
	reuse = 1;
	iret = setsockopt(clientSocket,SOL_SOCKET,SO_REUSEADDR,&reuse,optlen);

	printf("after setoptsock\n");

	iret = getsockopt(clientSocket,SOL_SOCKET,SO_REUSEADDR,&reuse,&optlen);
	printf("optval:%d  optlen:%d\n",reuse, optlen);




	int dataLen = 0;
	char dataBuf[BUF_LEN];

	//客户端连接成功，开始发送和接收数据,服务器先接收数据
	while(1)
	{

		memset(dataBuf, 0, BUF_LEN);
		strcpy(dataBuf, "I am client");
		dataLen = send(clientSocket,dataBuf, strlen(dataBuf),0);

		printf("send return :%d\n", dataLen);

	
		memset(dataBuf, 0, BUF_LEN);
		dataLen = recv(clientSocket,dataBuf, BUF_LEN,0);

		printf("client receive data:%s  recv return:%d\n", dataBuf, dataLen);
		
		sleep(2);
	   
	}
	
}




