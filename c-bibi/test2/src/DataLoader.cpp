#include "DataLoader.h"
#include "nlohmann/json.hpp"
#include <fstream>
#include <iostream>
#include <algorithm>

using json = nlohmann::json;

std::vector<OIDataPoint> DataLoader::loadFromJsonString(const std::string &jsonStr) {
    std::vector<OIDataPoint> out;
    auto j = json::parse(jsonStr);
    if (!j.is_array()) return out;

    for (auto &el : j) {
        OIDataPoint p;
        p.symbol = el.value("symbol", std::string("UNKNOWN"));
        p.oi = std::stod(el.value("sumOpenInterest", "0"));
        p.value = std::stod(el.value("sumOpenInterestValue", "0"));
        p.supply = std::stod(el.value("CMCCirculatingSupply", "0"));
        p.ts = el.value("timestamp", int64_t(0));
        out.push_back(p);
    }
    std::sort(out.begin(), out.end(), [](auto &a, auto &b){ return a.ts < b.ts; });
    return out;
}

std::vector<OIDataPoint> DataLoader::loadFromFile(const std::string &path) {
    std::ifstream ifs(path);
    if (!ifs) {
        std::cerr << "Failed to open file: " << path << std::endl;
        return {};
    }
    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    return loadFromJsonString(content);
}
