

/*

基于TCP 实现 电子在线词典服务器

*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>  
#include <unistd.h>
#include <time.h>


#include <string.h>

#include "sqlite3.h"



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


dict_msg_t recvDictMsg; //保存接收到的客户端消息
dict_msg_t sendDictMsg; //保存发送给客户端的消息

char serverIP[50] = {0};  //服务器IP
unsigned short serverPort = 0;  //服务器端口号

int serverSocket = 0; //服务socket
int connSocket = 0; //连接socket

sqlite3 *myDictDb = NULL; //数据库句柄指针

char loginName[50]={0};


//函数声明
int search_word(void);
int user_register(void);
int user_login(void);
int search_word(void);
int get_history(void);
int sendDictMsgToClient(dict_msg_t *msg);
void insert_history(char *name,char *word);




int main(int argc, char *argv[])
{

	int iret = 0;

	int len = 0;

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

	memset(&recvDictMsg,0, sizeof(dict_msg_t));
	memset(&sendDictMsg,0, sizeof(dict_msg_t));

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


	//打开数据库
	iret = sqlite3_open("myDict.db", &myDictDb);
	if(iret != 0)
	{
		printf("open myDict.db error\n");

		return 0;
	}

	

	struct sockaddr_in clientAddr;
	memset(&clientAddr,0, sizeof(struct sockaddr_in));
	

	char clientIP[50] = {0};
	unsigned short clientPort = 0;
	int cli_pid = 0;

	char msg_type = 0;

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

		 //创建子进程，用来处理客户端发送过来注册，登录，查询消息
		cli_pid = fork();
		if(cli_pid == 0)
		{
			//子进程处理客户端发送过来的注册，登录，查看历史记录，查找单词消息
			close(serverSocket); //关闭监听socket

			//接收客户端消息
			while(1)
			{
				len  = recv(connSocket,&recvDictMsg,sizeof(dict_msg_t),0);

				printf("recv len:%d\n",len);
				if(len != sizeof(dict_msg_t))
				{
					printf("recv error\n");
					exit(0);
				}

				/*
				msg_type:

				'R'  --- 注册
				'L'  --- 登录
				'H'  --- 查看历史记录
				'Q'  --- 查找单词

				*/

				msg_type = recvDictMsg.msg_type;

				printf("%c\n",recvDictMsg.msg_type);
			    printf("%s\n",recvDictMsg.name);
			    printf("%s\n",recvDictMsg.data);

				

				switch(msg_type)
				{
					case 'R': //用户注册功能
						user_register();
						break;
						
					case 'L': //用户登录
						user_login();
						break;

					case 'H':
						get_history(); //查看历史记录
						break;
						
					case 'Q':
						search_word(); //查找单词
						break;
						
					default:
						break;	
				}
			}
			
			sleep(10);
			exit(0);
			
		}

		//父进程，关闭连接socket,继续处理客户端连接建立请求
		sqlite3_close(myDictDb); 
		close(connSocket);	
	}

	return 0;
}


/*
	数据库说明,数据库名字 myDict.db

	注册用户信息表 userinfo: name(TEXT), password(TEXT)
	查询历史记录 hisinfo： name(TEXT)          date(TEXT)   word(TEXT)


*/


void get_date(char *date)
{
	time_t t;
	struct tm *tp;

	time(&t);
	
	tp = localtime(&t);
	/*
	sprintf(date, "%d-%02d-%02d %02d:%02d:%02d", tp->tm_year+1900, 
			tp->tm_mon+1, tp->tm_mday, tp->tm_hour, tp->tm_min, tp->tm_sec);
	*/
	strftime(date, 64, "%Y-%m-%d %H:%M:%S", tp);

	return;
}


void insert_history(char *name,char *word)
{
	char sqlCmd[80] = {0};
	int iret = 0;
	char *errmsg = NULL;

	char datetime[100]={0};

	get_date(datetime);

	sprintf(sqlCmd,"insert into hisinfo values(\"%s\",\"%s\",\"%s\")",name,word,datetime);

	iret = sqlite3_exec(myDictDb,sqlCmd,NULL,NULL,&errmsg);

	if(iret != SQLITE_OK)
	{
		printf("sqlite3 exec error\n");
		return ;
	}	

}


//实现用户注册功能
int  user_register(void)
{
	//打开数据库
	char sqlCmd[80] = {0};
	int iret = 0;
	char *errmsg = NULL;

	memset(&sendDictMsg,0,sizeof(dict_msg_t));
	sendDictMsg.msg_type = 'R';

	//插入用户信息
	printf("enter user_register\n");

	sprintf(sqlCmd,"insert into userinfo values(\"%s\",\"%s\")",recvDictMsg.name,recvDictMsg.data);

	iret = sqlite3_exec(myDictDb,sqlCmd,NULL,NULL,&errmsg);

		printf("enter user_register1111111\n");

	if(iret != SQLITE_OK)
	{
		printf("sqlite3 exec error\n");
		
		strcpy(sendDictMsg.data,"FAIL");
		sendDictMsgToClient(&sendDictMsg);

		return -1;
	
	}


	//给客户端发送注册成功消息
	strcpy(sendDictMsg.data,"OK");
	printf("send ok message\n");
	sendDictMsgToClient(&sendDictMsg);
	
	return 0;
	
}


//发送消息到客户端
int sendDictMsgToClient(dict_msg_t *msg)
{
	int slen = 0;

	int msgLen = sizeof(dict_msg_t);

	printf("start send msg socket:%d\n", connSocket);
	slen = send(connSocket, (unsigned char *)msg, msgLen,0);

	if(slen != msgLen)
	{
		printf("send dict_msg error\n");

		return -1;
	}

	return 0;
}


//实现用户登录功能,登录成功返回 0，失败返回 -1
int user_login(void)
{
	char sqlCmd[80] = {0};
	int iret = 0;
	char *errmsg = NULL;
	char **resultp = NULL;
	int row = 0;
	int colum = 0;

	memset(&sendDictMsg,0,sizeof(dict_msg_t));
	sendDictMsg.msg_type = 'L';

	sprintf(sqlCmd,"select * from userinfo where name = \"%s\"", recvDictMsg.name);

	iret = sqlite3_get_table(myDictDb,sqlCmd, &resultp,&row,&colum,&errmsg);

	if(iret != SQLITE_OK)
	{
	   //查找用户出错
	   printf("sqlite3_get_table error\n");
	   return -1;
	}

	if(row == 0)
	{
		//没有找到该用户
		printf("%s is not exist!\n",recvDictMsg.name);
		strcpy(sendDictMsg.data, "FAIL");
		sendDictMsgToClient(&sendDictMsg);

		return -1;
		
	}

	//在数据库中查找对应用户密码
	if(strcmp(recvDictMsg.data, resultp[3]) != 0)
	{
		printf("password is error\n");
		strcpy(sendDictMsg.data, "FAIL");
		sendDictMsgToClient(&sendDictMsg);

		return -1;
	}

	memset(loginName,0,50);
	strcpy(loginName,recvDictMsg.name);

	//登录成功
	
	strcpy(sendDictMsg.data, "OK");
	sendDictMsgToClient(&sendDictMsg);
	
	return 0;
	
}


//sql 回调函数
int  hiscall_back(void *para, int f_num, char**f_value, char **f_name)
{
	printf("enter call back\n");
	memset(&sendDictMsg,0,sizeof(dict_msg_t));
	sendDictMsg.msg_type = 'H'; //发送给客户端的历史记录
	
	sprintf(sendDictMsg.data, "%s: %s", f_value[1],f_value[2]);

	sendDictMsgToClient(&sendDictMsg); //发送历史记录给客户端

	return 0;
}

//查看历史记录功能
int get_history(void)
{
	char sqlCmd[80] = {0};
	int iret = 0;
	char *errmsg = NULL;

	printf("server enter get history!\n");

	sprintf(sqlCmd, "select * from hisinfo where name=\"%s\"",loginName);

	printf("%s\n", sqlCmd);

	iret = sqlite3_exec(myDictDb,sqlCmd,hiscall_back,NULL,&errmsg );

	if(iret != SQLITE_OK)
	{
		printf("sqlite3_exec error\n");
		return -1;
	}

	//发送查询历史记录结束标志

	printf("send history end flags!\n");
	memset(&sendDictMsg,0,sizeof(dict_msg_t));
	sendDictMsg.msg_type = 'H'; 
	sendDictMsgToClient(&sendDictMsg);

	return 0;
	
}


//查找单词功能
int search_word(void)
{

	FILE *fpDict = NULL;
	char *prb = NULL;

	char *p=NULL;

	int wordlen = 0;

	char lineBuf[300] = {0};
	
	memset(&sendDictMsg,0,sizeof(dict_msg_t));
	sendDictMsg.msg_type = 'Q'; 

	fpDict = fopen("dict.txt","rb");
	if(fpDict == NULL)
	{
		printf("can find dict.txt file\n");
		return -1;
	}

	//插入查找记录
	insert_history(loginName, recvDictMsg.data);


	do
	{
		memset(lineBuf, 0,300);
		prb = fgets(lineBuf,300,fpDict);
		if(prb == NULL)
		{
			break; //读到文件末尾跳出循环
		}

		wordlen = strlen(recvDictMsg.data);

		if( strncmp(recvDictMsg.data,lineBuf, wordlen) == 0)
		{
			//找到了该单词,COPY 单词解释到发送数据报文
			p = lineBuf+wordlen;
			while((*p) == ' ')
			{
				p++;
			}

			strcpy(sendDictMsg.data,p); //复制单词解释到发送数据报文
			sendDictMsgToClient(&sendDictMsg);

			break;
		}
		//该单词不是要找的单词，继续读取下一行		
	}while(1);

	if(prb == NULL)
	{
		//没有找到该单词
		strcpy(sendDictMsg.data,"can not find the word"); 
		sendDictMsgToClient(&sendDictMsg);
		
		return -1;
	}

	return 0;
}



