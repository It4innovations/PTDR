#include <map>
#include <regex>
#include "Data.h"
#include "CSVReader.h"

#define PROFILE_FILE_NAME_SEP "_"

void
Routing::Data::WriteResultAll(std::vector<float> &result, const std::string &file, int samples, float secondInterval) {
    std::ofstream rfile(file);
    int intervalsPerDay = 86400 / secondInterval;
    // Header
    rfile << "day;interval;1";
    for (int j = 2; j <= samples; ++j) {
        rfile << ";" << j;
    }
    rfile << std::endl;
    // Data
    for (int d = 0; d < 7; ++d) {
        for (int i = 0; i < intervalsPerDay; ++i) {
            rfile << d << ";" << i << ";";
            for (int s = 0; s < samples; ++s) {
                if (s != 0)
                    rfile << ";";
                rfile << result[(d * intervalsPerDay * samples) + (i * samples) + s];
            }
            rfile << std::endl;
        }
    }
    rfile.close();
}

void Routing::Data::WriteResultSingle(std::vector<float> &result, const std::string &file) {
    std::ofstream rfile(file);
    for (const auto &r : result) {
        rfile << r << "\n";
    }
    rfile.flush();
    rfile.close();
}
