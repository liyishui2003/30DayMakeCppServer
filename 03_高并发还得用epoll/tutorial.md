## 前置知识：
- **IO复用** ：所有的服务器都是高并发的，可以同时为成千上万个客户端提供服务，这一技术称为IO复用。IO复用和多线程有相似之处，但本质不同。IO复用针对IO接口，多线程针对CPU。linux中使用select, poll和epoll来实现该技术。

- **epoll**：由epoll_create(创建)、epoll_ctl(支持增/删/改) 和 epoll_wait(等待)三个系统调用组成。
```cpp
int epfd = epoll_create1(0);       //参数是一个flag，一般设为0，详细参考man epoll

epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev);    //添加事件到epoll，此处ev是一个epoll_event结构体，存有events和data
epoll_ctl(epfd, EPOLL_CTL_MOD, sockfd, &ev);    //修改epoll红黑树上的事件
epoll_ctl(epfd, EPOLL_CTL_DEL, sockfd, NULL);   //删除事件

int nfds = epoll_wait(epfd, events, maxevents, timeout);
/*
events是一个epoll_event结构体数组，maxevents是可供返回的最大事件大小
一般为events的大小，timeout表示最大等待时间，设置为-1表示一直等待。
*/
```

- **LT和ET**：epoll 有两种工作模式：
水平触发（LT，Level Triggered）：默认工作模式。当被监控的文件描述符上有事件发生时，epoll_wait 通知程序。如果程序在本次处理中没有将数据全部读取或写入，epoll_wait 在下一次调用时仍然会通知该事件，直到数据被处理完。
边缘触发（ET，Edge Triggered）：更高效。只有文件描述符的 I/O 事件状态发生变化时（不可读变可读，从不可写变可写），epoll_wait 才通知程序。因此程序要在一次处理中尽可能处理数据，否则可能错过后续数据。好东西都有代价，ET考验代码功底，且必须搭配非阻塞socket使用。

- **阻塞/非阻塞套接字**：阻塞套接字进行操作时，如果操作条件不满足，程序被阻塞，直到操作完成或发生错误。非阻塞套接字则不会阻塞程序执行，会立即返回错误码（通常是 EWOULDBLOCK 或 EAGAIN），程序继续执行其他任务。

Q：为什么ET模式一定要搭配非阻塞套接字？

A：
```
阻塞套接字读数据时若数据量较大，一次 read 操作无法读完，并且会阻塞程序执行。
后续新数据来了，由于没有状态变化(此刻还卡在这，当然没变化)，epoll 不会再次通知(ET模式的定义)。
这就导致部分数据无法读取，数据丢失。
```

面wxg日常实习时，面试官问完IO复用，又问你了解哪些IO复用函数？答了select和epoll后，对方要求我区别两者，于是有如下补充：

## I/O多路复用
- 概念：指在单线程或单进程环境下同时监控多个 I/O 事件的技术。
- 原理：使用一个系统调用(select、poll、epoll等)同时监听多个IO源的状态变化。
程序将需要监控的IO源(通常是文件描述符)注册到这个系统调用中，然后阻塞等待。
当其中任何一个或多个IO源发生变化(如可读、可写、出错等)时，系统调用会返回，并告知程序哪些IO源已经准备好进行相应的操作。
程序根据返回的信息，对准备好的IO源进行读写操作。

也就是说通过系统调用，起到一个监控的作用，同时监控很多IO。

## select/epoll了解过吗？
- select:
```cpp
#include <sys/select.h>
int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
```
参数依次是
nfds：需要监控的最大文件描述符值加 1；readfds：指向一个fd集合，监控读事件；writefds：指向一个fd集合，监控写事件。；exceptfds：指向一个fd集合，用于监控这些文件描述符的异常事件。；timeout：指定 select 函数的超时时间。

工作流程是：使用时初始fd_set集合，将需要监控的文件描述符加入，调用select函数。
原理是：监控时遍历所有监控的文件描述符，很慢；能监控的fd也有限，一般是1024个；每次调用都要传入这么多参数，有的fd
可能被反复传了很多次。

- epoll:

工作流程是：调用 epoll_create 创建一个 epoll 实例；使用 epoll_ctl 函数向 epoll 实例中增删改需要监控的文件描述符和事件;调用 epoll_wait 函数进行阻塞等待。当有事件发生或超时后，epoll_wait 函数返回，检查 events 数组来确定哪些文件描述符有事件发生。

原理是：红黑树+链表。红黑树实现快速增删改，链表存储有变动的事件调用 epoll_wait 函数时，内核会将链表复制到用户空间，通知用户哪些fd准备好了。
红黑树的增删改是O(logN)的，返回的链表也只包含了需要处理的事件，没有冗余信息。


## 这块变动比较大的主要是server.cpp，请看官主要关注server.cpp。
