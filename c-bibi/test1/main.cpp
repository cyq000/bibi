#include "Worker.h"
#include "Logger.h"
#include "BinanceAPI.h"
#include "FeishuNotifier.h"
#include <iostream>

int main() {
    std::cout << "[INFO] 程序启动" << std::endl;
    std::cout.flush();
    std::this_thread::sleep_for(std::chrono::seconds(3)); // 等 3 秒
    
    BinanceAPI api;
    Worker worker(api);

    // 设置飞书 Webhook（只需要一次）
    FeishuNotifier::instance().setWebhook("https://open.feishu.cn/open-apis/bot/v2/hook/6f399df9-a303-42a6-84cd-ea23d3d7cadd");

    Logger::instance().setLogFile("monitor.log");
    LOG_INFO("启动监控程序...");


    worker.start(2);  // 分4个线程跑条件1


    // 无限运行
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(21600)); // 每 6小时 做一次循环
        // 可以在这里添加周期性的日志或状态输出
        LOG_INFO("监控程序仍在运行...");
        std::cout.flush();
    }

    // 程序正常退出逻辑（永远不会执行到这里）
    LOG_INFO("停止监控程序...");
    worker.stop();

    return 0;
}
