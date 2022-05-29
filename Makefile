# Makefile for StarCharter
# 
# -------------------------------------------------
# Copyright 2015-2022 Dominic Ford
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

VERSION = 4.0
DATE    = 12/12/2021
PATHLINK= /

WARNINGS= -Wall -Wno-format-truncation -Wno-unused-result
COMPILE = $(CC) $(WARNINGS) -g -fopenmp -c -I $(CWD)/src
LIBS    = -lcairo -lgsl -lgslcblas -lz -lm
LINK    = $(CC) $(WARNINGS) -g -fopenmp

OPTIMISATION = -O3

DEBUG   = -D DEBUG=1 -D MEMDEBUG1=1 -D MEMDEBUG2=0
NODEBUG = -D DEBUG=0 -D MEMDEBUG1=0 -D MEMDEBUG2=0

LOCAL_SRCDIR = src
LOCAL_OBJDIR = obj
LOCAL_BINDIR = bin

CORE_FILES = astroGraphics/constellations.c astroGraphics/deepSky.c astroGraphics/deepSkyOutlines.c \
             astroGraphics/ephemeris.c astroGraphics/galaxyMap.c astroGraphics/greatCircles.c \
             astroGraphics/raDecLines.c astroGraphics/starListReader.c astroGraphics/stars.c coreUtils/asciiDouble.c \
             coreUtils/errorReport.c coreUtils/makeRasters.c listTools/ltDict.c listTools/ltList.c \
             listTools/ltMemory.c listTools/ltStringProc.c mathsTools/julianDate.c mathsTools/projection.c \
             mathsTools/sphericalTrig.c  settings/chart_config.c vectorGraphics/lineDraw.c \
             vectorGraphics/cairo_page.c

CORE_HEADERS = astroGraphics/constellations.h astroGraphics/deepSky.h astroGraphics/deepSkyOutlines.h \
               astroGraphics/ephemeris.h astroGraphics/galaxyMap.h astroGraphics/greatCircles.h \
               astroGraphics/raDecLines.h astroGraphics/starListReader.h astroGraphics/stars.h coreUtils/asciiDouble.h \
               coreUtils/errorReport.h coreUtils/makeRasters.h coreUtils/strConstants.h listTools/ltDict.h \
               listTools/ltList.h listTools/ltMemory.h listTools/ltStringProc.h mathsTools/julianDate.h \
               mathsTools/projection.h mathsTools/sphericalTrig.h settings/chart_config.h vectorGraphics/lineDraw.h \
               vectorGraphics/cairo_page.h

STARCHART_FILES = main.c

STARCHART_HEADERS =

CORE_SOURCES              = $(CORE_FILES:%.c=$(LOCAL_SRCDIR)/%.c)
CORE_OBJECTS              = $(CORE_FILES:%.c=$(LOCAL_OBJDIR)/%.o)
CORE_OBJECTS_DEBUG        = $(CORE_OBJECTS:%.o=%.debug.o)
CORE_HFILES               = $(CORE_HEADERS:%.h=$(LOCAL_SRCDIR)/%.h) Makefile

STARCHART_SOURCES         = $(STARCHART_FILES:%.c=$(LOCAL_SRCDIR)/%.c)
STARCHART_OBJECTS         = $(STARCHART_FILES:%.c=$(LOCAL_OBJDIR)/%.o)
STARCHART_OBJECTS_DEBUG   = $(STARCHART_OBJECTS:%.o=%.debug.o)
STARCHART_HFILES          = $(STARCHART_HEADERS:%.h=$(LOCAL_SRCDIR)/%.h) Makefile

ALL_HFILES = $(CORE_HFILES) $(STARCHART_HFILES)

SWITCHES = -D DCFVERSION=\"$(VERSION)\"  -D DATE=\"$(DATE)\"  -D PATHLINK=\"$(PATHLINK)\"  -D SRCDIR=\"$(CWD)/$(LOCAL_SRCDIR)/\"

all: $(LOCAL_BINDIR)/starchart.bin $(LOCAL_BINDIR)/debug/starchart.bin

#
# General macros for the compile steps
#

$(LOCAL_OBJDIR)/%.o:         $(LOCAL_SRCDIR)/%.c $(ALL_HFILES)
	mkdir -p $(LOCAL_OBJDIR) $(LOCAL_OBJDIR)/astroGraphics $(LOCAL_OBJDIR)/coreUtils $(LOCAL_OBJDIR)/listTools $(LOCAL_OBJDIR)/mathsTools $(LOCAL_OBJDIR)/settings $(LOCAL_OBJDIR)/vectorGraphics
	$(COMPILE) $(OPTIMISATION) $(NODEBUG) $(SWITCHES) $< -o $@

$(LOCAL_OBJDIR)/%.debug.o:   $(LOCAL_SRCDIR)/%.c $(ALL_HFILES)
	mkdir -p $(LOCAL_OBJDIR) $(LOCAL_OBJDIR)/astroGraphics $(LOCAL_OBJDIR)/coreUtils $(LOCAL_OBJDIR)/listTools $(LOCAL_OBJDIR)/mathsTools $(LOCAL_OBJDIR)/settings $(LOCAL_OBJDIR)/vectorGraphics
	$(COMPILE) $(OPTIMISATION) $(DEBUG)   $(SWITCHES) $< -o $@

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

#
# Clean macros
#

clean:
	rm -vfR $(LOCAL_OBJDIR) $(LOCAL_BINDIR)

afresh: clean all

