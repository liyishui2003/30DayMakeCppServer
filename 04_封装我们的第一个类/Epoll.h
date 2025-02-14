#pragma once
//pragma once保证该.h文件在编译时只被包含一次,避免因重复包含而导致编译错误
#include <sys/epoll.h>
#include <vector>
/*
如果说Epoll.cpp在实现函数的具体细节
Epoll.h就是在声明我有哪些函数、哪些变量
这样分开定义和细节的声明方便开发者调用函数、明确结构体的成员
*/
class Epoll{
    private:
        int epfd;
        struct epoll_event *events;
    public:
        Epoll();
        ~Epoll();

        void addFd(int fd,uint32_t op);
        std::vector<epoll_event> poll(int timeout=-1);
};
