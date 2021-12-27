cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(FunRootAna)

# You need to tell CMake where to find the ROOT installation. This can be done in a number of ways:
#   - ROOT built with classic configure/make use the provided $ROOTSYS/etc/cmake/FindROOT.cmake
#   - ROOT built with CMake. Add in CMAKE_PREFIX_PATH the installation prefix for ROOT
list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})

find_package(ROOT REQUIRED COMPONENTS Core Hist Tree)

include(${ROOT_USE_FILE})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${ROOT_INCLUDE_DIRS})
message(STATUS "ROOT_INCLUDE_DIRS: ${ROOT_INCLUDE_DIRS}")

add_library(FunRootAna SHARED fwksrc/Access.cpp	fwksrc/Conf.cpp	fwksrc/HIST.cpp	fwksrc/Weights.cpp	fwksrc/assure.cpp )
target_link_libraries(FunRootAna ${ROOT_LIBRARIES})
target_compile_features(FunRootAna PRIVATE cxx_std_17)


# Play with it when need another executables
add_executable(example_analysis examples/example_analysis.cpp)
target_link_libraries(example_analysis ${ROOT_LIBRARIES} FunRootAna)
target_compile_features(example_analysis PRIVATE cxx_std_17)
