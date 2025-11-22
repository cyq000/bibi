// #ifndef WORKER_H
// #define WORKER_H

// #include <string>
// #include <vector>
// #include <unordered_map>
// #include <thread>
// #include <mutex>
// #include <atomic>
// #include "BinanceAPIManager.h"
// #include "OIAnalyzer.h"

// class Worker {
// public:
//     Worker();
//     void init();
//     void start();
//     void stop();

// private:
//     // 工作线程1
//     void condition1Worker(std::vector<std::string> symbols, int id);
//     // 工作线程2
//     void condition2Worker();
//     // 公共线程
//     void commonWorker();
//     // 工作线程3
//     void condition3Worker(std::vector<std::string> symbols);


//     int m_numThreads;
//     std::vector<std::string> m_vecSymbols;  // 监控的币种列表
//     std::unordered_map<std::string, ConditionResult> cond2Map_;
//     std::mutex mu_;
//     std::mutex mu_3;    // 条件3专用
//     std::atomic<bool> stop_{false};
//     std::vector<std::thread> threads_;




//     /*
//     * 线程：监控：山寨币：币种24小时成交量(2000wu,8000wu)、只做多、
//     * 1.前置条件：12小时：OI持仓量平稳(小幅度波动)、价格平稳（小幅度波动）、
//     * 2.OI持仓量增长(幅度波动)、价格平稳（小幅度波动）、：持仓量增长说明有资金进入市场（庄家吸筹）
//     *   这段时间内（7天——30天）日交易量增加(幅度波动)、：交易量波动增长说明市场活跃度提升（庄家吸筹）
//     * 3.24小时：OI持仓量增长(幅度波动)、价格上涨（幅度波动）
//     *
//     */

//     /*
//     * 线程：监控：山寨币：币种24小时成交量(2000wu,8000wu)、只做多、
//     * 1.15分钟级别：OI持仓量增长(>8倍 / 10M)、价格平稳（小幅度波动）、：持仓量增长说明有资金进入市场（庄家吸筹）
//     *   这段时间内（7天——30天）日交易量增加(幅度波动)、：交易量波动增长说明市场活跃度提升（庄家吸筹）
//     *
//     */

// };



// class WorkerManager {
// public:
//     static WorkerManager& instance() {
//         static WorkerManager inst;
//         return inst;
//     }
//     void init();
//     void start();
//     void stop();

// private:
//     WorkerManager();
//     ~WorkerManager();
//     WorkerManager(const WorkerManager&) = delete;
//     WorkerManager& operator=(const WorkerManager&) = delete;

//     Worker m_worker;
// };

// #endif
