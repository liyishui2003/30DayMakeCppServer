# 07_Acceptor也抽象出来

说实话这章真的挺抽象的。回顾一下服务器处理多个客户端请求的流程：

1.服务器建立套接字，并绑定IP+端口，设置成listen模式接受客户端请求

2.客户端建立套接字，发起连接请求

3.服务器收到请求后，通过accept系统调用，会将请求取出，并_专门_为该连接创建一个新的套接字，也就是这里的clnt_sockfd。而此时最开始建立的套接字会一直打开，继续监听其它请求。

考虑把"服务器建立套接字，并绑定IP+端口，设置成listen模式接受客户端请求"这块抽象出来，封装成一个acceptor类。该类有以下特点：

- 只负责监听请求，如果有新的客户端请求，回去告诉服务器你要干活了，至于服务器后面怎么_专门_为该客户端连接创建一个新的套接字，和我没关系。
"回去告诉服务器你要干活了"可以用上一章学的"回调机制"实现。

- 要高效监听请求，就得丢到epoll里去，所以我的成员变量里要有一个channel，声明channel时又要有EventLoop，所以这些都要有

- 既然我的角色相当于服务器负责监听的套接字，那fd、ip地址肯定也都要有。

由此，可得acceptor的声明为：
```cpp
class Acceptor
{
private:
    EventLoop *loop;
    Socket *sock;
    InetAddress *addr;
    Channel *acceptChannel;
public:
    Acceptor(EventLoop *_loop);
    ~Acceptor();
    void acceptConnection();
    std::function<void(Socket*)> newConnectionCallback;
    void setNewConnectionCallback(std::function<void(Socket*)>);
};
```

接下来看下acceptor.cpp都实现了什么，首先是构造函数：
```cpp
Acceptor::Acceptor(EventLoop *_loop) : loop(_loop)
{
    sock = new Socket();
    addr = new InetAddress("127.0.0.1", 8888);
    sock->bind(addr);
    sock->listen(); 
    sock->setnonblocking();
    acceptChannel = new Channel(loop, sock->getFd());
    std::function<void()> cb = std::bind(&Acceptor::acceptConnection, this);
    acceptChannel->setCallback(cb);
    acceptChannel->enableReading();
}
```
发现这部分的内容和之前Server类的构造函数几乎一模一样：
```cpp
Server::Server(EventLoop *_loop) : loop(_loop){    
    Socket *serv_sock = new Socket();
    InetAddress *serv_addr = new InetAddress("127.0.0.1", 8888);
    serv_sock->bind(serv_addr);
    serv_sock->listen(); 
    serv_sock->setnonblocking();
       
    Channel *servChannel = new Channel(loop, serv_sock->getFd());
    std::function<void()> cb = std::bind(&Server::newConnection, this, serv_sock);
    servChannel->setCallback(cb);
    servChannel->enableReading();
}
```
再一次说明acceptor本质上是对Server类的server socket的封装。

唯一变动的是这里多了一层acceptor，原本的channel直接绑定Server类的newConnection，
这里的channel先绑了acceptor的acceptConnection函数，然后Server初始化时，又指定了acceptor绑定Server类的newConnection：
```cpp
Server::Server(EventLoop *_loop) : loop(_loop), acceptor(nullptr){ 
    acceptor = new Acceptor(loop);
    std::function<void(Socket*)> cb = std::bind(&Server::newConnection, this, std::placeholders::_1);
    acceptor->setNewConnectionCallback(cb);
}
```
所以绑定的东西本质还是没变，只不过中间多了一层acceptor，回调机制从：channel -> server 变成了channel -> acceptor -> server。

我们费尽周折这么做的意义何在？
- 职责分离，模块之间解耦
- 抽象出来的acceptor很通用，服务器可以借此处理多个服务。比如一个acceptor监听80端口，另一个监听25端口，两个之间不会互相阻塞。
- 如果搭配linux的SO_REUSEPORT(它允许多个socket绑定同一个端口)，可以提高并发能力。

后两点是我的个人判断，至于到底为什么要抽象出acceptor，在原教程和陈硕的《Linux多线程服务端编程》里都没找到很满意的答案。
