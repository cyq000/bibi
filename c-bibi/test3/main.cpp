#include "Worker.h"
#include "Logger.h"
#include "BinanceAPIManager.h"
#include "FeishuNotifier.h"
#include <iostream>

#include <fstream>

// --------测试--------
#if 1
#include "OIAnalyzer.h"
void testOIAnalyzer()
{
    // --------测试山寨监控策略--------

    // 1. JSON 文件名（放在同目录）
    const std::string filename = "tnsr.json";

    // 2. 打开文件
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file: " << filename << std::endl;
        return;
    }

    // 3. 读取整个文件内容到字符串
    std::string rawJson((std::istreambuf_iterator<char>(file)),
                   std::istreambuf_iterator<char>());

    file.close();

    // 4. 解析 JSON → 数据结构
    std::vector<OIData> data = OIAnalyzer().parseJson(rawJson);

    // 5. 执行分析
    OIAnalyzer analyzer;
    AnalysisResult r = analyzer.analyze(data);

    // 6. 输出结果
    std::cout << "==========================" << std::endl;
    std::cout << "   Analysis Result" << std::endl;
    std::cout << "==========================" << std::endl;
    std::cout << "Score: " << r.score << std::endl;
    std::cout << "OI Up Count: " << r.OI_up_count << std::endl;
    std::cout << "Price Down Count: " << r.Price_down_count << std::endl;
    std::cout << "Status: " << r.status << std::endl;

  
}
// --------测试--------
#endif

int main() {

    /* =============== 测试程序 ===============*/
    #if 1
    testOIAnalyzer();
    #endif
    /* =============== 测试程序 ===============*/

    /* =============== 程序启动前置设置 ===============*/
    std::cout << "[INFO] 程序启动" << std::endl;
    std::cout.flush();
    //std::this_thread::sleep_for(std::chrono::seconds(3)); // 等 3 秒




    // 设置飞书 Webhook（只需要一次）
    FeishuNotifier::instance().setWebhook("https://open.feishu.cn/open-apis/bot/v2/hook/6f399df9-a303-42a6-84cd-ea23d3d7cadd");
    // 设置日志文件
    Logger::instance().setLogFile("monitor.log");
    LOG_INFO("启动监控程序...");

    // worker.start(); 



    // /* =============== 程序启动中设置 ===============*/

    // // 无限运行
    // while (true) {
    //     std::this_thread::sleep_for(std::chrono::seconds(21600)); // 每 6小时 做一次循环
    //     // 可以在这里添加周期性的日志或状态输出
    //     LOG_INFO("监控程序仍在运行...");
    //     std::cout.flush();
    // }

    // // 程序正常退出逻辑（永远不会执行到这里）
    // LOG_INFO("停止监控程序...");
    // worker.stop();

    return 0;
}
