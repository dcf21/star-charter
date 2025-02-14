# Makefile for StarCharter
# 
# -------------------------------------------------
# Copyright 2015-2025 Dominic Ford
#
# This file is part of StarCharter.
#
# StarCharter is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# StarCharter is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with StarCharter.  If not, see <http://www.gnu.org/licenses/>.
# -------------------------------------------------

CWD=$(shell pwd)

VERSION = 8.0
DATE    = 23/02/2025
PATHLINK= /

WARNINGS= -Wall -Wno-format-truncation -Wno-unused-result -Wno-unknown-pragmas
COMPILE = $(CC) $(WARNINGS) -g -c -I $(CWD)/src
LIBS    = -lcairo -lgsl -lgslcblas -lz -lm
LINK    = $(CC) $(WARNINGS) -g -fopenmp

OPTIMISATION = -O3

DEBUG          = -D DEBUG=1 -D MEMDEBUG1=1 -D MEMDEBUG2=0 -fopenmp
NODEBUG        = -D DEBUG=0 -D MEMDEBUG1=0 -D MEMDEBUG2=0 -fopenmp
SINGLE_THREAD  = -D DEBUG=0 -D MEMDEBUG1=0 -D MEMDEBUG2=0

LOCAL_SRCDIR = src
LOCAL_OBJDIR = obj
LOCAL_BINDIR = bin

CORE_FILES = astroGraphics/constellations.c astroGraphics/deepSky.c astroGraphics/deepSkyOutlines.c \
             astroGraphics/ephemeris.c astroGraphics/galaxyMap.c astroGraphics/greatCircles.c \
             astroGraphics/gridLines.c astroGraphics/horizon.c astroGraphics/meteorShower.c \
             astroGraphics/solarSystem.c astroGraphics/starListReader.c astroGraphics/scaleBars.c \
             astroGraphics/stars.c astroGraphics/textAnnotations.c astroGraphics/zenith.c coreUtils/asciiDouble.c \
             coreUtils/errorReport.c coreUtils/makeRasters.c ephemCalc/constellations.c ephemCalc/magnitudeEstimate.c \
             ephemCalc/jpl.c ephemCalc/orbitalElements.c listTools/ltDict.c listTools/ltList.c \
             listTools/ltMemory.c listTools/ltStringProc.c mathsTools/julianDate.c mathsTools/projection.c \
             mathsTools/sphericalTrig.c settings/chart_config.c settings/read_config.c settings/render_chart.c \
             vectorGraphics/arrowDraw.c vectorGraphics/lineDraw.c vectorGraphics/cairo_page.c

CORE_HEADERS = astroGraphics/constellations.h astroGraphics/deepSky.h astroGraphics/deepSkyOutlines.h \
               astroGraphics/ephemeris.h astroGraphics/galaxyMap.h astroGraphics/greatCircles.h \
               astroGraphics/gridLines.h astroGraphics/horizon.h astroGraphics/meteorShower.h \
               astroGraphics/solarSystem.h astroGraphics/starListReader.h astroGraphics/scaleBars.h \
               astroGraphics/stars.h astroGraphics/textAnnotations.h astroGraphics/zenith.h coreUtils/asciiDouble.h \
               coreUtils/errorReport.h coreUtils/makeRasters.h coreUtils/strConstants.h ephemCalc/alias.h \
               ephemCalc/constellations.h ephemCalc/magnitudeEstimate.h ephemCalc/jpl.h ephemCalc/orbitalElements.h \
               listTools/ltDict.h listTools/ltList.h listTools/ltMemory.h listTools/ltStringProc.h \
               mathsTools/julianDate.h mathsTools/projection.h mathsTools/sphericalTrig.h settings/chart_config.h \
               settings/read_config.h settings/render_chart.h vectorGraphics/arrowDraw.h vectorGraphics/lineDraw.h \
               vectorGraphics/cairo_page.h

STARCHART_FILES = main.c

STARCHART_HEADERS =

CORE_SOURCES                    = $(CORE_FILES:%.c=$(LOCAL_SRCDIR)/%.c)
CORE_OBJECTS                    = $(CORE_FILES:%.c=$(LOCAL_OBJDIR)/%.o)
CORE_OBJECTS_DEBUG              = $(CORE_OBJECTS:%.o=%.debug.o)
CORE_OBJECTS_SINGLE_THREAD      = $(CORE_OBJECTS:%.o=%.single_thread.o)
CORE_HFILES                     = $(CORE_HEADERS:%.h=$(LOCAL_SRCDIR)/%.h) Makefile

STARCHART_SOURCES               = $(STARCHART_FILES:%.c=$(LOCAL_SRCDIR)/%.c)
STARCHART_OBJECTS               = $(STARCHART_FILES:%.c=$(LOCAL_OBJDIR)/%.o)
STARCHART_OBJECTS_DEBUG         = $(STARCHART_OBJECTS:%.o=%.debug.o)
STARCHART_OBJECTS_SINGLE_THREAD = $(STARCHART_OBJECTS:%.o=%.single_thread.o)
STARCHART_HFILES                = $(STARCHART_HEADERS:%.h=$(LOCAL_SRCDIR)/%.h) Makefile

ALL_HFILES = $(CORE_HFILES) $(STARCHART_HFILES)

SWITCHES = -D DCFVERSION=\"$(VERSION)\"  -D DATE=\"$(DATE)\"  -D PATHLINK=\"$(PATHLINK)\"  -D SRCDIR=\"$(CWD)/$(LOCAL_SRCDIR)/\"

all: $(LOCAL_BINDIR)/starchart.bin $(LOCAL_BINDIR)/debug/starchart.bin $(LOCAL_BINDIR)/single_thread/starchart.bin

#
# General macros for the compile steps
#

$(LOCAL_OBJDIR)/%.o:               $(LOCAL_SRCDIR)/%.c $(ALL_HFILES)
	mkdir -p $(LOCAL_OBJDIR) $(LOCAL_OBJDIR)/astroGraphics $(LOCAL_OBJDIR)/coreUtils $(LOCAL_OBJDIR)/ephemCalc $(LOCAL_OBJDIR)/listTools $(LOCAL_OBJDIR)/mathsTools $(LOCAL_OBJDIR)/settings $(LOCAL_OBJDIR)/vectorGraphics
	$(COMPILE) $(OPTIMISATION) $(NODEBUG) $(SWITCHES) $< -o $@

$(LOCAL_OBJDIR)/%.debug.o:         $(LOCAL_SRCDIR)/%.c $(ALL_HFILES)
	mkdir -p $(LOCAL_OBJDIR) $(LOCAL_OBJDIR)/astroGraphics $(LOCAL_OBJDIR)/coreUtils $(LOCAL_OBJDIR)/ephemCalc $(LOCAL_OBJDIR)/listTools $(LOCAL_OBJDIR)/mathsTools $(LOCAL_OBJDIR)/settings $(LOCAL_OBJDIR)/vectorGraphics
	$(COMPILE) $(OPTIMISATION) $(DEBUG)   $(SWITCHES) $< -o $@

$(LOCAL_OBJDIR)/%.single_thread.o: $(LOCAL_SRCDIR)/%.c $(ALL_HFILES)
	mkdir -p $(LOCAL_OBJDIR) $(LOCAL_OBJDIR)/astroGraphics $(LOCAL_OBJDIR)/coreUtils $(LOCAL_OBJDIR)/ephemCalc $(LOCAL_OBJDIR)/listTools $(LOCAL_OBJDIR)/mathsTools $(LOCAL_OBJDIR)/settings $(LOCAL_OBJDIR)/vectorGraphics
	$(COMPILE) $(OPTIMISATION) $(SINGLE_THREAD)   $(SWITCHES) $< -o $@

#
# Make the binaries
#

$(LOCAL_BINDIR)/starchart.bin: $(CORE_OBJECTS) $(STARCHART_OBJECTS)
	mkdir -p $(LOCAL_BINDIR)
	$(LINK) $(OPTIMISATION) $(CORE_OBJECTS) $(STARCHART_OBJECTS) $(LIBS) -o $(LOCAL_BINDIR)/starchart.bin

$(LOCAL_BINDIR)/debug/starchart.bin: $(CORE_OBJECTS_DEBUG) $(STARCHART_OBJECTS_DEBUG)
	mkdir -p $(LOCAL_BINDIR)/debug
	echo "The files in this directory are binaries with debugging options enabled: they produce activity logs called 'starchart.log'. It should be noted that these binaries can up to ten times slower than non-debugging versions." > $(LOCAL_BINDIR)/debug/README
	$(LINK) $(OPTIMISATION) $(CORE_OBJECTS_DEBUG) $(STARCHART_OBJECTS_DEBUG) $(LIBS) -o $(LOCAL_BINDIR)/debug/starchart.bin

$(LOCAL_BINDIR)/single_thread/starchart.bin: $(CORE_OBJECTS_SINGLE_THREAD) $(STARCHART_OBJECTS_SINGLE_THREAD)
	mkdir -p $(LOCAL_BINDIR)/single_thread
	$(LINK) $(OPTIMISATION) $(CORE_OBJECTS_SINGLE_THREAD) $(STARCHART_OBJECTS_SINGLE_THREAD) $(LIBS) -o $(LOCAL_BINDIR)/single_thread/starchart.bin

#
# Clean macros
#

clean:
	rm -vfR $(LOCAL_OBJDIR) $(LOCAL_BINDIR)

afresh: clean all
