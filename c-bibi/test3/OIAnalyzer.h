#pragma once
#include <string>
#include <vector>
#include <cmath>

struct OIData {
    std::string symbol;
    double OI;
    double OIV;
    double price;
    long long ts;
};

struct AnalysisResult {
    int score = 0;
    int OI_up_count = 0;
    int Price_down_count = 0;
    std::string status;
};

class OIAnalyzer {
public:
    AnalysisResult analyze(const std::vector<OIData>& data);

    std::vector<OIData> parseJson(const std::string& raw);

private:
    double changeRate(double curr, double prev);
    int direction(double x);
    std::string classify(const AnalysisResult& r);
};
