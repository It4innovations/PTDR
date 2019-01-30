#include "MCSimulation.h"
#include <omp.h>
#include <iostream>
#include <fstream>
#include "CSVReader.h"
#include <dirent.h>
#include <map>
#include <cmath>

#if defined USE_MKL || defined __INTEL_COMPILER // If we are using Intel compiler, MKL will be most certainly available as well
#define INTEL_RND
#include <mkl_vsl.h>
#include <mkl.h>
#else
#define GNU_RND
#include <random>
#endif

#define RANDS_PER_SEGMENT 5 // Margin of random numbers per segment generated
#define PROFILE_FILE_NAME_SEP "_" // Separator in file names
#define INDEX_RESOLUTION 100 // Size of the speed profile array

Routing::MCSimulation::MCSimulation(const std::string segmentsFile, const std::string profilesDir) {

#ifdef INTEL_RND
    std::cout << "RNG: Intel MKL";
#else
    std::cout << "RNG: GNU";
#endif
    std::cout << std::endl;

    LoadSegments(segmentsFile, profilesDir);
}

Routing::MCSimulation::~MCSimulation() {
    if (m_lengths != nullptr)
        delete[] m_lengths;

    if (m_freeSpeeds != nullptr)
        delete[] m_freeSpeeds;

    if (m_speedProfiles != nullptr) {
        for (int i = 0; i < m_segmentCount; ++i) {
            delete[] m_speedProfiles[i];
        }
        delete[] m_speedProfiles;
    }
}

std::vector<float>
Routing::MCSimulation::RunMonteCarloSimulation(int samples, int startDay, int startHour, int startMinute,
                                               bool all) const {
    int intervalsPerDay = 86400 / m_secondInterval;
    std::vector<float> travelTimes;
#pragma omp parallel shared(travelTimes)
    {
        int tid = omp_get_thread_num();
        int probsSize = m_segmentCount * RANDS_PER_SEGMENT;
        int *probs = new int[probsSize];

#ifdef INTEL_RND
        VSLStreamStatePtr rndStream;
        vslNewStream(&rndStream, VSL_BRNG_MT2203 + tid, std::rand());
        std::cout<<"using intel"<<std::cout;
#else
        std::mt19937_64 rnd((unsigned long long) (std::rand() + tid));
#endif


        if (all) {
#pragma omp single
            {
                travelTimes.resize(intervalsPerDay * 7 * samples);
            }

#pragma omp for schedule(dynamic)
            for (int s = 0; s < samples; ++s) {
                for (int d = 0; d < 7; ++d) {
                    for (int i = 0; i < intervalsPerDay; ++i) {

#ifdef INTEL_RND
                        viRngUniform(VSL_RNG_METHOD_UNIFORM_STD, rndStream, probsSize, probs, 0, INDEX_RESOLUTION);
#else
                        std::uniform_int_distribution<int> dist(0, INDEX_RESOLUTION - 1);
                        for (int r = 0; r < probsSize; ++r) {
                            probs[r] = dist(rnd);
                        }
#endif
                        int secs = (d * 86400) + (i * 900);
                        travelTimes[(d * intervalsPerDay * samples) + (i * samples) +
                                    s] = Routing::MCSimulation::GetRandomTravelTime(secs, probs);
                    }
                }
            }
        } else {
#pragma omp single
            {
                travelTimes.resize(samples);
            }

#pragma omp for schedule(dynamic)
            for (int s = 0; s < samples; ++s) {
#ifdef INTEL_RND
                viRngUniform(VSL_RNG_METHOD_UNIFORM_STD, rndStream, probsSize, probs, 0, INDEX_RESOLUTION);
#else
                std::uniform_int_distribution<int> dist(0, INDEX_RESOLUTION - 1);
                for (int r = 0; r < probsSize; ++r) {
                    probs[r] = dist(rnd);
                }
#endif

                int secs = (startDay * 86400) + (startHour * 3600) + (startMinute * 60);
                travelTimes[s] = Routing::MCSimulation::GetRandomTravelTime(secs, probs);
            }
        }

        delete[] probs;
    }
    return travelTimes;
}

std::vector<float>
Routing::MCSimulation::ComputeOptimalTravelTime(int startDay, int startHour, int startMinute, bool all) const {
    int intervalsPerDay = 86400 / m_secondInterval;
    std::vector<float> travelTimes;
    if (all) {
#pragma omp parallel shared(travelTimes)
        {
#pragma omp single
            {
                travelTimes.resize(intervalsPerDay * 7);
            }

#pragma omp for schedule(dynamic)
            for (int d = 0; d < 7; ++d) {
                for (int i = 0; i < intervalsPerDay; ++i) {
                    int secs = (d * 86400) + (i * 900);
                    travelTimes[(d * intervalsPerDay) + i] = Routing::MCSimulation::GetOptimalTravelTime(secs);
                }
            }
        }
    } else {
        travelTimes.resize(1);
        int secs = (startDay * 86400) + (startHour * 3600) + (startMinute * 60);
        travelTimes[0] = Routing::MCSimulation::GetOptimalTravelTime(secs);
    }
    return travelTimes;
}

float Routing::MCSimulation::GetRandomTravelTime(int startSeconds, int *probs) const {
    int pIdx = 0;
    float totalTravelTime = 0;
    float currentSeconds = static_cast<float>(startSeconds);
    for (int s = 0; s < m_segmentCount; ++s) {
        float remainingLength = m_lengths[s];
        while (remainingLength > 0) {
            int currentInterval = currentSeconds / m_secondInterval;
            int idx = (currentInterval * INDEX_RESOLUTION) + probs[pIdx++];
            float velocity = m_speedProfiles[s][idx];
            float currentTravelTime = remainingLength / velocity; // Rounded to seconds
            float newSeconds = currentSeconds + currentTravelTime;
            int newInterval = newSeconds / m_secondInterval;

            // Check if travel time was within single time interval (i.e. single speed profile)
            // Suggest via builtin_expect that this condition will be false in most cases (based on the data)
            if (__builtin_expect(newInterval != currentInterval, 0)) {
                // If not, compute distance travelled in time remaining to next interval
                int secsToNext = ((currentInterval + 1) * m_secondInterval) - currentSeconds;
                remainingLength -= (velocity * secsToNext);
                totalTravelTime += secsToNext;

                // Resolve wrapping of the week
                if (newSeconds < 604800) {
                    // Travel time is still within one week
                    currentSeconds += currentTravelTime;
                } else {
                    // We may have wrapped around one or more weeks... (better not)
                    int weekOverlap = newSeconds / 604800; // How many weeks are within this overlap
                    currentSeconds = newSeconds - (weekOverlap *
                                                   604800); // Set remainder as new current secs from the beginning of the week
                }
            } else {
                remainingLength = 0; // Go to next segment
                totalTravelTime += currentTravelTime;
                currentSeconds += currentTravelTime;
            }
        }
    }
    return totalTravelTime;
}

void Routing::MCSimulation::LoadSpeedProfile(const std::string speedProfileFile, float **speedProfileData,
                                             float freeflowSpeed) {
    const float oneDiv3point6 = 1 / 3.6; // For conversion of km/h to m/s
    std::ifstream profileFileStream(speedProfileFile);
    if (!profileFileStream.is_open()) {
        std::cerr << "ERROR: Cannot open profile file " << speedProfileFile << std::endl;
    }

    CSVReader row('|');

    // Get intervals from first two rows of the file
    std::vector<int> hours(2);
    std::vector<int> min(2);

    int cnt = 0;
    while (profileFileStream >> row && cnt < 2) {
        hours[cnt] = std::stoi(row[1]);
        min[cnt] = std::stoi(row[2]);
        cnt++;
    }

    if (hours[0] == hours[1]) {
        // Interval is less than 1 hour
        m_secondInterval = 60 * (min[1] - min[0]);
    } else {
        // Interval is longer than 1 hour
        m_secondInterval = (hours[1] - hours[0]) * 3600;
    }

    // Get profile count from number of columns in file
    int profilesPerInterval =
            (row.size() - 3) / 2; // Skip first three columns, divide by two values in single SpeedProbability
    int intervalsPerDay = 86400 / m_secondInterval;
    *speedProfileData = new float[INDEX_RESOLUTION * 7 * intervalsPerDay];

    // Rewind stream
    profileFileStream.seekg(0);

    while (profileFileStream >> row) {
        // Day of week
        int currentDay = -1;
        std::string currentDayString = row[0];

        if (currentDayString == "Monday") currentDay = 0; // Monday
        else if (currentDayString == "Tuesday") currentDay = 1; // Tuesday
        else if (currentDayString == "Wednesday") currentDay = 2; // Wednesday
        else if (currentDayString == "Thursday") currentDay = 3; //
        else if (currentDayString == "Friday") currentDay = 4;
        else if (currentDayString == "Saturday") currentDay = 5;
        else if (currentDayString == "Sunday") currentDay = 6;

        if (currentDay == -1)
            std::cerr << "ERROR: Unknown day of week string " << currentDayString << std::endl;

        // Time interval
        int hour = std::stoi(row[1]);
        int min = std::stoi(row[2]);
        int currentProfileIdx =
                (((hour * 3600) + (min * 60)) / (int) m_secondInterval) + (currentDay * intervalsPerDay);

        int startIdx = 0;
        float probabilitySum = 0.0f;
        float errSum = 0.0f;
        for (int i = 0; i < profilesPerInterval; i++) {

            int rowIdx = i * 2 + 3; // Skip first three columns
            std::string velocity_str = row[rowIdx];
            std::string probability_str = row[rowIdx + 1];

            if (velocity_str == "NaN" || probability_str == "NaN") {
                // Probability is nan
                //std::cout << "Day: " << currentDay << ", Index: " << i << " no probability" << std::endl;
                continue;
            }

            float velocity = std::stof(velocity_str) * oneDiv3point6; // Convert velocity from km/h to m/s
            float probability = std::stof(probability_str);

            if (std::floor(probability * INDEX_RESOLUTION) < 1 && probability > 0.0f) {
                std::cerr << "WARNING: Increase index resolution! (min. p: " << probability << " )" << std::endl;
            }

            if (velocity <= 0.0f && velocity > 300.0f) {
                std::cerr << "Invalid velocity value: " << velocity << std::endl;
            }

            if (probability < 0.0f && probability > 1.0f) {
                std::cerr << "Invalid probability value: " << probability << std::endl;
            }

            float idxP = INDEX_RESOLUTION * probability;
            int length = static_cast<int>(idxP);
            errSum += idxP - length;

            if (i == profilesPerInterval - 1) {
                length += std::round(errSum);
            }

            for (int j = startIdx; j < (startIdx + length); ++j) {
                int index = (currentProfileIdx * INDEX_RESOLUTION) + j;
                (*speedProfileData)[index] = velocity;
            }
            startIdx += length;
            probabilitySum += probability;
        }

        if ((1.0f - probabilitySum) > std::numeric_limits<float>::epsilon()) {
            std::cerr << "Invalid probability sum: " << probabilitySum << std::endl;
        }

        if (startIdx == 0) {
            std::cerr << "Day: " << currentDay << " no speed profile, using freeflow speed " << freeflowSpeed << "("
                      << speedProfileFile << ")" << std::endl;
            for (int j = 0; j < INDEX_RESOLUTION; ++j) {
                int index = (currentProfileIdx * INDEX_RESOLUTION) + j;
                (*speedProfileData)[index] = freeflowSpeed;
            }
        }
    }
    profileFileStream.close();
}

void Routing::MCSimulation::LoadSegments(const std::string segmentsFile, const std::string profilesDir) {

    // Load files in profile directory
    DIR *dirp = opendir(profilesDir.c_str());
    struct dirent *entry;
    std::map<std::string, std::string> profilesByTmcId;

    if (!dirp) {
        std::cerr << "ERROR: Cannot open directory " << profilesDir << std::endl;
    }

    char *fileName = new char[1024];
    while ((entry = readdir(dirp))) {
        if (entry->d_type == DT_REG) {
            // First field in the speed profile file name corresponds to segment ID
            char *token = strtok(strncpy(fileName, entry->d_name, 1024), PROFILE_FILE_NAME_SEP);
            profilesByTmcId.emplace(std::string(token), profilesDir + '/' + std::string(entry->d_name));
        }
    }

    delete[] fileName;
    m_segmentCount = profilesByTmcId.size();
    if (m_segmentCount < 1)
        std::cerr << "ERROR: No segments found in directory " << profilesDir << std::endl;

    // Load segments
    std::ifstream segmentFileStream(segmentsFile);
    // Allocate memory for lengths
    m_lengths = new int[m_segmentCount];
    m_freeSpeeds = new float[m_segmentCount];

    // Allocate memory for speed profiles
    m_speedProfiles = new float *[m_segmentCount];

    if (!segmentFileStream.is_open()) {
        std::cerr << "ERROR: Unable to open file " << segmentsFile << std::endl;
        std::exit(EXIT_FAILURE);
    } else {
        CSVReader row(';');
        int cnt = 0;
        segmentFileStream >> row; // Discard the header
        while (segmentFileStream >> row) {
            if (row.size() != 3) {
                std::cerr << "ERROR: Row " << cnt + 1 << " has invalid column count." << std::endl;
                continue;
            }
            std::string tmcId = row[0];
            if (profilesByTmcId.find(tmcId) == profilesByTmcId.end()) {
                std::cerr << "ERROR: Profile for segment " << tmcId << " not found in profile directory " << profilesDir
                          << std::endl;
                continue;
            }

            m_lengths[cnt] = std::stoi(row[1]);
            m_freeSpeeds[cnt] = std::stof(row[2]);
            LoadSpeedProfile(profilesByTmcId[tmcId], &m_speedProfiles[cnt], m_freeSpeeds[cnt]);
            cnt++;
        }
        segmentFileStream.close();
    }
}

float Routing::MCSimulation::GetOptimalTravelTime(int startSeconds) const {
    if (m_speedProfiles == nullptr || m_lengths == nullptr) {
        std::cerr << "ERROR: Profiles or segments missing" << std::endl;
    }

    int idx = (startSeconds / m_secondInterval * INDEX_RESOLUTION); // Take first velocity in the profile
    float optimalTravelTime = 0.0f;

    for (int s = 0; s < m_segmentCount; s++) {
        float velocity = m_speedProfiles[s][idx];
        float time = m_lengths[s] / velocity; // Rounded to seconds
        optimalTravelTime += time;
    }
    std::cout << "Optimal travel time " << optimalTravelTime << "s" << std::endl;
    return optimalTravelTime;
}