# 08_Tcp也是类

上一节我们分离了用来接受连接的`acceptor`类，并把主要逻辑都放在了`Server`类中(是的，所以我很难理解这点..acceptor就像一个壳子，
或许以后会真的明白为什么需要这个壳子吧)

Tcp作为可靠连接，广泛用在各类服务器中。三次握手新建连接后，这个连接将会一直存在。一个高并发服务器可能
同时拥有成千上万个Tcp连接，为了更方便地管理它们，我们也将Tcp连接抽象成`Connection`类。顺带提一嘴，把一些东西抽象成类实际上就是
面向对象编程，面向对象编程有几个好处：

- 封装。这样和某个对象相关的属性都封装在了一个类中，修改的时候关心类里面的东西即可。
- 复用。封装好了后，可以拿去到处用，还可以加点新东西，通过继承和多态，派生出新的类。
- 安全。类可以用private/protected来控制成员的访问权限，从语言层面实现安全管理。

好，回到正题说`Connection`，这个类也要满足几个特点：

- socket fd就是对应某个客户端连接的socket fd，一个connection对应一个socket fd

- 既然是连接，肯定也要注册到epoll上，所以channel也不必可少。Channel的事件处理函数`handleEvent()`会回调Connection中的事件处理函数`handleReadEvent()`
来相应客户端请求。这里具体怎么处理每个连接的逻辑就没必要写到`Server`类里了，当然也不能写到channel里，所以放在Connection里。

- 同时既然channel在，声明channel需要的EventLoop也在。此外从设计理念的角度来说，EventLoop 是 Reactor 的核心组件，负责事件监听和分发。Connection 类则是处理具体连接的实体。
在 Reactor 模式中，EventLoop 像调度器，将不同的 I/O 事件分发给对应的 Connection 对象进行处理，所以肯定也要关联EventLoop。

由此发现 Connection 类和我们上一章抽象出来的`acceptor`类很像，它们之间十分相似、是平行关系，都直接由`Server`管理、关联了channel和EventLoop、有回调机制。
我们先是用acceptor分离出了接受连接，又用Connection分离出了原本写在`Server`类里的`handleReadEvent()`这一功能。
写到这里，发现越来越贴近Reactor的设计理念了，`Server`类像CPU一样负责调度，至于`acceptor`和`Connection`则都是它的组件。

因此`Server`的声明也有所改动：

```cpp
class Server {
private:
    EventLoop *loop;    //事件循环
    Acceptor *acceptor; //用于接受TCP连接
    std::map<int, Connection*> connections; //所有TCP连接
public:
    Server(EventLoop*);
    ~Server();

    // void handleReadEvent(int);  这行代码原教程里有，但翻来翻去除了这里外没有其它地方再出现了，可能作者忘记删去。
    void newConnection(Socket *sock);   //新建TCP连接
    void deleteConnection(Socket *sock);   //断开TCP连接
};
```
这里用了一个map存储tcp连接，key为fd，value为fd对应的Connection*指针。

此外，由于Connection的生命周期由Server进行管理，所以也应该由Server来建立和删除连接。所以`Server`里有`newConnection`和`deleteConnection`。
如果在Connection业务中需要断开连接操作，也应该和之前一样使用回调函数来实现。说了这么多，来看这俩函数的实现：
```cpp
void Server::newConnection(Socket *sock){
    Connection *conn = new Connection(loop, sock);//new 一个 Connection
    //绑定deleteConnection回调函数，要是有需求，才能在Connection类中删除
    std::function<void(Socket*)> cb = std::bind(&Server::deleteConnection, this, std::placeholders::_1); 
    conn->setDeleteConnectionCallback(cb);
    //存进map里
    connections[sock->getFd()] = conn;
}

void Server::deleteConnection(Socket * sock){
    Connection *conn = connections[sock->getFd()];
    connections.erase(sock->getFd());
    delete conn;
}
```
接着来看Connection声明，刚才分析过了，肯定要有socket，EventLoop，channel，此外就是业务函数echo➕与回调有关的函数。
```cpp
class Connection
{
private:
    EventLoop *loop;
    Socket *sock;
    Channel *channel;
    std::function<void(Socket*)> deleteConnectionCallback;
public:
    Connection(EventLoop *_loop, Socket *_sock);
    ~Connection();
    
    void echo(int sockfd);
    void setDeleteConnectionCallback(std::function<void(Socket*)>);
};
```
函数的实现就没什么好说的了，把之前的搬过来用！

本章小结：
至此，几个类都已经抽象出来了(Server/Connection/Acceptor/EventLoop/Channel)，Reactor事件驱动大体成型。
最需要反复理解的是设计模式：为什么要抽出来封装，为什么要用回调。
