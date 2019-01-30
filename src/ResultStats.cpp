#include "ResultStats.h"

#include <cmath>
#include <iomanip>

ResultStats::ResultStats(std::vector<float> &travelTimes, const std::vector<float> inputPercentiles) {

    // Onepass algorithm based on Mark Hoemmenn, "Computing the standard deviation efficiently", 2007.
    // http://suave_skola.varak.net/proj2/stddev.pdf

    double M = travelTimes[0];
    double Q = 0.0;
    for (size_t i = 0; i < travelTimes.size(); ++i) {
        Q += (i * std::pow(travelTimes[i] - M, 2)) / (i + 1);
        M += (travelTimes[i] - M) / (i + 1);

    }
    this->mean = M;
    if (travelTimes.size() > 1) {
        this->sampleDev = std::sqrt(Q / (travelTimes.size() - 1));
    } else {
        this->sampleDev = 0.0;
    }

    this->variationCoeff = this->sampleDev / this->mean;

    std::sort(travelTimes.begin(), travelTimes.end());
    for (const auto &p : inputPercentiles) {
        this->percentiles[p] = travelTimes[static_cast<int>(travelTimes.size() * p)];
    }
}

std::ostream &operator<<(std::ostream &os, const ResultStats &st) {
    os << "sample dev: " << st.sampleDev << "  mean: " << st.mean << "  variation coeff.: " << st.variationCoeff
       << std::endl;
    os << "Percentiles: " << std::endl;
    for (const auto &p : st.percentiles) {
        os << p.first * 100.0f << "% " << p.second << std::endl;
    }

    return os;
}
