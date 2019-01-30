#include <iostream>
#include <fstream>
#include <vector>
#include "Data.h"
#include "MCSimulation.h"
#include "ResultStats.h"

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

int main(int argc, char *argv[]) {
    if (argc < 9) {
        // Assuming n e p o a
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

    // Initialize margot
    margot::init();

    // Run simulation
    std::cout << "Runnning simulation..." << std::flush;
    std::vector<float> result;

    // Extract the data features - unpredictability
    auto travelTimesFeatNew = mc.RunMonteCarloSimulation(100, startDay, startHour, startMinute, false);
    ResultStats featStats(travelTimesFeatNew, {});

    // Update the application knobs, if needed
    if (margot::travel::update(samples, featStats.variationCoeff)) {
        margot::travel::manager.configuration_applied();
    }

    // Obtain additional samples if required
    std::vector<float> travelTimesNew;
    if (samples != 100) {
        travelTimesNew = mc.RunMonteCarloSimulation(samples - 100, startDay, startHour, startMinute, false);
        travelTimesNew.insert(travelTimesNew.end(), travelTimesFeatNew.begin(), travelTimesFeatNew.end());
    } else
        result = travelTimesFeatNew;

    std::cout << "Used samples: " << samples << std::endl;

    // Obtain stats
    ResultStats stats(result);
    std::cout << stats << std::endl;
    margot::travel::log();

    // Write results
    std::cout << "Writing result..." << std::flush;
    Routing::Data::WriteResultSingle(result, outputFile);
    std::cout << "OK" << std::endl;

    return 0;
}