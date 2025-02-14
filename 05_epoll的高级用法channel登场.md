# Day05 epoll高级用法-Channel登场

发现在博客园传大片的项目代码很不优美，于是从Day05开始，知识点补充继续写在这里(干货预警:D)。代码将全部放在我的Github：https://github.com/liyishui2003/30DayMakeCppServer/tree/main

前置知识：

为什么要有channel？事实上 “Channel” 并非 epoll 本身的概念，而是在一些网络库（如 muduo 网络库）中为了更方便地使用 epoll 所抽象出来的一个概念，简单地说它将文件描述符（fd）及其感兴趣的事件和相应的回调函数封装在一起。

为什么要把它们封装在一起？回忆一下，在之前的专题中，我们往epoll里放的都是文件描述符，然后在server.cpp里实现了handleReadEvent()这一函数，用来处理读事件。但一个服务器在现实场景中，肯定能同时提供不同服务(如HTTP/FTP等)，就算文件描述符上发生的事件都是可读事件，不同的连接类型也将决定不同的处理逻辑，仅仅通过一个文件描述符来区分显然会很麻烦。因此我们需要加以区分，同时封装也能方便开发者调用。

那么如何实践？

原本我们用event(也就是下面的epoll_event类).data(下面的epoll_data).fd(epoll_data的成员变量，表示一个文件描述符)来作为事件在红黑树中的标识，任何增删改查都通过这个fd。

```cpp
typedef union epoll_data {
  void *ptr;
  int fd;
  uint32_t u32;
  uint64_t u64;
} epoll_data_t;
struct epoll_event {
  uint32_t events;	/* Epoll events */
  epoll_data_t data;	/* User data variable */
} __EPOLL_PACKED;
```

但注意到这里epoll_data是一个union类型。试想你有一个类有很多变量，但任何时刻只会用到一个，却要为它开辟如此多空间，太奢侈了——于是有了union。union允许不同的数据类型共享同一块内存空间，大小由最大的数据类型决定，同一时刻只能用一个数据类型。

观察到epoll_data里有一个指针ptr，启发我们让ptr指向channel类，就可以通过event.data.ptr来访问channel类，进而在channel类里根据不同的服务类型(HTTP/FTP)调用不同的处理函数。

来看channel的声明：

```cpp
class Channel{
private:
    Epoll *ep;
    int fd;
    uint32_t events;
    uint32_t revents;
    bool inEpoll;
};
```

- 这里记录了Epoll类型的指针ep，是因为Epoll类封装了 epoll 相关操作，如 epoll_create、epoll_ctl等。Channel 对象代表的是一个文件描述符及其感兴趣的事件，当需要将fd注册到 epoll 实例中，或者修改感兴趣的事件时，就需要借助 Epoll 类的方法来完成。虽然我们为了方便处理不同事件抽象出了channel，但当需要调用epoll的各种函数时，仍需要和它联系在一起，在这里通过记录指针来实现。

- fd没什么好说的，肯定要存

- events表示该文件描述符感兴趣的事件，例如 EPOLLIN（可读事件）、EPOLLOUT（可写事件）等，如果又可读又可写，events就等于EPOLLIN | EPOLLOUT。

- revents存储 epoll 实际返回的就绪事件，即当 epoll_wait 检测到该文件描述符上有事件发生时，会将这些事件信息存储在 revents 中。比如可读了，revents就设为EPOLLIN。

- inEpoll 表示该fd是否注册到epoll实例中。

好了，现在看完channel的定义，来看几个实例，彻底搞懂是怎么抽象出来的。

### 情况一：channel的声明
```cpp
Channel *servChannel = new Channel(ep, serv_sock->getFd());
```
这行代码调用了Channel的构造函数，第一个传入指向epoll实例的指针，第二个传入fd，表示该channel链接的是该实例和该fd。

### 情况二：设置channel监听可读事件

首先调用
```cpp
servChannel->enableReading();
```
而channel的enableReading()是这么写的：
```cpp
void Channel::enableReading(){
    events = EPOLLIN | EPOLLET;
    ep->updateChannel(this);
}
```
除了修改events的值外，本质是调用了Epoll类的updateChannel()，再来看它写了什么：
```cpp
void Epoll::updateChannel(Channel *channel){
    int fd = channel->getFd();
    struct epoll_event ev;
    bzero(&ev, sizeof(ev));
    ev.data.ptr = channel;
    ev.events = channel->getEvents();
    if(!channel->getInEpoll()){
        errif(epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) == -1, "epoll add error");
        channel->setInEpoll();
    } 
    else {
        errif(epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev) == -1, "epoll modify error");
    }
}
```
epoll_ctl要求传入一个epoll_event参数（这里fd也作为参数传入了，所以ev里有用的其实是刚才写的events = EPOLLIN | EPOLLET），为了满足这个要求，声明一个ev。接着就是调用函数。发现了吗？印证了我们刚才说“Channel 对象代表的是一个文件描述符及其感兴趣的事件，当需要将fd注册到 epoll 实例中，或者修改感兴趣的事件时，就需要借助 Epoll 类的方法来完成”。

### 情况三：channel返回需要处理的事件

原本写的是
```cpp
std::vector<epoll_event>events = ep->poll();
```
现在都抽象成channel了，就返回channel指针：
```cpp
std::vector<Channel*> activeChannels = ep->poll();
```
poll函数则同样调用了epoll_wait函数，函数返回的是events类型，同理要转化成channel类。这里做了两个事情：① 获取ch指针 ② 把有变化的events赋给ch的Revents(刚才说了，这里存储的是返回的事件)。
```cpp
std::vector<Channel*> Epoll::poll(int timeout){
    std::vector<Channel*> activeChannels;
    int nfds = epoll_wait(epfd, events, MAX_EVENTS, timeout);
    errif(nfds == -1, "epoll wait error");
    for(int i = 0; i < nfds; ++i){
        Channel *ch = (Channel*)events[i].data.ptr;
        ch->setRevents(events[i].events);
        activeChannels.push_back(ch);
    }
    return activeChannels;
}
```

看完这三个例子，这块的更新也就讲完了，耶( •̀ ω •́ )y
