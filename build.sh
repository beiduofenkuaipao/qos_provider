#!/bin/bash
set -e

CUR_PATH=${PWD}
echo ${CUR_PATH}
CDDS_INSTALL_PATH=${CUR_PATH}/cdds_install
CDDS_C_PATH=${CUR_PATH}/cyclonedds
CDDS_CXX_PATH=${CUR_PATH}/cyclonedds-cxx



###x64
mkdir -p ${CDDS_INSTALL_PATH}/x64
## c
cd ${CDDS_C_PATH}
mkdir build
cd build


cmake -DCMAKE_INSTALL_PREFIX=${CDDS_INSTALL_PATH}/x64/ ..
cmake --build .
cmake --build . --target install

## c++
cd ${CDDS_CXX_PATH}
mkdir build
cd build

cmake -DCMAKE_INSTALL_PREFIX=${CDDS_INSTALL_PATH}/x64/ ..
cmake --build .
cmake --build . --target install
