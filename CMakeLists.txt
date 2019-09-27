cmake_minimum_required(VERSION 3.6)

project(StaticMemoryAnalyzerForCpp)

# Common options
if(CMAKE_BUILD_TYPE STREQUAL "")
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
    "Set the build type, usually Release or RelWithDebInfo" FORCE)
endif(CMAKE_BUILD_TYPE STREQUAL "")


# Boost
set(NEEDED_BOOST_COMPONENTS program_options )

find_package(Boost COMPONENTS ${NEEDED_BOOST_COMPONENTS})

if(NOT Boost_FOUND)
  # Automatically print stuff if it failed
  set(Boost_DEBUG ON)
  find_package(Boost COMPONENTS ${NEEDED_BOOST_COMPONENTS})
  message(FATAL_ERROR "Failed to find Boost libraries")
endif(NOT Boost_FOUND)

# Boost is found or the configuration has already failed

include_directories(${Boost_INCLUDE_DIR})

# All boost libs are linked by cmake
add_definitions(-DBOOST_ALL_NO_LIB)
add_definitions(-DBOOST_PROGRAM_OPTIONS_DYN_LINK=1)

# Subdirs have the libraries and executables defined
add_subdirectory(src)
add_subdirectory(test)
