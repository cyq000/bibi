#ifndef WORKER_H
#define WORKER_H

#include <string>
#include <vector>
#include <unordered_set>
#include <thread>
#include <mutex>
#include <atomic>
#include "BinanceAPI.h"

class Worker {
public:
    Worker(BinanceAPI& api);
    void start(int numThreads);
    void stop();

private:
    // 工作线程1
    void condition1Worker(std::vector<std::string> symbols, int id);
    // 工作线程2
    void condition2Worker();
    // 公共线程
    void commonWorker();
    // 工作线程3
    void condition3Worker(std::vector<std::string> symbols);

    BinanceAPI& api_;
    std::vector<std::string> universe_;
    std::unordered_map<std::string, ConditionResult> cond2Map_;
    std::mutex mu_;
    std::mutex mu_3;    // 条件3专用
    std::atomic<bool> stop_{false};
    std::vector<std::thread> threads_;
};

#endif
