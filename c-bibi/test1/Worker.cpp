#include "Worker.h"
#include "Logger.h"
#include "FeishuNotifier.h"

#include <chrono>
#include <iostream>

Worker::Worker(BinanceAPI& api) : api_(api) {}

void Worker::start(int numThreads) {
    stop_ = false;

    // Step 1: 获取前提条件的币种
    universe_ = api_.getSymbolsWithVolume(50000000, 6000000000);    // 5千万到60亿

    // Step 1: 获取前提条件的币种 (测试)
    // universe_ = {"LTCUSDT", "XRPUSDT"};  // 固定测试数据

    LOG_INFO("初始化筛选出 " + std::to_string(universe_.size()) + " 个币种用于监控");


    // Step 2: 启动条件1线程（多线程）
    int perThread = std::max(1, (int)universe_.size() / numThreads);
    for (int i = 0; i < numThreads; ++i) {
        int startIdx = i * perThread;
        int endIdx = (i == numThreads - 1) ? universe_.size() : startIdx + perThread;
        if (startIdx >= universe_.size()) break; // 防止越界
        std::vector<std::string> symbols(universe_.begin() + startIdx, universe_.begin() + endIdx);
        threads_.emplace_back(&Worker::condition1Worker, this, symbols, i);
    }

    // Step 3: 启动条件2线程
    threads_.emplace_back(&Worker::condition2Worker, this);

    // Step 4: 启动公共事务线程
    threads_.emplace_back(&Worker::commonWorker, this);
}

void Worker::stop() {
    stop_ = true;
    for (auto& t : threads_) {
        if (t.joinable()) t.join();
    }
    threads_.clear();
}

void Worker::condition1Worker(std::vector<std::string> symbols, int id)
{
    LOG_INFO("条件1线程 " + std::to_string(id) + " 启动，负责 " + std::to_string(symbols.size()) + " 个币种");

    while (!stop_) {
        for (auto& sym : symbols) {
            auto res = api_.checkCondition1(sym);
            if (res.triggered) {
                std::lock_guard<std::mutex> lock(mu_);
                if (cond2Map_.find(sym) == cond2Map_.end()) {  // 不重复添加
                    cond2Map_[sym] = res;
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::hours(2));
    }
}

void Worker::condition2Worker() {
    LOG_INFO("条件2线程启动");

    while (!stop_) {
        std::vector<std::pair<std::string, ConditionResult>> cond2Symbols;
        {
            std::lock_guard<std::mutex> lock(mu_);
            cond2Symbols.assign(cond2Map_.begin(), cond2Map_.end());
        }

        for (auto& sym : cond2Symbols) {
            auto res = api_.checkCondition2(sym.first);
            if (res.triggered) {
                std::ostringstream msg;
                msg << "⚡ 币种: " << sym.first << "/n" 
                    // << " 平均OI=" << res.avgOI
                    // << " 最新OI=" << res.lastOI
                    << " | 条件1触发时间: " << std::put_time(std::localtime(&sym.second.triggerTime), "%F %T")
                    << " | 5分钟合约持仓量变化=" << (res.percentDiff * 100.0) << "%";

                LOG_INFO(msg.str());
                FeishuNotifier::instance().sendMessage(msg.str());

                // 通知发出后移出数组
                {
                    std::lock_guard<std::mutex> lock(mu_);
                    cond2Map_.erase(sym.first);
                    // LOG_INFO("已从条件2数组移除: " + sym);
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::minutes(5));
    }
}

void Worker::commonWorker() {
    LOG_INFO("公共事务线程启动");

    while (!stop_) {
        {
            std::lock_guard<std::mutex> lock(mu_);
            if (cond2Map_.empty()) {
                LOG_INFO("[公共事务] cond2Map_ 当前为空");
            } else {
                LOG_INFO("[公共事务] cond2Map_ 当前币种列表：");
                for (auto& [sym, res] : cond2Map_) {
                    std::ostringstream oss;
                    oss << "  - " << sym
                        << " | 条件1触发时间: "
                        << std::put_time(std::localtime(&res.triggerTime), "%F %T");
                    LOG_INFO(oss.str());
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::hours(6));
    }
}
