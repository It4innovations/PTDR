#pragma once

#include <list>

namespace Routing {
    namespace Data {

        /**
         * Write result of a simulation for all departure times
         * @param result contains vector of travel times obtained from the simulation
         * @param file path to write
         * @param secondInterval number of seconds in single time interval
         */
        void WriteResultAll(std::vector<float> &result, const std::string &file, int samples, float secondInterval);

        /**
       * Write result of a simulation for single departure time
       * @param result contains vector of travel times obtained from the simulation
       * @param file path to write
       */
        void WriteResultSingle(std::vector<float> &result, const std::string &file);
    }
}
