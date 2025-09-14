#include "Worker.h"
#include "Logger.h"
#include "FeishuNotifier.h"

#include <chrono>
#include <iostream>

Worker::Worker(BinanceAPI& api) : api_(api) {}

void Worker::start(int numThreads) {
    stop_ = false;

    // Step 1: 获取前提条件的币种
    universe_ = api_.getSymbolsWithVolume(10000000, 6000000000);    // 1千万到60亿

    // Step 1: 获取前提条件的币种 (测试)
    // universe_ = {"LTCUSDT", "XRPUSDT"};  // 固定测试数据

    LOG_INFO("初始化筛选出 " + std::to_string(universe_.size()) + " 个币种用于监控");


    // Step 2: 启动条件1线程
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
                if (cond2Set_.find(sym) == cond2Set_.end()) {  // 不重复添加
                    cond2Set_.insert(sym);
                    // LOG_INFO("条件1符合并加入条件2数组: " + sym +
                    //          " 平均OI=" + std::to_string(res.avgOI) +
                    //          " 最新OI=" + std::to_string(res.lastOI));
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::hours(2));
    }
}

void Worker::condition2Worker() {
    LOG_INFO("条件2线程启动");

    while (!stop_) {
        std::vector<std::string> cond2Symbols;
        {
            std::lock_guard<std::mutex> lock(mu_);
            cond2Symbols.assign(cond2Set_.begin(), cond2Set_.end());
        }

        for (auto& sym : cond2Symbols) {
            auto res = api_.checkCondition2(sym);
            if (res.triggered) {
                std::ostringstream msg;
                msg << "⚡ 币种: " << sym
                    // << " 平均OI=" << res.avgOI
                    // << " 最新OI=" << res.lastOI
                    << " .5分钟合约持仓量变化=" << (res.percentDiff * 100.0) << "%";

                LOG_INFO(msg.str());
                FeishuNotifier::instance().sendMessage(msg.str());

                // 通知发出后移出数组
                {
                    std::lock_guard<std::mutex> lock(mu_);
                    cond2Set_.erase(sym);
                    // LOG_INFO("已从条件2数组移除: " + sym);
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::minutes(5));
    }
}
