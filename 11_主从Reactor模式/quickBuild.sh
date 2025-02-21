#!/bin/bash

# 步骤 1: 执行 make clean 和 make
echo "执行 make clean 和 make..."
make clean
# if [ $? -ne 0 ]; then
#     echo "make clean 执行失败，脚本终止。"
#     exit 1
# fi

make
if [ $? -ne 0 ]; then
    echo "make 执行失败，脚本终止。"
    exit 1
fi

# 步骤 2: 查找并杀死占用 1234 端口的进程
echo "查找并杀死占用 1234 端口的进程..."
port=1234
pid=$(netstat -tulnp | grep ":$port " | awk '{print $7}' | cut -d/ -f1)
if [ -n "$pid" ]; then
    echo "发现占用 $port 端口的进程，进程 ID 为 $pid，正在杀死该进程..."
    kill -9 $pid
    if [ $? -eq 0 ]; then
        echo "进程 $pid 已成功杀死。"
    else
        echo "杀死进程 $pid 失败，脚本终止。"
        exit 1
    fi
else
    echo "未发现占用 $port 端口的进程。"
fi

# 步骤 3: 启动 GDB 调试./server 并自动执行 run 命令
echo "启动 GDB 调试./server..."
gdb ./server
