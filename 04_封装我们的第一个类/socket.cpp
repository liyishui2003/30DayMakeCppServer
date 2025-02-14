#include "Socket.h"
#include "InetAddress.h"
#include "util.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>

/*
第一个构造函数用于创建一个新的套接字
第二个构造函数用于复用已有的套接字文件描述符
*/
Socket::Socket() : fd(-1){
    fd = socket(AF_INET,SOCK_STREAM,0);
    errif(fd==-1,"socket create error");
}

Socket::Socket(int _fd) : fd(_fd){
    errif(fd==-1,"socket create error");
}

Socket::~Socket(){
    if(fd!=-1){
        close(fd);
        fd=-1;
    }
}

void Socket::bind(InetAddress *addr){
    /*
    函数前面只放 : : 表示调用的是全局作用域中的bind
    这么写是因为Socket类中也有一个bind函数
    如果没写 : : 不就变成递归调用了吗
    */
    errif(::bind(fd,(sockaddr*)&addr->addr,addr->addr_len)==-1,
    "socket bind error");
}

void Socket::listen(){
    errif(::listen(fd,SOMAXCONN)==-1,"socket listen error");
}

void Socket::setNonBlocking(){
    fcntl(fd,F_SETFL,fcntl(fd,F_GETFL) | O_NONBLOCK);
}

int Socket::accept(InetAddress *addr){
    int clnt_sockfd = :: accept(fd,(sockaddr*)&addr->addr,&addr->addr_len);
    /*
注意区分一下：
clnt_sockfd是服务器为接受连接请求(accept)新建的套接字
和刚才最开始建立的用来绑定IP+端口的套接字是不同的。

服务器和客户端建立通信的过程为：
1.服务器建立套接字，并绑定IP+端口，设置成listen模式接受客户端请求
2.客户端建立套接字，发起连接请求
3.服务器收到请求后，通过accept系统调用，会将请求取出，并_专门_为
该连接创建一个新的套接字，也就是这里的clnt_sockfd。而此时最开始建立
的套接字会一直打开，继续监听其它请求。

注：套接字只在计算机内部有用，用来标识到底是哪个连接；
也就是说客户端和服务器的套接字是各用各的，互不关心；
那它们怎么通信？通过暴露在外的端口，通过IP+端口就能确定
我要联系上的是哪个套接字。

上述内容参考《网络是怎样连接的》，非常好的一本外国教材。
    */
   errif(clnt_sockfd==-1,"socket accept error");
   return clnt_sockfd;
}

int Socket::getFd(){
    return fd;
}
