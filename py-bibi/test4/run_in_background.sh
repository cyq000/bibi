#!/bin/bash

# 配置：脚本路径和日志目录
SCRIPT=test.py #"1h_L_s_info.py"          # 替换成你的 Python 脚本名
LOG_FILE="output.log"            # 日志文件
PID_FILE="pid.pid"              # 保存进程号的文件

# 检查是否已经在运行
if [ -f "$PID_FILE" ]; then
    PID=$(cat "$PID_FILE")
    if ps -p $PID > /dev/null 2>&1; then
        echo "脚本已经在运行中，PID: $PID"
        exit 1
    else
        echo "发现 PID 文件但进程未运行，清除旧的 PID 文件"
        rm -f "$PID_FILE"
    fi
fi

# 启动程序
echo "正在启动脚本..."
nohup python3 "$SCRIPT" > "$LOG_FILE" 2>&1 &

# 保存 PID
PID=$!
echo $PID > "$PID_FILE"
echo "脚本已后台运行，PID: $PID"
