# Probability
This directory contains source code of the Probabilistic Time-Dependent Routing (PTDR) with optional mArgot autotuner integration.

Check out our IEEE paper on Autotuning and PTDR: https://ieeexplore.ieee.org/stamp/stamp.jsp?tp=&arnumber=8758945.

## Description and usage
The binary provides two main functions. Monte Carlo simulation of travel time and computation of the so-called optimal travel time along the given path. The Monte Carlo mode is default, computation of optimal path is specified by the 'l' flag.

Input of the simulation consists of two parts. The first one is the path which consists of individual road segments of given length. Each segment has its own ID, length and the so-called free-flow speed specified. The second part of the input are the speed profiles. They contain actual speeds with associated probability for given period of time. In current implementation, the intervals are loaded for a week, divided to days. The weekdays are further divided in time intervals of a specified length.

Output of the simulation is stored in a CSV file specified by the `-o` argument. First two columns of the file contains numbers which denote day of week and an interval of the day. The interval depends on time interval determined from the speed profiles. For example, 15 minute interval divides day to 96 intervals labelled 0-95.

The simulation can be performed either for all intervals available in the speed profiles (flag `-a`) or only for a specified interval determined by day `-d`, hour `-h` and minute `-m`.

## Speed profile data sets
* UK - [4 paths in UK road network of varying length](ExampleData/SpeedProfiles/probability_uk), generated from real data
* CZ - [300 paths in Czech road network](ExampleData/SpeedProfiles/benchmark), benchmark data set, artficially generated by Markov chain model


## Build instructions
Bootstrap script ```bootstrap_margot.sh``` handles integration with the mArgot autotuner and building of the application itself. 
It clones the mArgot autotuner and prepares the  selected application mode. There are two modes _AUTOTUNING_ mode is intended for 
run-time autotuning of the PTDR runs. It uses operation point list stored in the ```margot_config/oplist_90_script.xml``` file. 
The second mode is _EXPLORATION_ which is used for oplist generation using the DSE.

### Required modules
* C++ compiler (Intel, GCC, Clang)
* CMake
* Intel MKL library - optional

Run bootstrap script
```
sh bootstrap_margot.sh AUTOTUNING
```

## Command line arguments
ptdr -n [number of samples] -e [edges_file.csv] -p [profiles directory] -o [output_file.csv] (-l, -a) -d [start day] -h [start hour] -m [start minute]

* Arguments:
	* -n: number of Monte Carlo samples to execute
	* -e: Edges file (CSV)
	* -p: Directory with speed profiles
	* -o: Output file (CSV)
	* -d: Start day (0-6)
	* -h: Start hour (0-23)
	* -m: Start minute (0-59)
* Flags:
	* -l: Compute optimal travel time
	* -a: Compute for all week intervals (ignores start times)

## Acknowledgement
This work was supported by The Ministry of Education, Youth and Sports from the National Programme of Sustainability (NPU II) project ‘IT4Innovations excellence in science - LQ1602’, by the IT4Innovations infrastructure which is supported from the Large Infrastructures for Research, Experimental Development and Innovations project ‘IT4Innovations National Supercomputing Center – LM2015070’, and partially by ANTAREX, a project supported by the EU H2020 FET-HPC program under grant agreement  No. 671623.

## References
- Vitali, E., Gadioli, D., Palermo, G., Golasowski, M., Bispo, J., Pinto, P., ... & Silvano, C. (2019). An efficient monte carlo-based probabilistic time-dependent routing calculation targeting a server-side car navigation system. IEEE transactions on emerging topics in computing.
- Silvano, C., Agosta, G., Bartolini, A., Beccari, A. R., Benini, L., Besnard, L., ... & Cherubin, S. (2018, August). ANTAREX: A DSL-based Approach to Adaptively Optimizing and Enforcing Extra-Functional Properties in High Performance Computing. In 2018 21st Euromicro Conference on Digital System Design (DSD) (pp. 600-607). IEEE.
- Golasowski, M., Tomis, R., Martinovič, J., Slaninová, K., & Rapant, L. (2016, September). Performance evaluation of probabilistic time-dependent travel time computation. In IFIP International Conference on Computer Information Systems and Industrial Management (pp. 377-388). Springer, Cham.
