#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <vector>
#include "util.h"
#include "Epoll.h"
#include "InetAddress.h"
#include "Socket.h"

#define MAX_EVENTS 1024
#define READ_BUFFER 1024

void setNonBlocking(int fd){
    fcntl(fd,F_SETFL,fcntl(fd,F_GETFL)|O_NONBLOCK);
}
void handleReadEvent(int sockfd){
    char buf[READ_BUFFER];
    //《网络是怎样连接的》里提到在做IO操作时
    //如果读到一点就要写入内存，CPU开销很大
    //所以通常会有个缓冲区，读到缓冲区里再memcpy到内存
    while (true)
    {
        bzero(&buf,sizeof(buf));
        ssize_t bytesRead= read(sockfd,buf,sizeof buf);
        if(bytesRead > 0){
            printf("message from client fd %d: %s\n", sockfd, buf);
            write(sockfd, buf, sizeof(buf));
        } else if(bytesRead == -1 && errno == EINTR){  //客户端正常中断、继续读取
            printf("continue reading");
            continue;
        } else if(bytesRead == -1 && ((errno == EAGAIN) || (errno == EWOULDBLOCK))){//非阻塞IO，这个条件表示数据全部读取完毕
            printf("finish reading once, errno: %d\n", errno);
            break;
        } else if(bytesRead == 0){  //EOF，客户端断开连接
            printf("EOF, client fd %d disconnected\n", sockfd);
            close(sockfd);   //关闭socket会自动将文件描述符从epoll树上移除
            break;
        }
    }
    
}

int main(){
    
    Socket *serv_sock = new Socket();
    //调用的 Socket 类的无参构造函数,new表示为对象分配内存空间
    InetAddress *serv_addr = new InetAddress("127.0.0.1",8888);
    serv_sock->bind(serv_addr);
    serv_sock->listen();
    serv_sock->setNonBlocking();
    //调用刚写好的函数，指针访问成员时用的是->,如果是实例就可以用.
    Epoll *ep = new Epoll();
    ep->addFd(serv_sock->getFd(),EPOLLIN|EPOLLET);
    //封装了类后写法好优雅
    while (true)
    {
        std::vector<epoll_event>events = ep->poll();
        int nfds=events.size();
        for(int i=0;i<nfds;i++){
            if(events[i].data.fd==serv_sock->getFd()){
                InetAddress *clnt_addr = new InetAddress();
                Socket *clnt_sock=new Socket(serv_sock->accept(clnt_addr));
                /*
                这里新建了一个套接字，专门用来和该客户端连接。
                serv_sock调用accept后会返回一个fd
                回忆一下socket的两个构造函数：无参构造，或者传入fd进行赋值
                显然这里用了后者
                */
                clnt_sock->setNonBlocking();
                ep->addFd(clnt_sock->getFd(),EPOLLIN | EPOLLET);
            }
            else if(events[i].events & EPOLLIN){
                handleReadEvent(events[i].data.fd);
            }
        }
    }
    
}
