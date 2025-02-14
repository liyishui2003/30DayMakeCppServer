#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "util.h"
void letsEcho(const int clnt_sockfd){
    while(true){
        char buf[1024];
        bzero(&buf,sizeof(buf));
        ssize_t readBytes=read(clnt_sockfd,buf,sizeof(buf));
        /*
        ssize_t用于系统调用函数返回值，表示操作字节数的结果
        read函数从 clnt_sockfd 中读取最多 sizeof(buf) 字节的数据到 buf
        返回值>0为读取字节数/=0关闭连接/<0出错
        */
        if(readBytes>0){
            printf("message from client fd %d: %s\n",clnt_sockfd,buf);
            write(clnt_sockfd,buf,sizeof(buf));
        }
        else if(readBytes==0){
            printf("client fd %d disconnected\n",clnt_sockfd);
            close(clnt_sockfd);//文件描述符理论上有限,要关
            break;
        }
        else if(readBytes==-1){
            close(clnt_sockfd);
            errif(1,"socket read error");
        }
    }
    close(clnt_sockfd);
}
int main() {
	
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  errif(sockfd == -1,"socket create error");
	struct sockaddr_in serv_addr;//结构体 定义在arpa/inet.h里
	bzero(&serv_addr, sizeof(serv_addr));//初始化 填充为0

	serv_addr.sin_family = AF_INET;//IPV4
	serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	/*
	inet_addr把点分十进制的IPV4地址转成网络字节序。
	为什么要转成网络字节序？
	字节序有大端小端之分，比如0x1234在大端字节序里为0x12、0x34
	小端反之，如果不统一显然会出错。
	*/
	serv_addr.sin_port = htons(8888);//htons将16位主机字节序转为网络字节序。
	
  int bindRet=bind(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr));
	/*
	为什么要绑定sockfd & serv_addr？
	把套接字(sockfd)理解为插座，那IP+端口就是插头。
	*/
  errif(bindRet==-1,"bind error");
	int listenRet=listen(sockfd, SOMAXCONN);
	/*
	创建套接字且bind后并不能直接接受客户端连接请求
	但listen一下就能被动接受
	*/
  errif(listenRet==-1,"bind error");
	
  struct sockaddr_in clnt_addr;
	socklen_t clnt_addr_len = sizeof(clnt_addr);
	bzero(&clnt_addr, sizeof(clnt_addr));
	int clnt_sockfd = accept(sockfd, (sockaddr*)&clnt_addr, &clnt_addr_len);
	errif(clnt_sockfd==-1,"accept error");
    
  printf("new client fd %d! IP:%s Port: %d\n", clnt_sockfd, inet_ntoa(clnt_addr.sin_addr), ntohs(clnt_addr.sin_port));
	
  letsEcho(clnt_sockfd);
}
