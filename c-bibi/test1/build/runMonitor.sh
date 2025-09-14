#!/bin/bash

# ./monitor.sh start
# ./monitor.sh stop
# ./monitor.sh restart
# ./monitor.sh status

# 程序名
PROGRAM="monitor"
# 程序路径（改成你 monitor 可执行文件所在的路径）
PROGRAM_PATH="/home/ubuntu/bibi/c-bibi/test1/build/$PROGRAM"
# 日志输出文件
LOG_FILE="monitor.out"

# 查找进程函数
get_pid() {
    pgrep -x "$PROGRAM"
}

start() {
    PID=$(get_pid)
    if [ -n "$PID" ]; then
        echo "[INFO] $PROGRAM 已在运行，PID=$PID"
    else
        echo "[INFO] 启动 $PROGRAM ..."
        nohup "$PROGRAM_PATH" > "$LOG_FILE" 2>&1 &
        NEWPID=$!
        echo "[INFO] $PROGRAM 已启动，PID=$NEWPID"
    fi
}

stop() {
    PID=$(get_pid)
    if [ -n "$PID" ]; then
        echo "[INFO] 停止 $PROGRAM (PID=$PID) ..."
        kill -9 "$PID"
        echo "[INFO] $PROGRAM 已停止"
    else
        echo "[INFO] $PROGRAM 未在运行"
    fi
}

restart() {
    stop
    sleep 1
    start
}

status() {
    PID=$(get_pid)
    if [ -n "$PID" ]; then
        echo "[INFO] $PROGRAM 正在运行，PID=$PID"
    else
        echo "[INFO] $PROGRAM 未运行"
    fi
}

case "$1" in
    start)
        start
        ;;
    stop)
        stop
        ;;
    restart)
        restart
        ;;
    status)
        status
        ;;
    *)
        echo "用法: $0 {start|stop|restart|status}"
        exit 1
        ;;
esac
