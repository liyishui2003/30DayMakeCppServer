#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include "util.h"
#define MAX_EVENTS 1024
#define READ_BUFFER 1024

void setNonBlocking(const int fd){
    fcntl(fd,F_SETFL,fcntl(fd,F_GETFL) | O_NONBLOCK);
    /*
    F_GETFL 是fcntl的一个命令，获取 fd 的状态标志。
    这些标志描述了例如是否为阻塞模式、是否为追加模式等。
    O_NONBLOCK表示非阻塞，用 | 表示在fd的状态里加一个非阻塞
    */
}

int main() {
	
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    errif(sockfd == -1,"socket create error");
	struct sockaddr_in serv_addr;
	bzero(&serv_addr, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;//IPV4
	serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	serv_addr.sin_port = htons(8888);//htons将16位主机字节序转为网络字节序。
	
    int bindRet=bind(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr));
    errif(bindRet==-1,"bind error");
	int listenRet=listen(sockfd, SOMAXCONN);
    errif(listenRet==-1,"bind error");
	
    int epfd=epoll_create1(0);//创建epoll实例，用来同时监视多个IO事件
    errif(epfd==-1,"epoll create error");
    
    
    struct epoll_event events[MAX_EVENTS],ev;
    bzero(&events,sizeof(events));
    
    bzero(&ev,sizeof(ev));
    ev.events=EPOLLIN | EPOLLET;
    //原本只有EPOLLIN，多一个EPOLLET表示采用et模式
    ev.data.fd=sockfd;
    setNonBlocking(sockfd);
    epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev);
    //向 epoll 实例(epfd)中添加一个需要监控的文件描述符(sockfd)
    //及其关注的事件(&ev)
    

   while(true){
        int nfds = epoll_wait(epfd,events,MAX_EVENTS,-1);
        //events里有nfds个待处理事件,包括服务器和客户端
        
        for(int i=0;i<nfds;++i){
            if(events[i].data.fd==sockfd){
                
                struct sockaddr_in clnt_addr;
                bzero(&clnt_addr, sizeof(clnt_addr));
                socklen_t clnt_addr_len = sizeof(clnt_addr);
                
                
                int clnt_sockfd = accept(sockfd, (sockaddr*)&clnt_addr, &clnt_addr_len);
                //调用 accept 接受新连接，从而得到新的客户端套接字 clnt_sockfd
                errif(clnt_sockfd==-1,"accept error");

                bzero(&ev,sizeof(ev));
                ev.data.fd=clnt_sockfd;//同理，关联来自客户端的套接字和ev
                ev.events=EPOLLIN | EPOLLET;//设置模式为ET+关注读入事件
                setNonBlocking(clnt_sockfd);
                epoll_ctl(epfd,EPOLL_CTL_ADD,clnt_sockfd,&ev);
            }
            else if(events[i].events & EPOLLIN){
                char buf[READ_BUFFER];
                while(true){
                    bzero(&buf,sizeof(buf));
                    ssize_t bytesRead=read(events[i].data.fd,buf,sizeof(buf));
                    if(bytesRead > 0){
                        printf("message from client fd %d: %s\n",events[i].data.fd,buf);
                        write(events[i].data.fd,buf,sizeof(buf));
                    }
                    else if(bytesRead==-1&&errno==EINTR){
                        //errno在<errno.h>里，有一系列预定义的值
                        //比如EACCES/EBADF/EINTR/ENOMEM
                        //此处EINTR表示正常中断
                        printf("continue reading\n");
                        continue;
                    }
                    else if(bytesRead==-1&&( (errno==EAGAIN || errno==EWOULDBLOCK))){
                        //这两个错误码都表示资源暂时不可用
                        printf("finish reading once,errno: %d\n",errno);
                    }
                    else if(bytesRead==0){//EOF，客户端断开连接
                        printf("EOF,client fd %d disconnected\n",events[i].data.fd);
                        close(events[i].data.fd);
                        //关闭socket会自动将文件描述符从epoll树上移除
                    }
                }
            }
        }
   }
}
