#ifndef BINANCEAPI_MANAGER_H
#define BINANCEAPI_MANAGER_H
#include "BinanceAPI.h"

/*
* BinanceAPIManager类
* 提供Binance交易所API的管理和持仓量趋势分析功能
*/

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
    std::time_t triggerTime; // 满足条件的触发时间 (秒级时间戳)

    OIConditionResult oiTrend12h; // 12小时持仓量趋势分析结果
    OIConditionResult oiTrend1d;  // 1天持仓量趋势分析结果

    ConditionResult() : triggered(false), avgOI(0.0), lastOI(0.0), percentDiff(0.0), triggerTime(0) {}
};


class BinanceAPIManager {
public:
    static BinanceAPIManager& instance() {
        static BinanceAPIManager inst;
        return inst;
    }

    void init();

    /* ============特定条件判断============ */
    /*
    / 条件1检查：2小时级别，持仓量基本不变
    */
    ConditionResult checkCondition1(const std::string& symbol);

    /*
    * 条件2检查：（period）分钟级别，持仓量增加超过（increaseRate）
    */
    ConditionResult checkCondition2(const std::string& symbol,const std::string& period,int increaseRate, int limit = 4);

    /*
    * 判断持仓量是否稳定（±2% 波动范围）
    */
    ConditionResult evaluateStableOI(const std::vector<json>& data);

    /*
    * 简单趋势分析
    *
    */
    void analyzeOITrend(const std::vector<double>& oiData, ConditionResult& conditionResult);

private:
    BinanceAPIManager() = default;
    ~BinanceAPIManager() = default;
    BinanceAPIManager(const BinanceAPIManager&) = delete;
    BinanceAPIManager& operator=(const BinanceAPIManager&) = delete;

    BinanceAPI m_binanceAPI;
};

#endif // BINANCEAPI_MANAGER_H