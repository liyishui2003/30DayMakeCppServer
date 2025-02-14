#include "util.h"
#include <stdio.h>
#include <stdlib.h>
void errif(bool condition,const char *errmsg){
    if(condition){
        perror(errmsg);
        /*
        perror输出最近一次系统调用产生的错误信息。
        它会将 errmsg 字符串和系统错误信息一起输出到标准错误输出（通常是终端）。
        例，如果 errmsg 是 "Open file failed"，
        最近的系统调用因为文件不存在而失败，
        perror 可能输出 "Open file failed: No such file or directory" 
        */
        exit(EXIT_FAILURE);
    }
}
