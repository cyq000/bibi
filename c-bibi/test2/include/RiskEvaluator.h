#pragma once
#include "FeatureExtractor.h"
#include <string>

struct RiskInfo {
    std::string level;
    std::string reason;
    double score;
};

class RiskEvaluator {
public:
    RiskInfo evaluate(const std::vector<OIFeature>& features);
};
