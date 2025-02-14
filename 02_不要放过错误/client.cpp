#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include "util.h"
void letsEcho(const int sockfd){
    while (1){
        char buf[1024];
        bzero(&buf,sizeof(buf));
        scanf("%s",buf);
        ssize_t writeBytes=write(sockfd,buf,sizeof(buf));
        if(writeBytes==-1){
            printf("socket disconnected,can't write.");
            break;
        }
        bzero(&buf,sizeof(buf));
        ssize_t readBytes=read(sockfd,buf,sizeof(buf));
        if(readBytes>0){
            printf("message from server:%s\n",buf);
        }
        else if(readBytes==0){
            printf("server disconnected\n");
            break;
        }
        else if(readBytes==-1){
            close(sockfd);
            errif(1,"socket read error");
        }
    }
    close(sockfd);
}
int main() {
	
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    errif(sockfd == -1,"socket create error");
	
    struct sockaddr_in serv_addr;
  	bzero(&serv_addr, sizeof(serv_addr));
  	serv_addr.sin_family = AF_INET;
  	serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  	serv_addr.sin_port = htons(8080);
	
    int bindRet=bind(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr));
	  errif(bindRet == -1,"bind error");
    
    int connnectRet=connect(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr));
    errif(connnectRet == -1,"connect error");

    letsEcho(sockfd);
}
