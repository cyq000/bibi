#include "BinanceAPI.h"
#include "Logger.h"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <chrono>
#include <sstream>

using json = nlohmann::json;

// libcurl 回调函数：写入内存
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

BinanceAPI::BinanceAPI() {
    curl_global_init(CURL_GLOBAL_ALL);
}

std::string BinanceAPI::httpGet(const std::string& url) {
    CURL* curl = curl_easy_init();
    if (!curl) throw std::runtime_error("CURL init failed");

    std::string readBuffer;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); // 忽略证书
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        curl_easy_cleanup(curl);
        throw std::runtime_error("CURL request failed: " + std::string(curl_easy_strerror(res)));
    }

    curl_easy_cleanup(curl);
    return readBuffer;
}

// 获取符合条件的合约交易对
std::vector<std::string> BinanceAPI::getSymbolsWithVolume(long minVol, long maxVol) {
    std::vector<std::string> symbols;
    std::string url = "https://fapi.binance.com/fapi/v1/ticker/24hr";
    std::string resp = httpGet(url);

    auto data = json::parse(resp);
    for (auto& item : data) {
        try {
            std::string symbol = item["symbol"];
            double volume = std::stod(item["quoteVolume"].get<std::string>());

            // 只监控 USDT 结算的交易对
            if (symbol.size() > 4 && symbol.substr(symbol.size() - 4) == "USDT") {
                if (volume >= minVol && volume <= maxVol) {
                    symbols.push_back(symbol);
                }
            }
        } catch (...) {
            continue;
        }
    }

    LOG_INFO("筛选出 " + std::to_string(symbols.size()) + " 个合约交易对");
    return symbols;
}

// 安全获取 double
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

//https://fapi.binance.com/futures/data/openInterestHist?symbol=LTCUSDT&period=2h&limit=10;
//https://developers.binance.com/docs/zh-CN/derivatives/usds-margined-futures/market-data/rest-api/Open-Interest-Statistics
// 条件1：2小时级别，持仓量稳定
// ConditionResult BinanceAPI::checkCondition1(const std::string& symbol) {
//     ConditionResult result;

//     std::ostringstream url;
//     url << "https://fapi.binance.com/futures/data/openInterestHist?symbol="
//         << symbol << "&period=2h&limit=10";

//     try {
//         std::string resp = httpGet(url.str());
//         auto data = json::parse(resp);

//         //  // 打印解析后的前 3 条数据
//         // for (size_t i = 0; i < std::min<size_t>(3, data.size()); i++) {
//         //     LOG_INFO("Parsed json[" + std::to_string(i) + "]: " + data[i].dump());
//         // }

//         if (data.size() < 10) {
//             // LOG_WARN("Condition1 skipped (" + symbol + "): insufficient data " + std::to_string(data.size()));
//             return result;
//         }

//         // // 打印每根数据
//         // LOG_INFO("Condition1 raw data (" + symbol + "):");
//         // for (size_t i = 0; i < data.size(); i++) {
//         //     double sumOpenInterest = getDoubleSafe(data[i], "sumOpenInterest");
//         //     double sumOpenInterestValue = getDoubleSafe(data[i], "sumOpenInterestValue");
//         //     //std::string ts = data[i].value("timestamp", "N/A");
//         //     LOG_INFO(" sumOpenInterest=" + std::to_string(sumOpenInterest) +
//         //              " sumOpenInterestValue=" + std::to_string(sumOpenInterestValue));
//         // }

//         // 前3根平均持仓量
//         double avg = 0.0;
//         for (size_t i = 0; i < 3; i++) {
//             avg += getDoubleSafe(data[i], "sumOpenInterest");
//         }
//         avg /= 3.0;
//         // LOG_INFO("Condition1 (" + symbol + "): avg(前3根)=" + std::to_string(avg));

//         // 检查最近10根是否在 ±2% 波动范围内
//         for (size_t i = 0; i < 10; i++) {
//             double oi = getDoubleSafe(data[i], "sumOpenInterest");
//             double diff = std::abs(oi - avg) / avg;
//             // LOG_INFO("  [" + std::to_string(i) + "] oi=" + std::to_string(oi) +
//             //          " diff=" + std::to_string(diff * 100) + "%");
//             if (diff > 0.02) {
//                 // LOG_INFO("Condition1 (" + symbol + "): 不满足 (波动超出2%)");
//                 return result;
//             }
//         }

//         result.triggered = true;
//         result.avgOI = avg;
//         result.lastOI = getDoubleSafe(data.back(), "sumOpenInterest");
//         result.percentDiff = (result.lastOI - avg) / avg; 
//         result.triggerTime = std::time(nullptr);// 记录当前时间

//         // char buf[64];
//         // std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&result.triggerTime));
//         // LOG_INFO("Condition1 (" + symbol + "): 满足 ✅ lastOI=" + std::to_string(result.lastOI) +
//         //          " percentDiff=" + std::to_string(result.percentDiff * 100) + "%" + " time=" + std::string(buf));

//         return result;

//     } catch (std::exception& e) {
//         LOG_ERROR("checkCondition1 error (" + symbol + "): " + std::string(e.what()));
//         return result;
//     }
// }

ConditionResult BinanceAPI::checkCondition1(const std::string& symbol) {
    try {
        auto data = fetchOpenInterestData(symbol, "2h", 10);
        return evaluateStableOI(data);
    } catch (std::exception& e) {
        //LOG_ERROR("checkCondition1 error (" + symbol + ")");
        LOG_ERROR("checkCondition1 error (" + symbol + "): " + std::string(e.what()));
        return ConditionResult{};
    }
}

// 条件2：5分钟级别，持仓量明显增加
ConditionResult BinanceAPI::checkCondition2(const std::string& symbol) {
    ConditionResult result;

    std::ostringstream url;
    url << "https://fapi.binance.com/futures/data/openInterestHist?symbol="
        << symbol << "&period=5m&limit=4";  // 4个5m = 20分钟
    try {
        std::string resp = httpGet(url.str());
        auto data = json::parse(resp);

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

        if (diff > 0.04) {
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

// 拉取 open interest 数据
std::vector<json> BinanceAPI::fetchOpenInterestData(const std::string &symbol, const std::string &period, int limit)
{
    std::ostringstream url;
    url << "https://fapi.binance.com/futures/data/openInterestHist?symbol="
        << symbol << "&period=" << period << "&limit=" << limit;

    std::string resp = httpGet(url.str());

    if (resp.empty())
    {
        LOG_WARN("fetchOpenInterestData empty response for " + symbol + " " + period);
        std::cerr << "Error: empty response for " << symbol << " " << period << std::endl;
        return {}; // 返回空结果，不解析
    }

    try
    {
        auto data = json::parse(resp);
        return data;
    }
    catch (const json::parse_error &e)
    {
        LOG_WARN("fetchOpenInterestData JSON parse error for " + symbol + " " + period + ": " + e.what());
        std::cerr << "JSON parse error: " << e.what()
                  << " | Response: " << resp.substr(0, 200) << std::endl;
        return {};
    }
}

// 判断持仓量是否稳定（±2% 波动范围）
ConditionResult BinanceAPI::evaluateStableOI(const std::vector<json>& data) {
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
void BinanceAPI::analyzeOITrend(const std::vector<double>& oiData, ConditionResult& conditionResult) {
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


std::vector<double> BinanceAPI::fetchOIHistory(const std::string& symbol, const ConditionResult& conditionResult) {
    std::vector<double> oiData{};

    std::ostringstream url;
    url << "https://fapi.binance.com/futures/data/openInterestHist?symbol="
        << symbol << "&period=12h&limit=10";

    try {
        std::string resp = httpGet(url.str());
        auto data = json::parse(resp);

        //  // 打印解析后的前 3 条数据
        // for (size_t i = 0; i < std::min<size_t>(3, data.size()); i++) {
        //     LOG_INFO("Parsed json[" + std::to_string(i) + "]: " + data[i].dump());
        // }

        if (data.size() < 10) {
            LOG_WARN("fetchOIHistory skipped (" + symbol + "): insufficient data " + std::to_string(data.size()));
            return oiData;
        }
        for (size_t i = 0; i < 10; i++) {
            double oi = getDoubleSafe(data[i], "sumOpenInterest");
            oiData.push_back(oi);
        }

        return oiData;

    }catch (std::exception& e) {
        LOG_ERROR("checkCondition3 error (" + symbol + "): " + std::string(e.what()));
        return oiData;
    }
}
