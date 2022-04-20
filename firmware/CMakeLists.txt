# This is the project CMakeLists.txt file for the test subproject
cmake_minimum_required(VERSION 3.5)
set(IDF_TARGET esp32s3)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(firewaterburn)
message(${CXX_COMPILE_OPTIONS})