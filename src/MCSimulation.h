#pragma once

#include <list>
#include <vector>
#include <string>

namespace Routing {
    class MCSimulation {
    public:
        /**
         * Constructor, loads speed profiles from the supplied files
         * @param segmentsFile CSV file with segment IDs and lengths in meters
         * @param profilesDir Directory with CSV files with probabilistic speed profiles for the segments
         */
        MCSimulation(const std::string segmentsFile, const std::string profilesDir);

        /**
         * Destructor frees memory for the loaded segments
         */
        ~MCSimulation();

        /**
         * Loads data from the supplied files
         * @param segmentsFile CSV file with segment IDs and lengths in meters
         * @param profilesDir Directory with CSV files with probabilistic speed profiles for the segments
         */
        void LoadSegments(const std::string segmentsFile, const std::string profilesDir);

        /**
         * Runs the actual simulation
         * @param samples number of samples to take
         * @param startDay departure day (0-6)
         * @param startHour departure hour (0-23)
         * @param startMinute departure minute (0-59)
         * @param all if true, iterate over all possible departure intervals
         * @return vector of travel times of size equal to the samples paramter
         */
        std::vector<float>
        RunMonteCarloSimulation(const int samples, const int startDay, const int startHour, const int startMinute,
                                bool all) const;

        /**
         * Get optimal travel time for the supplied route.
         * @param startDay departure day (0-6)
         * @param startHour departure hour (0-23)
         * @param startMinute departure minute (0-59)
         * @param all if true, iterate over all possible departure intervals
         * @return vector of optimal travel times, if all is true then size is equal to number of intervals, otherwise 1
         */
        std::vector<float>
        ComputeOptimalTravelTime(const int startDay, const int startHour, const int startMinute, bool all) const;

    private:
        /**
         * Load single speed profile from the supplied CSV file
         * @param speedProfileFile path to the CSV file
         * @param speedProfileData pointer to the beginning of memory to store the profile data in
         * @param freeflowSpeed default speed to be used when segment does not have a profile
         */
        void LoadSpeedProfile(const std::string speedProfileFile, float **speedProfileData, float freeflowSpeed);

        /**
         * Simulate pass of a single car along the entire route - obtain single MC sample
         * @param startSeconds departure time in seconds from the beginning of the week
         * @param probs random indexes to use
         * @return random travel time in seconds
         */
        float GetRandomTravelTime(int startSeconds, int *probs) const;

        /**
         * Simulate pass of a single car along the entire route - using only first speed profile
         * @param startSeconds departure time in seconds from the beginning of the week
         * @return optimal travel time in seconds
         */
        float GetOptimalTravelTime(int startSeconds) const;

        /**
         * Members
         */

        /**
         * Length of time interval for which a single profile is valid in seconds
         */
        float m_secondInterval = 0;

        /**
         * Number of segments in the current route
         */
        int m_segmentCount = 0;

        /**
         * Linear array of speed profiles for all segments
         */
        float **m_speedProfiles = nullptr;

        /**
         * Lengths of the individual segments
         */
        int *m_lengths = nullptr;

        /**
         * Segment freeflow speeds
         */
        float *m_freeSpeeds = nullptr;
    };
}
