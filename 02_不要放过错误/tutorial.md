这块主要是对Day01的补充：对socketfd进行读(read())/写(write())，以及封装好util.cpp用来处理异常。 没有什么新知识，除了注意：linux的文件描述符有限，用完要closed。这和malloc后要free一样，是个好习惯。 

由于引入了util.h,makefile也有所变动，编译时需要把util.cpp分别和server/client链接。此外，多了一个clean功能（之前说过make是支持clean和test的），rm server && rm client表示删除生成的server.exe和client.exe。

同理，这部分能补充的新知识很有限，请看官移步code。
