#pragma once
#include "FeatureExtractor.h"
#include "TrendAnalyzer.h"
#include "RiskEvaluator.h"
#include <string>

class ReportGenerator {
public:
    void printToConsole(const std::string &symbol,
                        const TrendSignal &t,
                        const RiskInfo &r,
                        const std::vector<OIFeature> &features);

    void saveToFile(const std::string &symbol,
                    const TrendSignal &t,
                    const RiskInfo &r,
                    const std::vector<OIFeature> &features);
};
