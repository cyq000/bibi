#pragma once
#include "DataLoader.h"
#include "FeatureExtractor.h"
#include "TrendAnalyzer.h"
#include "RiskEvaluator.h"
#include "ReportGenerator.h"

class TrendMonitor {
public:
    void runOnceFromFile(const std::string &path);
    void runOnceFromJsonString(const std::string &jsonStr);

private:
    void runOnce(const std::vector<OIDataPoint> &data);

    DataLoader loader;
    FeatureExtractor extractor;
    TrendAnalyzer analyzer;
    RiskEvaluator riskEval;
    ReportGenerator report;
};
