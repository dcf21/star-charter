cmake_minimum_required(VERSION 3.6)
project(galaxyMap)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -Wall -fopenmp -std=c99 -I${CMAKE_SOURCE_DIR}/src -DSRCDIR='\"${CMAKE_SOURCE_DIR}/src\"' -DDCFVERSION='\"x\"' -DDEBUG=0 -D MEMDEBUG1=0 -D MEMDEBUG2=0 -D NORISESET=0")

set(SOURCE_FILES
    src/coreUtils/asciiDouble.c
    src/coreUtils/asciiDouble.h
    src/coreUtils/errorReport.c
    src/coreUtils/errorReport.h
    src/coreUtils/strConstants.h
    src/image.h
    src/image_in.c
    src/image_out.c
    src/main.c
    src/main.h)

add_executable(galaxyMap ${SOURCE_FILES})

target_link_libraries(galaxyMap gsl gslcblas m)
