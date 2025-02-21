


#include "Server.h"
#include "Socket.h"
#include "Acceptor.h"
#include "Connection.h"
#include "ThreadPool.h"
#include "EventLoop.h"
#include <unistd.h>
#include <functional>

Server::Server(EventLoop *_loop) : mainReactor(_loop), acceptor(nullptr){ 
    acceptor = new Acceptor(mainReactor);

    std::function<void(Socket*)> cb = std::bind(&Server::newConnection,this,std::placeholders::_1);
    acceptor->setNewConnectionCallback(cb);

    int size = std::thread::hardware_concurrency();

    thpool = new ThreadPool(size);

    for(int i=0;i<size;i++){
       
        EventLoop* newEventLoop = new EventLoop();
        
        std::function<void(double,double,double,double,EventLoop*)> cb = std::bind(&Server::leastEventPoll,this,
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4,std::placeholders::_5);
        
        newEventLoop->setUpdateCallback(cb);

        subReactors.push_back(newEventLoop);
    }

    for(int i=0;i<size;i++){
        std::function<void()> sub_loop=std::bind(&EventLoop::loop,subReactors[i]);

        thpool->add(sub_loop);
    }
}

Server::~Server(){
    delete acceptor;
    delete thpool;
}


void Server::newConnection(Socket *sock){
    if(sock->getFd() != -1){

        EventLoop* leastSubReactor=nullptr;
        { 
            std::lock_guard<std::mutex> lock(bookMutex);
                for(int i=0;i<8;i++){
                    if(book[subReactors[i]]) continue;
                    book[subReactors[i]]=1;
                    leastSubReactor=subReactors[i];
                    break;
            }

            if(leastSubReactor==nullptr){
                if(eventLoopSet.empty()){
                    leastSubReactor = subReactors[sock->getFd()%8];
                }
                else leastSubReactor = (*(eventLoopSet.begin())).eventLoop; 
            }
        }
        
        printf("leastReactor: %p\n",leastSubReactor);
        Connection *conn = new Connection(leastSubReactor, sock);
        std::function<void(int)> cb = std::bind(&Server::deleteConnection, this, std::placeholders::_1);
        conn->setDeleteConnectionCallback(cb);
        connections[sock->getFd()] = conn;
    }
}

void Server::deleteConnection(int sockfd){
    if(sockfd != -1){
        auto it = connections.find(sockfd);
        if(it != connections.end()){
            Connection *conn = connections[sockfd];
            connections.erase(sockfd);
            // close(sockfd);       //正常
            delete conn;         //会Segmant fault
        }
    }
}

void Server::leastEventPoll(double oldAvgHandleTIme,double oldChannelCount,double avgHandleTime,double channelCount,EventLoop* eventLoop){
      
      std::lock_guard<std::mutex> lock(this->eventLoopSetMutex);

      double oldWeight = 0.6*oldAvgHandleTIme+0.4*oldChannelCount;
      double newWeight = 0.6*avgHandleTime+0.4*channelCount;

      EventLoopInfo oldInfo = {oldWeight, oldAvgHandleTIme, oldChannelCount, eventLoop};
      
      printf("Old Info: Weight = %.2lf, Avg Handle Time = %.2lf, Channel Count = %.lf, EventLoop Address = %p\n",
        oldWeight, oldAvgHandleTIme, oldChannelCount, eventLoop);


        for (auto it = eventLoopSet.begin(); it != eventLoopSet.end(); ) {
            if (it->eventLoop == eventLoop) {
                it = eventLoopSet.erase(it);
                break;
            } else {
                ++it;
            }
        }
      

      EventLoopInfo newInfo = {newWeight, avgHandleTime, channelCount, eventLoop};
      printf("New Info: Weight = %.2lf, Avg Handle Time = %.2lf, Channel Count = %.lf, EventLoop Address = %p\n",
        newWeight, avgHandleTime, channelCount, eventLoop);

      auto result = eventLoopSet.insert(newInfo);
      if (result.second) {
        puts( "Insertion into eventLoopSet was successful." );
        for(auto v:eventLoopSet){
            printf(" T:%.2lf cnt:%.lf w:%lf *p:%p\n",v.avgHandleTime,v.channelCount,v.weight,v.eventLoop);
        }
      } 

      
}