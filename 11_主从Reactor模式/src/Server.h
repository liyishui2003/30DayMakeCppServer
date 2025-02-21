#pragma once
#include <map>
#include <set>
#include <vector>
#include <mutex>
class EventLoop;
class Socket;
class Acceptor;
class Connection;
class ThreadPool;

struct EventLoopInfo {
    double weight;
    double avgHandleTime;
    double channelCount;
    EventLoop* eventLoop;

    bool operator<(const EventLoopInfo& other) const {
        if (weight != other.weight) {
            return weight < other.weight;
        }
        if (avgHandleTime != other.avgHandleTime) {
            return avgHandleTime < other.avgHandleTime;
        }
        if (channelCount != other.channelCount) {
            return channelCount < other.channelCount;
        }
        return eventLoop < other.eventLoop;
    };
};

class Server
{
private:
    EventLoop *mainReactor;
    Acceptor *acceptor;
    std::map<int, Connection*> connections;
    std::vector<EventLoop*> subReactors;
    ThreadPool *thpool;
    std::set<EventLoopInfo> eventLoopSet;
    std::mutex eventLoopSetMutex;

    std::mutex bookMutex;  // 定义互斥锁
    std::map<EventLoop*,int> book;
    public:
    Server(EventLoop*);
    ~Server();

    void handleReadEvent(int);
    void newConnection(Socket *sock);
    void deleteConnection(int sockfd);
    /*
        double oldChannelCount = 1.0*channelCount;
        double oldAvgHandleTIme = avgHandleTime;
        channelCount=chs.size();
        avgHandleTime=q->getAvg();
    */
    void leastEventPoll(double oldAvgHandleTIme,double oldChannelCount,double avgHandleTime,double channelCount,EventLoop* EventLoop);
};


