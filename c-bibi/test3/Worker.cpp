// #include "Worker.h"
// #include "Logger.h"
// #include "FeishuNotifier.h"
// #include "BinanceAPIManager.h"

// #include <chrono>
// #include <iostream>

// Worker::Worker() {}


// void Worker::init() {
// }
// void Worker::start() {
//     stop_ = false;

//     // 获取前提条件的币种
//     m_vecSymbols = BinanceAPIManager::instance(). getSymbolsWithVolume(20000000, 80000000);    // 2千万到8千万

//     // 获取前提条件的币种 (测试)
//     // m_vecSymbols = {"LTCUSDT", "XRPUSDT"};  // 固定测试数据

//     LOG_INFO("初始化筛选出 " + std::to_string(m_vecSymbols.size()) + " 个币种用于监控");


//     // 条件1线程（因为币种数量比较多，使用多线程）
//     int perThread = std::max(1, (int)m_vecSymbols.size() / m_numThreads);
//     for (int i = 0; i < m_numThreads; ++i) {
//         int startIdx = i * perThread;
//         int endIdx = (i == m_numThreads - 1) ? m_vecSymbols.size() : startIdx + perThread;
//         if (startIdx >= m_vecSymbols.size()) break; // 防止越界
//         std::vector<std::string> symbols(m_vecSymbols.begin() + startIdx, m_vecSymbols.begin() + endIdx);
//         threads_.emplace_back(&Worker::condition1Worker, this, symbols, i);
//     }

//     // 条件2线程
//     threads_.emplace_back(&Worker::condition2Worker, this);


//     // 获取前提条件的币种
//     m_vecSymbols = BinanceAPIManager::instance().getSymbolsWithVolume(30000000, 6000000000);    // 24小时合约持仓量 3000万到60亿
//     // 条件3线程
//     threads_.emplace_back(&Worker::condition3Worker, this, m_vecSymbols);

    
//     // 公共事务线程
//     threads_.emplace_back(&Worker::commonWorker, this);
// }

// void Worker::stop() {
//     stop_ = true;
//     for (auto& t : threads_) {
//         if (t.joinable()) t.join();
//     }
//     threads_.clear();
// }

// void Worker::condition1Worker(std::vector<std::string> symbols, int id)
// {
//     LOG_INFO("条件1线程 " + std::to_string(id) + " 启动，负责 " + std::to_string(symbols.size()) + " 个币种");

//     while (!stop_) {
//         for (auto& sym : symbols) {
//             if(sym == "币安人生USDT") continue;
//             auto res = BinanceAPIManager::instance().checkCondition1(sym);
//             if (res.triggered) {
//                 std::lock_guard<std::mutex> lock(mu_);
//                 if (cond2Map_.find(sym) == cond2Map_.end()) {  // 不重复添加
//                     cond2Map_[sym] = res;
//                 }
//             }
//         }
//         std::this_thread::sleep_for(std::chrono::hours(2));
//     }
// }

// void Worker::condition2Worker() {
//     LOG_INFO("条件2线程启动");

//     while (!stop_) {
//         std::vector<std::pair<std::string, ConditionResult>> cond2Symbols;
//         {
//             std::lock_guard<std::mutex> lock(mu_);
//             cond2Symbols.assign(cond2Map_.begin(), cond2Map_.end());
//         }

//         for (auto& sym : cond2Symbols) {
//             auto res = BinanceAPIManager::instance().checkCondition2(sym.first);
//             if (res.triggered) {
//                 std::ostringstream msg;
//                 msg << "⚡ 币种: " << sym.first << "/n"
//                     // << " 平均OI=" << res.avgOI
//                     // << " 最新OI=" << res.lastOI
//                     << " | 条件1触发时间: " << std::put_time(std::localtime(&sym.second.triggerTime), "%F %T")
//                     << " | 5分钟合约持仓量变化=" << (res.percentDiff * 100.0) << "%";

//                 LOG_INFO(msg.str());
//                 FeishuNotifier::instance().sendMessage(msg.str());

//                 // 通知发出后移出数组
//                 {
//                     std::lock_guard<std::mutex> lock(mu_);
//                     cond2Map_.erase(sym.first);
//                     // LOG_INFO("已从条件2数组移除: " + sym);
//                 }
//             }
//         }
//         std::this_thread::sleep_for(std::chrono::minutes(5));
//     }
// }

// void Worker::commonWorker() {
//     LOG_INFO("公共事务线程启动");

//     while (!stop_) {
//         {
//             std::lock_guard<std::mutex> lock(mu_);
//             if (cond2Map_.empty()) {
//                 LOG_INFO("[公共事务] cond2Map_ 当前为空");
//             } else {
//                 LOG_INFO("[公共事务] cond2Map_ 当前币种列表：");
//                 for (auto& [sym, res] : cond2Map_) {
//                     std::ostringstream oss;
//                     oss << "  - " << sym
//                         << " | 条件1触发时间: "
//                         << std::put_time(std::localtime(&res.triggerTime), "%F %T");
//                     LOG_INFO(oss.str());
//                 }
//             }
//         }

//         std::this_thread::sleep_for(std::chrono::hours(6));
//     }
// }

// void Worker::condition3Worker(std::vector<std::string> symbols)
// {
//     LOG_INFO("条件3线程启动,负责 " + std::to_string(symbols.size()) + " 个币种");

//     while (!stop_)
//     {
//         for (auto &sym : symbols)
//         {

//             std::lock_guard<std::mutex> lock(mu_3);
//             ConditionResult Cond_12h;
//             // Cond_12h.oiTrend12h.timeframe = "12h";
//             // // 获取 OI 数据
//             // std::vector<double> oi12h = BinanceAPIManager::instance().fetchOIHistory(sym, Cond_12h);
//             // // 分析1
//             // BinanceAPIManager::instance().analyzeOITrend(oi12h, Cond_12h);

//             // if (Cond_12h.oiTrend12h.isGrowing)
//             // {
//             //     // 打印日志
//             //     std::ostringstream msg;
//             //     msg << std::fixed << std::setprecision(1);  //只保留小数点后一位
//             //     msg << "[OITrend] " << sym
//             //         << " | 12h growing=" << Cond_12h.oiTrend12h.isGrowing
//             //         << " slope=" << Cond_12h.oiTrend12h.slope
//             //         << " growthRate=" << Cond_12h.oiTrend12h.growthRate << "%";
//             //     LOG_INFO(msg.str());
//             //     FeishuNotifier::instance().sendMessage(msg.str());
//             // }


//             auto data = BinanceAPIManager::instance().fetchOpenInterestData(sym, "12h", 10);
//             ConditionResult result = BinanceAPIManager::instance().evaluateStableOI(data);
//             if (result.triggered)
//             {
//                 // 打印日志
//                 std::ostringstream msg;
//                 msg << std::fixed << std::setprecision(1);  //只保留小数点后一位
//                 msg << "[Condition3] " << sym
//                     << " triggerTime=" << std::put_time(std::localtime(&result.triggerTime), "%F %T");

//                 LOG_INFO(msg.str());
//                 FeishuNotifier::instance().sendMessage(msg.str());
//             }

//         }

//         // 每10小时更新一次
//         std::this_thread::sleep_for(std::chrono::hours(10));
//     }
// }

// void WorkerManager::init()
// {
//     m_worker.init();
// }
// void WorkerManager::start()
// {
//     m_worker.start();
// }
// void WorkerManager::stop()
// {
//     m_worker.stop();
// }