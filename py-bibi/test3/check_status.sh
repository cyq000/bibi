#!/bin/bash

PID_FILE="pid.pid"

if [ -f "$PID_FILE" ]; then
    PID=$(cat "$PID_FILE")
    if ps -p $PID > /dev/null 2>&1; then
        echo "脚本正在运行，PID: $PID"
    else
        echo "脚本未运行"
    fi
else
    echo "没有 PID 文件，脚本可能未启动"
fi
