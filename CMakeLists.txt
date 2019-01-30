# check the minimum version
cmake_minimum_required(VERSION 3.0)

# the project name
project(PTDR CXX)

################################
#### General configure section
################################

# setting common c++ flags
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -ftemplate-depth=1100" )

# setting debug flags
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -g3 -O0")

# setting release with debug info flags
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -march=native -mtune=native -g2")

# setting release flags
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native -mtune=native")

# Source files
set(SOURCE_FILES
		src/CSVReader.cpp
		src/Data.cpp
		src/MCSimulation.cpp
		src/ResultStats.cpp)

# Build probability executable
option(EXPLORATION "Perform the DSE" OFF)
if (EXPLORATION)
	set(MAIN src/main_dse.cpp)
	set(MARGOT_CONF_FILE exploration.conf)
endif (EXPLORATION)

option(AUTOTUNING "Compile for autotuning" OFF)
if (AUTOTUNING)
	set(MAIN src/main_autotuning.cpp)
	set(MARGOT_CONF_FILE autotuning.conf)
	set(MARGOT_OPLIST_FILE oplist_90_script.xml)
endif (AUTOTUNING)

if (NOT MAIN)
	message (FATAL_ERROR "No main selected. Enable one of the three options - validation, exploration or autotuning.")
endif (NOT MAIN)


###############################################
##             EXTERNAL LIB                  ##
###############################################

# Intel MKL
option(USE_MKL "Use MKL for RNG" OFF)
if(USE_MKL)
	find_package(MKL REQUIRED)
	include_directories(${MKL_INCLUDE_DIR})
endif()

# OpenMP
# Find OpenMP
if(APPLE)
	if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
		set(OpenMP_CXX "${CMAKE_CXX_COMPILER}")
		set(OpenMP_CXX_FLAGS "-fopenmp=libomp -Wno-unused-command-line-argument")
		set(OpenMP_CXX_LIB_NAMES "libomp" "libgomp" "libiomp5")
		set(OpenMP_libomp_LIBRARY ${OpenMP_CXX_LIB_NAMES})
		set(OpenMP_libgomp_LIBRARY ${OpenMP_CXX_LIB_NAMES})
		set(OpenMP_libiomp5_LIBRARY ${OpenMP_CXX_LIB_NAMES})
	endif()
endif()

find_package(OpenMP REQUIRED COMPONENTS CXX)
if(OPENMP_FOUND)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()

# mARGOt and mARGOt HEEL
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/margot_project/core/install/lib/cmake")
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/margot_heel_if/cmake")
#
## margot heel interface
find_package(MARGOT REQUIRED)
find_package(MARGOT_HEEL REQUIRED)

# add the required include directories
include_directories(${MARGOT_INCLUDES} ${MARGOT_HEEL_INCLUDES})
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src")


###############################################
##             BUILD SECTION                 ##
###############################################

# Set the application name
set(APP_NAME "ptdr")

# Main target
add_executable(${APP_NAME} ${SOURCE_FILES} ${MAIN})
target_link_libraries(${APP_NAME} ${MKL_MINIMAL_LIBRARY} ${MARGOT_HEEL_LIBRARIES} ${OpenMP_CXX_LIBRARY} dl pthread m)

# Installation
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set (
      CMAKE_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}"
      CACHE PATH "default install path"
      FORCE )
endif()

install(TARGETS ${APP_NAME} DESTINATION bin)
