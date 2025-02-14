#include "Epoll.h"
#include "util.h"
#include <unistd.h>
#include <string.h>

#define MAX_EVENTS 1024
//:: 是作用域解析运算符，用来明确标识符(像类名、函数名、变量名等)所属作用域
//此处::表明这些函数属于Epoll类，和在struct Epoll的定义里写函数是一样的
Epoll::Epoll() : epfd(-1),events(nullptr){
    /*
    在成员初始化，表示epfd默认-1，events默认nullptr。通用模板为：
    ClassName::ConstructorName(parameters) : member1(value1), member2(value2), ... {
        // 构造函数体
    }
    */
    epfd = epoll_create1(0);
    errif(epfd==-1,"epoll create error");
    events=new epoll_event[MAX_EVENTS];
    bzero(events,sizeof(*events)*MAX_EVENTS);
}

// ~用于定义类的析构函数
//析构函数用来资源清理，通常有delete或者close等。
Epoll::~Epoll(){
    if(epfd!=-1){
        close(epfd);
        epfd=-1;
    }
    delete [] events;
}

//往epoll的红黑树里加入event事件
void Epoll:addFd(int fd,__uint32_t op){
    struct epoll_event ev;
    bzero(&ev,sizeof(ev));
    ev.data.fd=fd;
    ev.events=op;
    errif(epoll_ctl(epfd,EPOLL_CTL_ADD,fd,&ev)==-1,"epoll add event error");
}

//把epoll实例中监控的事件存储在vector中返回
std::vector<epoll_event>Epoll:poll(int timeout){
    std::vector<epoll_event> activeEvents;
    int nfds = epoll_wait(epfd,events,MAX_EVENTS,timeout);
    errif(nfds==-1,"epoll wait error");
    for(int i=0;i<nfds;i++){
        activeEvents.push_back(events[i]);
    }
    return activeEvents;
}
