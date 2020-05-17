# Makefile for galaxy map generator
# 
# -------------------------------------------------
# Copyright 2015-2019 Dominic Ford
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

VERSION = 1.0
DATE    = 29/10/2017
PATHLINK= /

WARNINGS= -Wall -Wno-format-truncation
COMPILE = $(CC) $(WARNINGS) -g -fopenmp -c -I $(CWD)/src
LIBS    = -lpng -lgsl -lgslcblas -lm
LINK    = $(CC) $(WARNINGS) -g -fopenmp

OPTIMISATION = -O3

DEBUG   = -D DEBUG=1 -D MEMDEBUG1=1 -D MEMDEBUG2=0
NODEBUG = -D DEBUG=0 -D MEMDEBUG1=0 -D MEMDEBUG2=0

LOCAL_SRCDIR = src
LOCAL_OBJDIR = obj
LOCAL_BINDIR = bin

CORE_FILES = coreUtils/asciiDouble.c coreUtils/errorReport.c image_in.c image_out.c

CORE_HEADERS = coreUtils/asciiDouble.h coreUtils/errorReport.h coreUtils/strConstants.h image.h

GALAXYMAP_FILES = main.c

GALAXYMAP_HEADERS = main.h

CORE_SOURCES              = $(CORE_FILES:%.c=$(LOCAL_SRCDIR)/%.c)
CORE_OBJECTS              = $(CORE_FILES:%.c=$(LOCAL_OBJDIR)/%.o)
CORE_OBJECTS_DEBUG        = $(CORE_OBJECTS:%.o=%.debug.o)
CORE_HFILES               = $(CORE_HEADERS:%.h=$(LOCAL_SRCDIR)/%.h) Makefile

GALAXYMAP_SOURCES         = $(GALAXYMAP_FILES:%.c=$(LOCAL_SRCDIR)/%.c)
GALAXYMAP_OBJECTS         = $(GALAXYMAP_FILES:%.c=$(LOCAL_OBJDIR)/%.o)
GALAXYMAP_OBJECTS_DEBUG   = $(GALAXYMAP_OBJECTS:%.o=%.debug.o)
GALAXYMAP_HFILES          = $(GALAXYMAP_HEADERS:%.h=$(LOCAL_SRCDIR)/%.h) Makefile

ALL_HFILES = $(CORE_HFILES) $(GALAXYMAP_HFILES)

SWITCHES = -D DCFVERSION=\"$(VERSION)\"  -D DATE=\"$(DATE)\"  -D PATHLINK=\"$(PATHLINK)\"  -D SRCDIR=\"$(CWD)/$(LOCAL_SRCDIR)/\"

all: $(LOCAL_BINDIR)/galaxymap.bin $(LOCAL_BINDIR)/debug/galaxymap.bin

#
# General macros for the compile steps
#

$(LOCAL_OBJDIR)/%.o:         $(LOCAL_SRCDIR)/%.c $(ALL_HFILES)
	mkdir -p $(LOCAL_OBJDIR) $(LOCAL_OBJDIR)/coreUtils
	$(COMPILE) $(OPTIMISATION) $(NODEBUG) $(SWITCHES) $< -o $@

$(LOCAL_OBJDIR)/%.debug.o:   $(LOCAL_SRCDIR)/%.c $(ALL_HFILES)
	mkdir -p $(LOCAL_OBJDIR) $(LOCAL_OBJDIR)/coreUtils
	$(COMPILE) $(OPTIMISATION) $(DEBUG)   $(SWITCHES) $< -o $@

#
# Make the binaries
#

$(LOCAL_BINDIR)/galaxymap.bin: $(CORE_OBJECTS) $(GALAXYMAP_OBJECTS)
	mkdir -p $(LOCAL_BINDIR)
	$(LINK) $(OPTIMISATION) $(CORE_OBJECTS) $(GALAXYMAP_OBJECTS) $(LIBS) -o $(LOCAL_BINDIR)/galaxymap.bin

$(LOCAL_BINDIR)/debug/galaxymap.bin: $(CORE_OBJECTS_DEBUG) $(GALAXYMAP_OBJECTS_DEBUG)
	mkdir -p $(LOCAL_BINDIR)/debug
	echo "The files in this directory are binaries with debugging options enabled: they produce activity logs called 'galaxymap.log'. It should be noted that these binaries can up to ten times slower than non-debugging versions." > $(LOCAL_BINDIR)/debug/README
	$(LINK) $(OPTIMISATION) $(CORE_OBJECTS_DEBUG) $(GALAXYMAP_OBJECTS_DEBUG) $(LIBS) -o $(LOCAL_BINDIR)/debug/galaxymap.bin

#
# Clean macros
#

clean:
	rm -vfR $(LOCAL_OBJDIR) $(LOCAL_BINDIR)

afresh: clean all

