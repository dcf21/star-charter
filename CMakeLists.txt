cmake_minimum_required(VERSION 3.6)
project(starcharter)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -Wall -fopenmp -std=c99 -I${CMAKE_SOURCE_DIR}/src -DSRCDIR='\"${CMAKE_SOURCE_DIR}/src\"' -DDCFVERSION='\"x\"' -DDEBUG=0 -D MEMDEBUG1=0 -D MEMDEBUG2=0")

set(SOURCE_FILES
        src/astroGraphics/constellations.c
        src/astroGraphics/constellations.h
        src/astroGraphics/deepSky.c
        src/astroGraphics/deepSky.h
        src/astroGraphics/deepSkyOutlines.c
        src/astroGraphics/deepSkyOutlines.h
        src/astroGraphics/ephemeris.c
        src/astroGraphics/ephemeris.h
        src/astroGraphics/galaxyMap.c
        src/astroGraphics/galaxyMap.h
        src/astroGraphics/greatCircles.c
        src/astroGraphics/greatCircles.h
        src/astroGraphics/raDecLines.c
        src/astroGraphics/raDecLines.h
        src/astroGraphics/starListReader.c
        src/astroGraphics/starListReader.h
        src/astroGraphics/stars.c
        src/astroGraphics/stars.h
        src/coreUtils/asciiDouble.c
        src/coreUtils/asciiDouble.h
        src/coreUtils/errorReport.c
        src/coreUtils/errorReport.h
        src/coreUtils/makeRasters.c
        src/coreUtils/makeRasters.h
        src/coreUtils/strConstants.h
        src/listTools/ltDict.c
        src/listTools/ltDict.h
        src/listTools/ltList.c
        src/listTools/ltList.h
        src/listTools/ltMemory.c
        src/listTools/ltMemory.h
        src/listTools/ltStringProc.c
        src/listTools/ltStringProc.h
        src/mathsTools/julianDate.c
        src/mathsTools/julianDate.h
        src/mathsTools/projection.c
        src/mathsTools/projection.h
        src/mathsTools/sphericalTrig.c
        src/mathsTools/sphericalTrig.h
        src/settings/chart_config.c
        src/settings/chart_config.h
        src/vectorGraphics/lineDraw.c
        src/vectorGraphics/lineDraw.h
        src/vectorGraphics/cairo_page.c
        src/vectorGraphics/cairo_page.h
        src/main.c)

add_executable(starcharter ${SOURCE_FILES})

target_link_libraries(starcharter gsl gslcblas z cairo m)

