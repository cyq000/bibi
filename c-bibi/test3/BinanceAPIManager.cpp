#include "BinanceAPIManager.h"
#include "Logger.h"
#include <cmath>
#include <ctime>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// 安全获取 string 转 double
double getDoubleSafe(const json& j, const std::string& key) {
    try {
        if (j[key].is_string()) {
            return std::stod(j[key].get<std::string>());
        } else if (j[key].is_number()) {
            return j[key].get<double>();
        } else {
            throw std::runtime_error("Unexpected JSON type for key: " + key);
        }
    } catch (const std::exception& e) {
        LOG_ERROR("getDoubleSafe error for key " + key + ": " + e.what());
        return 0.0;
    }
}

void BinanceAPIManager::init()
{
    // 线程1
    m_binanceAPI.getSymbolsWithVolume(20000000, 60000000);   // 2千万到6千万

    // 线程2
    m_binanceAPI.getSymbolsWithVolume(60000000, 600000000);  // 6千万到6亿
}

ConditionResult BinanceAPIManager::checkCondition1(const std::string& symbol) {
    try {
        auto data = m_binanceAPI.fetchOpenInterestData(symbol, "2h", 10);
        return evaluateStableOI(data);
    } catch (std::exception& e) {
        //LOG_ERROR("checkCondition1 error (" + symbol + ")");
        LOG_ERROR("checkCondition1 error (" + symbol + "): " + std::string(e.what()));
        return ConditionResult{};
    }
}


// 条件2：5分钟级别，持仓量明显增加
ConditionResult BinanceAPIManager::checkCondition2(const std::string& symbol,const std::string& period,int increaseRate, int limit) {
    ConditionResult result;

    try {
        auto data = m_binanceAPI.fetchOpenInterestData(symbol, period, limit);
        if (data.size() < 4) return result;

        // 前3根平均持仓量
        double avg = 0.0;
        for (size_t i = 0; i < 3; i++) {
            avg += std::stod(data[i]["sumOpenInterest"].get<std::string>());
        }
        avg /= 3.0;

        // 最新持仓量
        double lastOI = std::stod(data.back()["sumOpenInterest"].get<std::string>());

        double diff = (lastOI - avg) / avg;

        // if (diff > 0.04)
        // {
        //     LOG_INFO("Condition2 (" + symbol + "): 满足 ✅ lastOI=" +
        //              std::to_string(lastOI) + " avg=" + std::to_string(avg) +
        //              " percentDiff=" + std::to_string(diff * 100) + "%");
        // }

        if (diff > increaseRate) {
            result.triggered = true;
            result.avgOI = avg;
            result.lastOI = lastOI;
            result.percentDiff = diff;
        }
        return result;

    } catch (std::exception& e) {
        LOG_ERROR("checkCondition2 error: " + std::string(e.what()));
        return result;
    }
}

// 判断持仓量是否稳定（±2% 波动范围）
ConditionResult BinanceAPIManager::evaluateStableOI(const std::vector<json>& data) {
    ConditionResult result;

    if (data.size() < 10) {
        return result; // 数据不足
    }

    // 前3根平均持仓量
    double avg = 0.0;
    for (size_t i = 0; i < 3; i++) {
        avg += getDoubleSafe(data[i], "sumOpenInterest");
    }
    avg /= 3.0;

    // 检查最近10根是否在 ±2% 波动范围内
    for (size_t i = 0; i < 10; i++) {
        double oi = getDoubleSafe(data[i], "sumOpenInterest");
        double diff = std::abs(oi - avg) / avg;
        if (diff > 0.02) {
            return result; // 超过波动范围 → 不满足
        }
    }

    // 满足条件
    result.triggered = true;
    result.avgOI = avg;
    result.lastOI = getDoubleSafe(data.back(), "sumOpenInterest");
    result.percentDiff = (result.lastOI - avg) / avg;
    result.triggerTime = std::time(nullptr);

    return result;
}


// 简单趋势分析
void BinanceAPIManager::analyzeOITrend(const std::vector<double>& oiData, ConditionResult& conditionResult) {
    OIConditionResult result;

    if (oiData.size() < 8) {
        result.isGrowing = false;
        return ;
    }

    double start = oiData.front();
    double end   = oiData.back();
    result.growthRate = ((end - start) / start) * 100.0;

    // 简单线性回归 (y = a + b*x)，计算斜率 b
    size_t n = oiData.size();
    double sumX = 0, sumY = 0, sumXY = 0, sumX2 = 0;
    for (size_t i = 0; i < n; i++) {
        sumX += i;
        sumY += oiData[i];
        sumXY += i * oiData[i];
        sumX2 += i * i;
    }
    result.slope = (n * sumXY - sumX * sumY) / (n * sumX2 - sumX * sumX);

    // 归一化：把斜率转换为每步平均增长百分比
    double avgOI = sumY / n;
    result.slope = (avgOI > 0) ? (result.slope / avgOI * 100.0) : 0.0;

    // 判断是否稳步增长（归一化斜率 > 0 且增长率 > 阈值2%）
    result.isGrowing = (result.slope > 0 && result.growthRate > 2.0);
    conditionResult.oiTrend12h = result;

    // -------- 格式化输出 --------
    // std::ostringstream oss;
    // oss << std::fixed << std::setprecision(1);  //只保留小数点后一位

    // oss << "analyzeOITrend: slopeNorm=" << result.slope << "%/step"
    //     << " growthRate=" << result.growthRate << "%"
    //     << " isGrowing=" << (result.isGrowing ? "true" : "false");

    // LOG_INFO(oss.str());

    return ;
}
