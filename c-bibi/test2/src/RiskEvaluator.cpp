#include "RiskEvaluator.h"
#include <cmath>
#include <algorithm>

static double stddev(const std::vector<double>& v) {
    if (v.size() < 2) return 0.0;
    double mean = 0;
    for (double x: v) mean += x;
    mean /= v.size();
    double s = 0;
    for (double x: v) s += (x - mean)*(x - mean);
    return std::sqrt(s / (v.size()-1));
}

static double clamp(double x, double a, double b) {
    return std::max(a, std::min(b, x));
}

RiskInfo RiskEvaluator::evaluate(const std::vector<OIFeature> &features) {
    RiskInfo r{"low", "normal", 0.0};
    if (features.size() < 2) return r;

    std::vector<double> rates;
    double avgQ = 0;
    for (auto &f: features) {
        rates.push_back(f.oiChangeRate);
        avgQ += f.qualityQ;
    }
    avgQ /= features.size();
    double vol = stddev(rates);
    double score = clamp((vol*5.0) + fabs(avgQ-1.0)*0.6, 0.0, 1.0);
    r.score = score;

    if (score < 0.25) { r.level="low"; r.reason="stable OI"; }
    else if (score < 0.6) { r.level="medium"; r.reason="moderate volatility"; }
    else { r.level="high"; r.reason="high volatility or overheat"; }
    return r;
}
