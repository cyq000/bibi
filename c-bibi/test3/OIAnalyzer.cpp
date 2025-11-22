#include "OIAnalyzer.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

double OIAnalyzer::changeRate(double curr, double prev) {
    if (prev == 0) return 0.0;
    return (curr - prev) / prev;
}

int OIAnalyzer::direction(double x) {
    if (x > 0) return 1;
    if (x < 0) return -1;
    return 0;
}

AnalysisResult OIAnalyzer::analyze(const std::vector<OIData>& data) {
    AnalysisResult result;
    int n = data.size();
    if (n < 3) return result;

    int& score = result.score;
    int& OI_up_count = result.OI_up_count;
    int& Price_down_count = result.Price_down_count;

    for (int i = 1; i < n; i++) {
        const OIData& prev = data[i - 1];
        const OIData& curr = data[i];

        double dOI    = changeRate(curr.OI, prev.OI);
        double dOIV   = changeRate(curr.OIV, prev.OIV);
        double dPrice = changeRate(curr.price, prev.price);

        // ------------------------
        // ① OI 因子
        // ------------------------
        if (dOI > 0) score += 1;
        if (dOI > 0.05) score += 1;

        if (dOI > 0) OI_up_count++;
        else OI_up_count = 0;

        // ------------------------
        // ② 价格因子
        // ------------------------
        if (dPrice < 0) score += 1;
        if (dPrice < -0.03) score += 1;

        if (dPrice < 0) Price_down_count++;
        else Price_down_count = 0;

        // -------------------------
        // ③ OIV 上涨弱于 OI → 吸筹结构
        // -------------------------
        if (dOI > 0 && dOIV <= dOI * 0.3)
            score += 2;

        // -------------------------
        // ④ 方向差异（反向结构）
        // -------------------------
        int dirOI = direction(dOI);
        int dirPrice = direction(dPrice);
        if (dirOI == 1 && dirPrice == -1)
            score += 1;

        // -------------------------
        // ⑤ 加速度（第二阶导）
        // -------------------------
        if (i >= 2) {
            double prev_dOI = changeRate(prev.OI, data[i - 2].OI);
            if ((dOI - prev_dOI) > 0)
                score += 1;
        }
    }

    // -------------------------
    // ⑥ 连续结构加分
    // -------------------------
    if (OI_up_count >= 3)    score += 2;
    if (Price_down_count >= 3) score += 2;

    // 最终分类
    result.status = classify(result);

    return result;
}

std::string OIAnalyzer::classify(const AnalysisResult& r) {
    if (r.score >= 8)
        return "Strong Accumulation Signal";
    else if (r.score >= 6)
        return "Potential Accumulation";
    else
        return "Normal / No Signal";
}


// ===============================
// 解析 JSON → vector<OIData>
// ===============================
std::vector<OIData> OIAnalyzer::parseJson(const std::string& raw) {
    std::vector<OIData> out;

    json j = json::parse(raw);

    for (auto& item : j) {
        OIData d;
        d.symbol = item["symbol"];
        d.OI = stod(item["sumOpenInterest"].get<std::string>());
        d.OIV = stod(item["sumOpenInterestValue"].get<std::string>());
        d.ts = item["timestamp"];

        // 推算价格
        d.price = (d.OIV / d.OI);

        out.push_back(d);
    }

    return out;
}