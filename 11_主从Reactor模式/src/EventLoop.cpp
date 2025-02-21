#include "EventLoop.h"
#include "Epoll.h"
#include "Channel.h"
#include <vector>
#include <mutex>
#include <iostream>
#include <chrono>

EventLoop::EventLoop() : ep(nullptr), quit(false),  q(new CircularQueue(100)){
    ep = new Epoll();
    channelCount=0;
    avgHandleTime=0;
}

EventLoop::~EventLoop(){
    delete ep;
    delete q;
}


void EventLoop::loop(){
    while(!quit){
        std::vector<Channel*> chs;
        chs = ep->poll();
        
        double oldChannelCount = channelCount;
        double oldAvgHandleTIme = avgHandleTime;
        channelCount=chs.size();
        avgHandleTime=q->getAvg();
        printf("avgTime:%.2lf\n",q->getAvg());
        bool handleTimeChanged = (oldAvgHandleTIme > 0) && 
                                      ((avgHandleTime / oldAvgHandleTIme > 2.0) || 
                                       (avgHandleTime / oldAvgHandleTIme < 0.5));
            
        bool channelCountChanged = (oldChannelCount > 0) && 
                                        ((oldChannelCount / channelCount > 2.0) || 
                                         (oldChannelCount / channelCount < 0.5));
        
     //   std::mutex callbackMutex;
     //   {
     //       std::lock_guard<std::mutex> lock(callbackMutex);
            if (updateCallback &&
                (handleTimeChanged || channelCountChanged)
                && 1) {
                updateCallback(oldAvgHandleTIme, oldChannelCount, avgHandleTime, channelCount, this);
            }
     //    }
/*
修改前:
互斥锁+不判非空：bad_function_callback
互斥锁+判非空：正常运行
无互斥锁+判非空：正常运行
无互斥锁+不判非空：bad_function_callback

修改:

*/
        for(auto it = chs.begin(); it != chs.end(); ++it){
            auto start = std::chrono::high_resolution_clock::now();
            (*it)->handleEvent();
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = end - start;
            

            double elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
            q->enqueue(elapsedTime);
        }
    }
}

void EventLoop::updateChannel(Channel *ch){
    ep->updateChannel(ch);
}

void EventLoop::setUpdateCallback(std::function<void(double, double, double, double,EventLoop*)> _cb){
    updateCallback = _cb;
}
