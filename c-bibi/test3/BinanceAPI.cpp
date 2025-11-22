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

// 拉取 open interest 数据
std::vector<json> BinanceAPI::fetchOpenInterestData(const std::string &symbol, const std::string &period, int limit)
{
    // 例：https://fapi.binance.com/futures/data/openInterestHist?symbol=BTCUSDT&period=4h&limit=30

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

