

/*
 基于TCP 实现 电子在线词典客户端
 
*/


#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>



//定义消息类型结构体
struct  dict_msg
{
	char msg_type;  //消息类型
	char name[50];  //user name
	char data[256]; //password word and word explain
};

/*
msg_type:

'R'  --- 注册
'L'  --- 登录
'H'  --- 查看历史记录
'Q'  --- 查找单词

*/

typedef struct  dict_msg  dict_msg_t;

dict_msg_t recvDictMsg; //保存接收到的服务器消息
dict_msg_t sendDictMsg; //保存发送给服务器的消息



//函数声明
void do_register(void);
void show_dict_menu(void);
int sendDictMsgToServer(dict_msg_t *msg);
int recvDictMsgFromServer(dict_msg_t *msg);

int  do_login(void);
void show_search_menu(void);
void do_query(void);
void do_history(void);



//.dictcli  ServerIP  Port


int clientSocket = 0;

char serverIP[50] = {0};
unsigned short serverPort = 0;


int main(int argc, char *argv[])
{
 	int iret = 0;
 	char user_choser = 0;
 	char user_sub_choser = 0;

	if(argc != 3)
	{
		printf("parameter number error\n");
		printf("usage: ./dictcli serverIP port\n");

		return 0;
	}

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

	iret = connect(clientSocket, (struct sockaddr *)&serverAddr, addr_len);

	if(iret < 0)
	{
		printf("connect error\n");
		return 0;
	}

	printf("connect server ok\n");


	int login_res = 0;
	
	while(1)
	{

		show_dict_menu();
		user_choser = getchar();
		while(getchar()!='\n'); //清空输入缓冲区

		switch(user_choser)
		{
			case '1':
				do_register();
				break;
				
			case '2':
				login_res = do_login();
				if(login_res == 1)
				{
				 //登录成功，开始进入查询单词界面
				 	show_search_menu();
				 	user_sub_choser = getchar();
				 	while(getchar()!='\n'); //清空输入缓冲区
				 	if(user_sub_choser == '1')
				 	{
				 	    //查找单词
				 	    printf("start find word\n");
				 	    do_query();
				 	    break;
				 	}
				 	else if(user_sub_choser == '2')
				 	{
				 		//查看历史记录
				 		printf("start find history\n");
				 		do_history();
				 		break;
				 	}
				 	else
				 	{
				 		break; //返回上级菜单
				 	}
				 	
				}
				else
				{
					break;
				}
				
			case '3':
				return 0;
				
			default:
				printf("error input");
				break;
		}
	}
	

	return 0;

}


void do_history(void)
{

	memset(&sendDictMsg,0,sizeof(dict_msg_t));

	sendDictMsg.msg_type = 'H';
	sendDictMsgToServer(&sendDictMsg); //发送单词查询消息到服务器

	printf("client enter do history!");
	while(1)
	{
		memset(&recvDictMsg,0,sizeof(dict_msg_t));
		//接收历史记录
		recvDictMsgFromServer(&recvDictMsg);

		printf("%s\n", recvDictMsg.data);

		if(recvDictMsg.data[0] == '\0') break;
		
	}

	
}


void do_query(void)
{
	while(1)
	{
	    memset(&recvDictMsg,0,sizeof(dict_msg_t));
		memset(&sendDictMsg,0,sizeof(dict_msg_t));

		sendDictMsg.msg_type = 'Q';

		printf("input word:");
		scanf("%s",sendDictMsg.data);

		if(strcmp(sendDictMsg.data,"#") == 0) break;

		sendDictMsgToServer(&sendDictMsg); //发送单词查询消息到服务器

		usleep(5000); //延时 5 ms
		recvDictMsgFromServer(&recvDictMsg);
		printf("%s\n", recvDictMsg.data);
		putchar('\n');
			
	}
}


void show_dict_menu(void)
{
	printf("*******************************************\n");
	printf("1.register*********************************\n");
	printf("2.login*********************************\n");
	printf("3.quit*********************************\n");
    printf("*******************************************\n");
	
}


void show_search_menu(void)
{
	printf("*******************************************\n");
	printf("1.find word*********************************\n");
	printf("2.history*********************************\n");
	printf("3.return*********************************\n");
    printf("*******************************************\n");
}


//发送消息到客户端
int sendDictMsgToServer(dict_msg_t *msg)
{
	int slen = 0;

	int msgLen = sizeof(dict_msg_t);

	slen = send(clientSocket, (unsigned char *)msg, msgLen,0);

	if(slen != msgLen)
	{
		printf("send dict_msg to server error\n");

		return -1;
	}

	return 0;
}


int recvDictMsgFromServer(dict_msg_t *msg)
{
	 int len = 0;
	len  = recv(clientSocket,msg,sizeof(dict_msg_t),0);

    printf("recv len:%d  size:%d\n",len,(int)sizeof(dict_msg_t));

	if(len != sizeof(dict_msg_t))
	{
		printf("recv error\n");
		exit(0);
	}

	return 0;
}

//实现登录功能,登录成功返回 1，失败返回 0
int do_login(void)
{
    memset(&recvDictMsg,0,sizeof(dict_msg_t));
	memset(&sendDictMsg,0,sizeof(dict_msg_t));

	printf("input user name:");
	scanf("%s",sendDictMsg.name);
	printf("input password: ");
	scanf("%s",sendDictMsg.data);

	while(getchar()!='\n'); //清空输入缓冲区
	sendDictMsg.msg_type = 'L'; //构建注册用户消息

	sendDictMsgToServer(&sendDictMsg); //发送注册消息到服务器

	usleep(5000); //延时 5 ms
	recvDictMsgFromServer(&recvDictMsg);

	if(strcmp(recvDictMsg.data,"OK") == 0)
	{
		printf("login success!\n");
		return  1;
	}

	printf("login fail!\n");

	return 0;
}


//实现注册功能
void do_register(void)
{
	memset(&recvDictMsg,0,sizeof(dict_msg_t));
	memset(&sendDictMsg,0,sizeof(dict_msg_t));

	printf("input user name:");
	scanf("%s",sendDictMsg.name);
	printf("input password: ");
	scanf("%s",sendDictMsg.data);
	while(getchar()!='\n'); //清空输入缓冲区
	
	sendDictMsg.msg_type = 'R'; //构建注册用户消息

	sendDictMsgToServer(&sendDictMsg); //发送注册消息到服务器

	usleep(5000); //延时 5 ms
	recvDictMsgFromServer(&recvDictMsg);

	if(strcmp(recvDictMsg.data,"OK") == 0)
	{
		printf("register is OK!\n");
		return ;
	}

	printf("register fail!\n");
	
}



