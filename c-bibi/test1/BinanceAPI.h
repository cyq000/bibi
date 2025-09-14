#ifndef BINANCE_API_H
#define BINANCE_API_H

#include <string>
#include <vector>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

struct ConditionResult {
    bool triggered;      // 是否满足条件
    double avgOI;        // 前3根平均持仓量
    double lastOI;       // 最新持仓量
    double percentDiff;  // 相差百分比
};

class BinanceAPI {
public:
    BinanceAPI();

    // 获取 24 小时成交额在区间内的所有合约币种
    std::vector<std::string> getSymbolsWithVolume(long minVol, long maxVol);

    // 条件1检查：2小时级别，持仓量基本不变
    ConditionResult checkCondition1(const std::string& symbol);

    // 条件2检查：10分钟级别，持仓量明显增加
    ConditionResult checkCondition2(const std::string& symbol);

private:
    std::string httpGet(const std::string& url);
};

#endif
