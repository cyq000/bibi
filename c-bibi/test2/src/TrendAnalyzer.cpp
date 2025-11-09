#include "TrendAnalyzer.h"
#include <cmath>
#include <algorithm>

static double clamp(double x, double a, double b) {
    return std::max(a, std::min(b, x));
}

TrendSignal TrendAnalyzer::analyze(const std::vector<OIFeature> &features) {
    TrendSignal s{};
    s.stage = MarketStage::UNKNOWN;
    s.hasSignal = false;
    s.direction = "neutral";
    s.confidence = 0.0;

    if (features.size() < 2) return s;

    size_t N = std::min<size_t>(features.size(), 5);
    std::vector<OIFeature> w(features.end() - N, features.end());

    int consecPosOI = 0;
    for (int i = (int)w.size()-1; i >= 0; --i) {
        if (w[i].oiChangeRate > 0.01) consecPosOI++;
        else break;
    }

    double avgOIChange = 0, avgValChange = 0, avgQ = 0;
    for (auto &f: w) {
        avgOIChange += f.oiChangeRate;
        avgValChange += f.valueChangeRate;
        avgQ += f.qualityQ;
    }
    avgOIChange /= w.size();
    avgValChange /= w.size();
    avgQ /= w.size();

    if (avgOIChange > 0.01 && avgValChange > 0.01)
        s.stage = (avgOIChange > 0.05) ? MarketStage::ACCELERATION : MarketStage::BUILD_UP;
    else if (avgOIChange < -0.01 && avgValChange < -0.01)
        s.stage = MarketStage::SHAKEOUT;
    else
        s.stage = MarketStage::BALANCE;

    if (consecPosOI >= 2 && avgQ > 0.7 && avgQ < 1.3) {
        s.hasSignal = true;
        s.direction = avgValChange > 0 ? "long" : "short";
        s.confidence = clamp((fabs(avgOIChange)+fabs(avgValChange))/0.2, 0.0, 1.0);
    }

    if (avgQ > 1.6) s.stage = MarketStage::OVERHEAT;

    return s;
}
