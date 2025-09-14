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
ConditionResult BinanceAPI::checkCondition1(const std::string& symbol) {
    ConditionResult result{false, 0.0, 0.0, 0.0};

    std::ostringstream url;
    url << "https://fapi.binance.com/futures/data/openInterestHist?symbol="
        << symbol << "&period=2h&limit=10";

    try {
        std::string resp = httpGet(url.str());
        auto data = json::parse(resp);

        //  // 打印解析后的前 3 条数据
        // for (size_t i = 0; i < std::min<size_t>(3, data.size()); i++) {
        //     LOG_INFO("Parsed json[" + std::to_string(i) + "]: " + data[i].dump());
        // }

        if (data.size() < 10) {
            // LOG_WARN("Condition1 skipped (" + symbol + "): insufficient data " + std::to_string(data.size()));
            return result;
        }

        // // 打印每根数据
        // LOG_INFO("Condition1 raw data (" + symbol + "):");
        // for (size_t i = 0; i < data.size(); i++) {
        //     double sumOpenInterest = getDoubleSafe(data[i], "sumOpenInterest");
        //     double sumOpenInterestValue = getDoubleSafe(data[i], "sumOpenInterestValue");
        //     //std::string ts = data[i].value("timestamp", "N/A");
        //     LOG_INFO(" sumOpenInterest=" + std::to_string(sumOpenInterest) +
        //              " sumOpenInterestValue=" + std::to_string(sumOpenInterestValue));
        // }

        // 前3根平均持仓量
        double avg = 0.0;
        for (size_t i = 0; i < 3; i++) {
            avg += getDoubleSafe(data[i], "sumOpenInterest");
        }
        avg /= 3.0;
        // LOG_INFO("Condition1 (" + symbol + "): avg(前3根)=" + std::to_string(avg));

        // 检查最近10根是否在 ±2% 波动范围内
        for (size_t i = 0; i < 10; i++) {
            double oi = getDoubleSafe(data[i], "sumOpenInterest");
            double diff = std::abs(oi - avg) / avg;
            // LOG_INFO("  [" + std::to_string(i) + "] oi=" + std::to_string(oi) +
            //          " diff=" + std::to_string(diff * 100) + "%");
            if (diff > 0.02) {
                // LOG_INFO("Condition1 (" + symbol + "): 不满足 (波动超出2%)");
                return result;
            }
        }

        result.triggered = true;
        result.avgOI = avg;
        result.lastOI = getDoubleSafe(data.back(), "sumOpenInterest");
        result.percentDiff = (result.lastOI - avg) / avg;

        // LOG_INFO("Condition1 (" + symbol + "): 满足 ✅ lastOI=" +
        //          std::to_string(result.lastOI) +
        //          " percentDiff=" + std::to_string(result.percentDiff * 100) + "%");

        return result;

    } catch (std::exception& e) {
        LOG_ERROR("checkCondition1 error (" + symbol + "): " + std::string(e.what()));
        return result;
    }
}



// 条件2：5分钟级别，持仓量明显增加
ConditionResult BinanceAPI::checkCondition2(const std::string& symbol) {
    ConditionResult result{false, 0.0, 0.0, 0.0};

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