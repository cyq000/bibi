#pragma once
#include "DataLoader.h"
#include <vector>

struct OIFeature {
    int64_t ts;
    double oi;
    double value;
    double oiChangeRate;
    double valueChangeRate;
    double avgPrice;
    double avgPriceChange;
    double qualityQ;
};

class FeatureExtractor {
public:
    std::vector<OIFeature> extract(const std::vector<OIDataPoint> &data);
};
