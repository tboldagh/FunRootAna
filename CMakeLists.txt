cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

# EXAMPLE intgeration with ATLAS experiment analsysis s/w as a library
# to do checkout the FunRootAna into the source area (athena) and add the dependency FunRootAna in atlas_add_library
# atlas_subdir( FunRootAna )
# find_package( ROOT COMPONENTS MathCore RIO Core Tree Hist Physics )

# atlas_add_library( FunRootAna src/*.cpp
#                    PUBLIC_HEADERS FunRootAna
#                    SHARED
#                    LINK_LIBRARIES ${ROOT_LIBRARIES}
#                    PRIVATE_INCLUDE_DIRS FunRootAna ${ROOT_INCLUDE_DIRS}
# )



SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall --pedantic -O2 -g")

project(FunRootAna)

# You need to tell CMake where to find the ROOT installation. This can be done in a number of ways:
#   - ROOT built with classic configure/make use the provided $ROOTSYS/etc/cmake/FindROOT.cmake
#   - ROOT built with CMake. Add in CMAKE_PREFIX_PATH the installation prefix for ROOT
list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})

find_package(ROOT REQUIRED COMPONENTS Core Hist Tree)

include(${ROOT_USE_FILE})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/FunRootAna)
include_directories(${ROOT_INCLUDE_DIRS})
message(STATUS "ROOT_INCLUDE_DIRS: ${ROOT_INCLUDE_DIRS}")

file(GLOB LIBSRC "src/*.cpp")
add_library(FunRootAna SHARED ${LIBSRC} )
target_link_libraries(FunRootAna ${ROOT_LIBRARIES})
target_compile_features(FunRootAna PRIVATE cxx_std_17)

SET(CMAKE_INSTALL_PREFIX ../)
install(TARGETS FunRootAna LIBRARY DESTINATION lib)

# Play with it when need another executables
add_executable(example_analysis examples/example_analysis.cpp)
target_link_libraries(example_analysis ${ROOT_LIBRARIES} FunRootAna)
target_compile_features(example_analysis PRIVATE cxx_std_17)

add_executable( test_efv tests/test_efv.cpp )
target_link_libraries(test_efv ${ROOT_LIBRARIES} FunRootAna)
target_compile_features(test_efv PRIVATE cxx_std_17)

add_executable( test_hist tests/test_hist.cpp )
target_link_libraries(test_hist ${ROOT_LIBRARIES} FunRootAna)
target_compile_features(test_hist PRIVATE cxx_std_17)

add_executable( test_lfv tests/test_fv.cpp )
target_link_libraries(test_lfv ${ROOT_LIBRARIES} FunRootAna)
target_compile_definitions(test_lfv PRIVATE TEST_LAZY=1)
target_compile_features(test_lfv PRIVATE cxx_std_17)

add_executable( test_efv2 tests/test_fv.cpp )
target_link_libraries(test_efv2 ${ROOT_LIBRARIES} FunRootAna)
target_compile_features(test_efv2 PRIVATE cxx_std_17)


add_executable( test_lfv_objects tests/test_fv_objects.cpp )
target_link_libraries(test_lfv_objects ${ROOT_LIBRARIES} FunRootAna)
target_compile_definitions(test_lfv_objects PRIVATE TEST_LAZY=1)
target_compile_features(test_lfv_objects PRIVATE cxx_std_17)

add_executable( test_efv2_objects tests/test_fv_objects.cpp )
target_link_libraries(test_efv2_objects ${ROOT_LIBRARIES} FunRootAna)
target_compile_features(test_efv2_objects PRIVATE cxx_std_17)

add_executable( test_weight tests/test_weight.cpp )
target_link_libraries(test_weight ${ROOT_LIBRARIES} FunRootAna)
target_compile_features(test_weight PRIVATE cxx_std_17)

add_executable( test_conf tests/test_conf.cpp )
target_link_libraries(test_conf ${ROOT_LIBRARIES} FunRootAna)
target_compile_features(test_conf PRIVATE cxx_std_17)


add_executable( benchmark tests/benchmark.cpp )
target_link_libraries(benchmark ${ROOT_LIBRARIES} FunRootAna)
target_compile_features(benchmark PRIVATE cxx_std_17)


enable_testing() 
add_test(NAME test_HIST COMMAND test_hist)
add_test(NAME test_EagerFunctionalVector COMMAND test_efv)
add_test(NAME test_LazyFunctionalVector COMMAND test_lfv)
add_test(NAME test_EagerFunctionalVector2 COMMAND test_efv2)
add_test(NAME test_LazyFunctionalVector_objects COMMAND test_lfv_objects)
add_test(NAME test_EagerFunctionalVector2_objects COMMAND test_efv2_objects)
add_test(NAME test_conf COMMAND test_conf)


add_test(NAME test_Weight COMMAND test_weight)

add_test(NAME benchmark COMMAND benchmark)


#target_compile_features(example_analysis PRIVATE cxx_std_17)
