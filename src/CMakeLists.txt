# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

get_filename_component(IDF_PATH "${CMAKE_SOURCE_DIR}/../libs/ESP8266_RTOS_SDK" ABSOLUTE)
set(ENV{IDF_PATH} "${IDF_PATH}")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(ledPanel)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
