// 数据加载与解析
#pragma once
#include <string>
#include <vector>

struct OIDataPoint {
    std::string symbol;
    double oi;
    double value;
    double supply;
    int64_t ts;
};

class DataLoader {
public:
    std::vector<OIDataPoint> loadFromJsonString(const std::string& jsonStr);
    std::vector<OIDataPoint> loadFromFile(const std::string& path);
};
