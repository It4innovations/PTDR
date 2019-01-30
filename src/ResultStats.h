#pragma once

#include <map>
#include <ostream>
#include <vector>
#include <numeric>

class ResultStats {
public:
    /**
     * Constructor, computes simple statistics for the travel times
     * @param travelTimes vector of travel times
     * @param inputPercentiles percentile values to obtain
     */
    ResultStats(std::vector<float> &travelTimes,
                const std::vector<float> inputPercentiles = {0.05, 0.1, 0.25, 0.5, 0.75, 0.9, 0.95});

    /**
     * Sample deviation
     */
    double sampleDev;

    /**
     * Variation coefficient
     */
    double variationCoeff;

    /**
     * Sample mean
     */
    double mean;

    /**
     * Percentiles
     */
    std::map<float, double> percentiles;

    /**
     * Overloaded stream write operator for simple readable output
     */
    friend std::ostream &operator<<(std::ostream &os, const ResultStats &st);
};