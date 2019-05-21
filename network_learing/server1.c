/*
 * server.c
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

int main(void)
{
    int listsk,cnsk;
    char rbuf[51];                             // the buffer for receveing data
    struct sockaddr_in svraddr;                 // socket address struct
    listsk = socket(AF_INET,SOCK_STREAM,0);  // creat a socket to listen
    bzero(&svraddr,sizeof(struct sockaddr)); // clear the socket address struct
    svraddr.sin_family = AF_INET;             // select a protocol family for communication
    svraddr.sin_addr.s_addr = htonl(INADDR_ANY); // select receving any connection
    svraddr.sin_port = htons(5000);             // select the port to listen
    // bind a address for socket
    if(bind(listsk,(struct sockaddr *)&svraddr,sizeof(struct sockaddr_in))== -1){
        fprintf(stderr,"Bind error:%s\n",strerror(errno));
        exit(1);
    }
    if(listen(listsk,1024) == -1){    // begin to listen
        fprintf(stderr,"Listen error:%s\n",strerror(errno));
        exit(1);
    }
    //creat the socket for communication,and wait the connection from client
    if((cnsk = accept(listsk,(struct sockaddr *)NULL,NULL)) == -1){
        fprintf(stderr,"accept error:%s\n",strerror(errno));
        exit(1);
    }
    memset(rbuf,0,51);        // reset the buffer to 0
    recv(cnsk,rbuf,50,0);    // receve the data from client
    printf("%s\n",rbuf);    // print the data had receved
    sleep(3);
    close(cnsk);            // close the socket for communication
    close(listsk);            // close the socket for listening
}