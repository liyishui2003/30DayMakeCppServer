#pragma once
#include <functional>
#include <vector>
class Epoll;
class Channel;

class CircularQueue {
    private:
        std::vector<double> queue;
        double avg;
        double sum;
        int capacity;
        int front;
        int rear;
        int size;
    
    public:
        CircularQueue(int cap) : capacity(cap), front(0), rear(0), size(0), avg(0), sum(0){
            queue.resize(capacity);
        }
    
        void enqueue(double value) {
            if (size == capacity) {
                sum-=queue[front];
                front = (front + 1) % capacity;
            } else {
                size++;
            }
            queue[rear] = value;
            sum+=value;
            rear = (rear + 1) % capacity;
            avg=sum/(1.0*size);
        }

        double getAvg(){
            return this->avg;
        }
};

class EventLoop
{
private:
    Epoll *ep;
    CircularQueue *q;
    double channelCount;
    double avgHandleTime;
    bool quit;
    std::function<void(double, double, double, double,EventLoop*)> updateCallback;
public:
    EventLoop();
    ~EventLoop();

    void loop();
    void updateChannel(Channel*);
    void addThread(std::function<void()>);
    void setUpdateCallback(std::function<void(double, double, double, double,EventLoop*)>);
};



