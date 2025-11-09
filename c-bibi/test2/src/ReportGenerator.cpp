#include "ReportGenerator.h"
#include <iostream>
#include <fstream>

static std::string stageToStr(MarketStage s) {
    switch(s) {
        case MarketStage::ACCUMULATION: return "Accumulation";
        case MarketStage::BUILD_UP: return "BuildUp";
        case MarketStage::ACCELERATION: return "Acceleration";
        case MarketStage::OVERHEAT: return "Overheat";
        case MarketStage::SHAKEOUT: return "Shakeout";
        case MarketStage::BALANCE: return "Balance";
        default: return "Unknown";
    }
}

void ReportGenerator::printToConsole(const std::string &symbol,
                                     const TrendSignal &t,
                                     const RiskInfo &r,
                                     const std::vector<OIFeature> &features) {
    std::cout << "===== Trend Report for " << symbol << " =====\n";
    std::cout << "Stage: " << stageToStr(t.stage) << "\n";
    std::cout << "Signal: " << (t.hasSignal?"YES":"NO")
              << " Direction: " << t.direction
              << " Confidence: " << t.confidence << "\n";
    std::cout << "Risk: " << r.level << " (score=" << r.score << ") " << r.reason << "\n";
    if (!features.empty()) {
        auto &f = features.back();
        std::cout << "Latest ts=" << f.ts << " OI=" << f.oi
                  << " Value=" << f.value << " avgPrice=" << f.avgPrice
                  << " Q=" << f.qualityQ << "\n";
    }
    std::cout << "========================================\n";
}

void ReportGenerator::saveToFile(const std::string &symbol,
                                 const TrendSignal &t,
                                 const RiskInfo &r,
                                 const std::vector<OIFeature> &features) {
    std::ofstream ofs("report_" + symbol + ".txt");
    if (!ofs) return;
    ofs << "Stage: " << stageToStr(t.stage) << "\n";
    ofs << "Signal: " << (t.hasSignal?"YES":"NO") << " Dir=" << t.direction << " Conf=" << t.confidence << "\n";
    ofs << "Risk: " << r.level << " (" << r.score << ") " << r.reason << "\n";
}
