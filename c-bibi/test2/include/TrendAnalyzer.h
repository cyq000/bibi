#pragma once
#include "FeatureExtractor.h"
#include <string>

enum class MarketStage {
    UNKNOWN, ACCUMULATION, BUILD_UP, ACCELERATION, OVERHEAT, SHAKEOUT, BALANCE
};

struct TrendSignal {
    MarketStage stage;
    bool hasSignal;
    std::string direction;
    double confidence;
};

class TrendAnalyzer {
public:
    TrendSignal analyze(const std::vector<OIFeature>& features);
};
