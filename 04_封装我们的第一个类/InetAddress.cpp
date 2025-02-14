#include "InetAddress.h"
#include <string.h>

InetAddress :: InetAddress() : addr_len(sizeof(addr)){
    bzero(&addr,sizeof(addr));
}

InetAddress :: InetAddress(const char* ip,uint16_t port) : addr_len(sizeof(addr)){
    /*
    上述语句中addr_len(sizeof(addr))意在初始化addr_len的值为sizeof(addr)；
    这样的写法叫初始化列表，当然也可以写在函数体里
    Q：此处为什么跟在参数后面？
    A：会比较快。在函数中对成员变量赋值时，会先调用默认构造函数初始化。
    再调用赋值运算符进行赋值，多了一次默认构造的过程，效率较低。

    用武之地：
    常量成员、引用成员、没有默认构造函数的类成员时就必须用初始化列表初始化；
    而需要根据传入的参数判断赋什么值时，就只能用函数体初始化了。

    */
    
    bzero(&addr,sizeof(addr));
    addr.sin_addr.s_addr = inet_addr(ip);
    addr.sin_port = htons(port);

}

InetAddress::~InetAddress(){

}
