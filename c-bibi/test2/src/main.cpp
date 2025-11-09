#include "TrendMonitor.h"
#include <iostream>

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <json_file>\n";
        return 1;
    }
    TrendMonitor monitor;
    monitor.runOnceFromFile(argv[1]);
    return 0;
}
