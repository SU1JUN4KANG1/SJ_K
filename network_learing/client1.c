/*
 * client.c
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/time.h>
#include <errno.h>

// passing the target address and the data to be sended by parameters
int main(int argc,char *argv[])
{
    int cnsk;
    struct sockaddr_in svraddr;    // socket address struct
    if(argc < 3){
        printf("command format:./client target_server_IP_address data_to_be_sended\n");
        exit(0);
    }
    // creat a socket
    if((cnsk = socket(AF_INET,SOCK_STREAM,0)) == -1){
        fprintf(stderr,"Socket error:%s\n",strerror(errno));
        exit(1);
    }
    bzero(&svraddr,sizeof(struct sockaddr_in)); // clear the socket address struct
    svraddr.sin_family = AF_INET;    // select a protocol family for communication
    svraddr.sin_port = htons(5000);    // select the server port to connect
    inet_pton(AF_INET,argv[1],&svraddr.sin_addr);
    // connect the server
    if(connect(cnsk,(struct sockaddr *)&svraddr,sizeof(struct sockaddr_in)) == -1){
        fprintf(stderr,"connection error:%s\n",strerror(errno));
        exit(1);
    }
    send(cnsk,argv[2],strlen(argv[2]),0); // send data to server
    close(cnsk);                          // close the socket
    return 0;
}