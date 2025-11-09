#ifndef BINANCE_API_H
#define BINANCE_API_H

#include <string>
#include <vector>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

// 持仓量趋势分析结果
struct OIConditionResult {
    bool isGrowing = false;      // 是否稳步增长
    double slope = 0.0;          // 线性回归斜率
    double growthRate = 0.0;     // 增长比例 (%)
    std::string timeframe;       // 时间周期 (12h / 1d)
    std::time_t triggerTime;     // 触发时间 (秒级时间戳)
    OIConditionResult() : isGrowing(false), slope(0.0), growthRate(0.0), timeframe(""), triggerTime(0) {}
};

struct ConditionResult {
    bool triggered;      // 是否满足条件
    double avgOI;        // 前3根平均持仓量
    double lastOI;       // 最新持仓量
    double percentDiff;  // 相差百分比
    std::time_t triggerTime; // 条件触发时间 (秒级时间戳)

    OIConditionResult oiTrend12h; // 12小时持仓量趋势分析结果
    OIConditionResult oiTrend1d;  // 1天持仓量趋势分析结果

    ConditionResult() : triggered(false), avgOI(0.0), lastOI(0.0), percentDiff(0.0), triggerTime(0) {}
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

    // 条件3检查：12小时、1天级别，持仓量稳步增长
    std::vector<double> fetchOIHistory(const std::string& symbol, const ConditionResult& conditionResult);

    std::vector<json> fetchOpenInterestData(const std::string& symbol, const std::string& period, int limit);
    ConditionResult evaluateStableOI(const std::vector<json>& data);
    void analyzeOITrend(const std::vector<double>& oiData, ConditionResult& conditionResult);

private:
    std::string httpGet(const std::string& url);
};

#endif
