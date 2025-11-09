#include "TrendMonitor.h"
#include <iostream>

void TrendMonitor::runOnceFromFile(const std::string &path) {
    auto data = loader.loadFromFile(path);
    runOnce(data);
}

void TrendMonitor::runOnceFromJsonString(const std::string &jsonStr) {
    auto data = loader.loadFromJsonString(jsonStr);
    runOnce(data);
}

void TrendMonitor::runOnce(const std::vector<OIDataPoint> &data) {
    if (data.empty()) {
        std::cerr << "No data\n";
        return;
    }
    std::string symbol = data.front().symbol;
    auto features = extractor.extract(data);
    auto trend = analyzer.analyze(features);
    auto risk = riskEval.evaluate(features);
    report.printToConsole(symbol, trend, risk, features);
    report.saveToFile(symbol, trend, risk, features);
}
