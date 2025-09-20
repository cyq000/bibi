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
    void condition1Worker(std::vector<std::string> symbols, int id);
    void condition2Worker();
    void commonWorker();

    BinanceAPI& api_;
    std::vector<std::string> universe_;
    std::unordered_map<std::string, ConditionResult> cond2Map_;
    std::mutex mu_;
    std::atomic<bool> stop_{false};
    std::vector<std::thread> threads_;
};

#endif
