#!/bin/bash

# Bootstrap script for mARGOt 2
CWD=${PWD}

# Select mode
if [ "$#" -ne "1" ]
then
    echo "Usage $0: [mode] AUTOTUNING, EXPLORATION, clean"
    exit 1
fi
MODE=$1

# Get absolute path to project dir
if git rev-parse --git-dir > /dev/null 2>&1
then
	PROJECT_ROOT=$(git rev-parse --show-toplevel)
else
	echo "Error: run this script in root of the StaticRoutingCPP repo"
	exit -1
fi

echo "Bootstrapping mARGOt 2 in $PROJECT_ROOT"
MARGOT_CONFIG_ROOT=${PROJECT_ROOT}/margot_config
MARGOT_CORE_ROOT=${PROJECT_ROOT}/margot_project/core
MARGOT_CONFIG_FILE=""
MARGOT_OPLIST=""

echo "Mode: $MODE"

if [ $MODE == "AUTOTUNING" ]
then
    MARGOT_CONFIG_FILE=${MARGOT_CONFIG_ROOT}/autotuning.conf
    MARGOT_OPLIST=${MARGOT_CONFIG_ROOT}/oplist_90_script.xml
fi

if [ $MODE == "EXPLORATION" ]
then
    MARGOT_CONFIG_FILE=${MARGOT_CONFIG_ROOT}/exploration.conf
fi

if [ "$MODE" == "clean" ]
then
    echo "Removing build artifacts"
    rm -rf ${PROJECT_ROOT}/margot_project/core/build ${PROJECT_ROOT}/margot_heel_if ${PROJECT_ROOT}/build
    exit 0
fi

if [ -z "${MARGOT_CONFIG_FILE}" ]
then
    echo "Invalid mode."
    exit 1
fi

if [[ -d "${PROJECT_ROOT}/margot_project/core/build" || -d "${PROJECT_ROOT}/margot_heel_if/build" ]]
then
    echo "Directory margot_project/core/build or margot_heel_if/build already exists, bailing out."
    echo "run \"$0 clean\" to remove margot"
    exit -1
fi

# Clone margot repo
if [ ! -d "${PROJECT_ROOT}/margot_project/core" ]
then
    git clone https://gitlab.com/margot_project/core.git ${MARGOT_CORE_ROOT} || exit -1
else
    echo "Found mArgot: ${PROJECT_ROOT}/margot_project/core"
fi

# Build margot
echo "Building mARGOt"
mkdir ${MARGOT_CORE_ROOT}/build || exit -1
pushd ${MARGOT_CORE_ROOT}/build
cmake -DCMAKE_INSTALL_PREFIX:PATH=${MARGOT_CORE_ROOT}/install -DCMAKE_BUILD_TYPE=Release .. || exit -1
make || exit -1
make install
popd

# Setup HEEL
echo "Resetting high-level interface"
rm -rfv ${PROJECT_ROOT}/margot_heel_if
cp -r ${MARGOT_CORE_ROOT}/margot_heel/margot_heel_if ${PROJECT_ROOT}
rm ${PROJECT_ROOT}/margot_heel_if/config/*.conf

echo "Configuring mARGOt from ${MARGOT_CONFIG_ROOT}"
cp ${MARGOT_CONFIG_FILE} ${PROJECT_ROOT}/margot_heel_if/config/

if [ -n "${MARGOT_OPLIST}" ]
then
    cp -v ${MARGOT_OPLIST} ${PROJECT_ROOT}/margot_heel_if/config/oplist.routing-cli.travel.conf
fi

echo "Building high-level interface with config file: ${MARGOT_CONFIG_FILE}"
mkdir ${PROJECT_ROOT}/margot_heel_if/build
pushd ${PROJECT_ROOT}/margot_heel_if/build
cmake -DCMAKE_BUILD_TYPE=Release -DMARGOT_CONF_FILE=$(basename ${MARGOT_CONFIG_FILE}) .. || exit -1
make
popd

echo "Bootstrap FINISHED"

# Build the app
echo "Building PTDR"
mkdir ${PROJECT_ROOT}/build
pushd ${PROJECT_ROOT}/build
cmake -DCMAKE_BUILD_TYPE=Release -D${MODE}=ON ..
make
popd

echo "PTDR Build FINISHED"