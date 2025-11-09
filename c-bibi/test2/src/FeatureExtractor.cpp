#include "FeatureExtractor.h"
#include <cmath>

static double safeDiv(double a, double b, double fallback=0.0) {
    if (b == 0.0) return fallback;
    return a / b;
}

std::vector<OIFeature> FeatureExtractor::extract(const std::vector<OIDataPoint> &data) {
    std::vector<OIFeature> features;
    if (data.empty()) return features;
    features.reserve(data.size());
    for (size_t i = 0; i < data.size(); ++i) {
        OIFeature f{};
        f.ts = data[i].ts;
        f.oi = data[i].oi;
        f.value = data[i].value;
        if (i == 0) {
            f.oiChangeRate = f.valueChangeRate = f.avgPriceChange = f.qualityQ = 0.0;
            f.avgPrice = safeDiv(f.value, f.oi, 0.0);
        } else {
            const auto &p = data[i-1];
            f.oiChangeRate = safeDiv(f.oi - p.oi, p.oi, 0.0);
            f.valueChangeRate = safeDiv(f.value - p.value, p.value, 0.0);
            f.avgPrice = safeDiv(f.value, f.oi, 0.0);
            double prevAvg = safeDiv(p.value, p.oi, 0.0);
            f.avgPriceChange = safeDiv(f.avgPrice - prevAvg, prevAvg, 0.0);
            if (fabs(f.oiChangeRate) < 1e-9)
                f.qualityQ = fabs(f.valueChangeRate) < 1e-9 ? 1.0 : 100.0;
            else
                f.qualityQ = f.valueChangeRate / f.oiChangeRate;
        }
        features.push_back(f);
    }
    return features;
}
