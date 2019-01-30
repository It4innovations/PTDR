#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
//two more includes required from the profile function
#include <cmath>
#include <algorithm>
#include "Data.h"
#include "MCSimulation.h"
#include "ResultStats.h"

#if defined(_OPENMP)

#include <omp.h>

#endif
//margot header
#include <margot.hpp>

void printHelp() {
    std::cout
            << "Usage: ptdr -n [number of samples] -e [edges_file.csv] -p [profiles directory] -o [output_file.csv] (-l, -a) -d [start day] -h [start hour] -m [start minute]"
            << std::endl;
    std::cout << "\t Arguments:" << std::endl;
    std::cout << "\t\t -n: number of Monte Carlo samples to execute" << std::endl;
    std::cout << "\t\t -e: Edges file (CSV)" << std::endl;
    std::cout << "\t\t -p: Directory with speed profiles" << std::endl;
    std::cout << "\t\t -o: Output file (CSV)" << std::endl;
    std::cout << "\t\t -d: Start day (0-6)" << std::endl;
    std::cout << "\t\t -h: Start hour (0-23)" << std::endl;
    std::cout << "\t\t -m: Start minute (0-59)" << std::endl;
    std::cout << "\t Flags:" << std::endl;
    std::cout << "\t\t -l: Compute optimal travel time" << std::endl;
    std::cout << "\t\t -a: Compute for all week intervals (ignores start times)" << std::endl;
}
//we will be working with different data collections: the previous result vector is not enough for the 
//profiling version of the application. we define the result type as below
using result_t = std::map<float, std::vector<double>>;//percentile map, for every float (percentile of interst) has a vector of doubles (the actual values)
// define percentile of interest.
const std::vector<float> percentiles_of_interest = {0.05, 0.1, 0.25, 0.5, 0.75, 0.9, 0.95};
//result variable need to be global since is used in the added code.
result_t results;

//function prototype. the definition is done after the main.
void margot_profile_montecarlo(const Routing::MCSimulation &runner, const int samples, const int start_day,
                               const int start_hour, const int start_minute, bool all);

int main(int argc, char *argv[]) {
    if (argc < 9) // Assuming n e p o a
    {
        std::cerr << "Invalid argument count." << std::endl;
        printHelp();
        std::exit(1);
    }

    char **largv = argv;
    std::string edgesPath, profilePath, outputFile;
    int samples = 0, startDay = -1, startHour = -1, startMinute = -1;
    bool optimal = false, all = false;
    while (*++largv) {
        switch ((*largv)[1]) {
            case 'n':
                samples = std::stoi(*++largv);
                break;
            case 'e':
                edgesPath = *++largv;
                break;
            case 'p':
                profilePath = *++largv;
                break;
            case 'o':
                outputFile = *++largv;
                break;
            case 'd':
                startDay = std::stoi(*++largv);
                break;
            case 'h':
                startHour = std::stoi(*++largv);
                break;
            case 'm':
                startMinute = std::stoi(*++largv);
                break;
            case 'l':
                optimal = true;
                break;
            case 'a':
                //all = true;  see lower, running all doesn't make sense for the exploration/validation/autotuning version of the program. we want to simulate the actual program implementation (i.e. one punctual request from the user for path with defined temporal coordinates).
                break;
            default:
                printHelp();
                std::exit(1);
        }
    }

    if (!all && (startDay == -1 || startHour == -1 || startMinute == -1)) {
        std::cerr << "Invalid start time." << std::endl;
        printHelp();
        std::exit(1);
    }

    std::cout << "Samples: " << samples << std::endl;
    std::cout << "Edges file: " << edgesPath << std::endl;
    std::cout << "Profiles directory: " << profilePath << std::endl;
    std::cout << "Output file: " << outputFile << std::endl;
    std::cout << "Compute optimal travel time: " << (optimal ? std::string("Yes") : std::string("No")) << std::endl;
    std::cout << "Run all simulations: " << (all ? std::string("Yes") : std::string("No")) << std::endl;
    if (!all)
        std::cout << "Start day: " << startDay << " at " << startHour << ":" << startMinute << std::endl;


    // Load data
    std::cout << "Loading data...";
    std::cout.flush();
    auto startTime = std::chrono::high_resolution_clock::now();
    Routing::MCSimulation mc(edgesPath, profilePath);
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - startTime).count();
    std::cout << "OK" << std::endl;
    std::cout << "Elapsed time: " << elapsed << " ms" << std::endl;

    //added by hand, dunno if want to go back to parameter or no
    int threads = 8;
#if defined(_OPENMP)
    omp_set_num_threads(threads);
#endif
    //init of margot must be inserted in the main, before the first call to any of its functions
    margot::init();


    // Run simulation.
    // the program has been stripped by the optimal and the write all doesn't make sense for this kind of exploration.
    std::cout << "Runnning simulation..." << std::flush;
    // this called function is the only substantial difference in the main.
    margot_profile_montecarlo(mc, samples, startDay, startHour, startMinute, all);


    //commented out result writing, since it is working on different types and margot is already logging the required information
    //ResultStats stats(result);
    //std::cout << stats << std::endl;
    //std::cout << "Writing result..." << std::flush;

    //Routing::Data::WriteResultSingle(result, outputFile, mc.GetSecondInterval());
    //std::cout << "OK" << std::endl;
    return 0;
}


//the code from now and below are the functions for the profiling version of the time dependent routing. must be inserted with LARA.
template<int repetitions>
inline void
montecarlo_n_runner(const Routing::MCSimulation &runner, const int samples, const int start_day, const int start_hour,
                    const int start_minute, bool all) {
    auto run_result = runner.RunMonteCarloSimulation(samples, start_day, start_hour, start_minute, all);
    ResultStats stats(run_result, percentiles_of_interest);
    for (float perc : percentiles_of_interest) {
        //	std::cout << "percentile: "<<perc<<" has value: "<<stats.percentiles[perc]<<std::endl;
        results[perc].emplace_back(stats.percentiles[perc]);
    }

    montecarlo_n_runner<repetitions - 1>(runner, samples, start_day, start_hour, start_minute, all);
}

template<>
inline void montecarlo_n_runner<0>(const Routing::MCSimulation &runner, const int samples, const int start_day,
                                   const int start_hour, const int start_minute, bool all) {}


void margot_profile_montecarlo(const Routing::MCSimulation &runner, const int samples, const int start_day,
                               const int start_hour, const int start_minute, bool all) {
    static const int NUM_ITERATION_FOR_ERROR = 1000;
    // get the number of segments
    const int number_of_segments = runner.GetSegmentNumber();

    // start the measures
    margot::travel::start_monitor();

    // actually run the montecarlo NUM_ITERATION_FOR_ERROR times
    // this could be a simple for loop
    montecarlo_n_runner<NUM_ITERATION_FOR_ERROR>(runner, samples, start_day, start_hour, start_minute, false);

    // stop the observations for the performance measures
    margot::travel::monitor::my_elapsed_time_monitor_us.stop();
    margot::travel::monitor::my_energy_monitor.stop();
    margot::travel::monitor::my_throughput_monitor.stop(number_of_segments * NUM_ITERATION_FOR_ERROR);

    // consider the fact the we are actually measuring NUM_ITERATION_FOR_ERROR times the execution
    margot::travel::monitor::my_energy_monitor.push(
            margot::travel::monitor::my_energy_monitor.last() / static_cast<float>(NUM_ITERATION_FOR_ERROR));
    margot::travel::monitor::my_elapsed_time_monitor_us.push(
            margot::travel::monitor::my_elapsed_time_monitor_us.last() / static_cast<float>(NUM_ITERATION_FOR_ERROR));

    // compute the power value (uJ / uS)
    margot::travel::monitor::my_power_monitor.push(
            static_cast<float>(margot::travel::monitor::my_energy_monitor.last()) /
            static_cast<float>(margot::travel::monitor::my_elapsed_time_monitor_us.last()) * 1000000);

    //postprocessing of errors to select max to push in monitor. i took the algoritm from the "resultstats" class, could not use a member of class because they want float as constructor array.
    std::vector<double> errors;
    for (result_t::iterator it = results.begin(); it != results.end(); ++it) {
        double M = (*it).second[0];
        double Q = 0.0;
        for (size_t i = 0; i < (*it).second.size(); ++i) {
            Q += (i * std::pow((*it).second[i] - M, 2)) / (i + 1);
            M += ((*it).second[i] - M) / (i + 1);
        }
        if ((*it).second.size() > 1) {
            errors.emplace_back(std::sqrt(Q / ((*it).second.size() - 1)) / M);
        } else {
            errors.emplace_back(0.0);
        }


    }

    margot::travel::monitor::my_error_monitor.push(*(std::max_element(errors.begin(), errors.end())));



    //data feature extraction
    std::vector<float> features100;
    for (int i = 0; i < NUM_ITERATION_FOR_ERROR; i++) {
        auto feat_100 = runner.RunMonteCarloSimulation(100, start_day, start_hour, start_minute, false);
        ResultStats feat_100_stat(feat_100, {});
        features100.push_back(feat_100_stat.variationCoeff);
    }
    std::sort(features100.begin(), features100.end());
    margot::travel::monitor::my_unpredictability_monitor.push(features100[NUM_ITERATION_FOR_ERROR / 100 * 50]);
    features100.clear();
    // log the result to file
    margot::travel::log();

}







