#!/bin/bash
#
# -------------------------------------------------
# Copyright 2015-2024 Dominic Ford
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

# Do all of the tasks we need to get StarCharter up and running

cd "$(dirname "$0")" || exit
cwd=`pwd`

# Download all of the data we need from the internet
echo "Downloading required data files"
cd ${cwd} || exit
./dataFetch.py

# Convert the constellation stick figures, defined by the HIP numbers of stars, into RA/Dec
cd ${cwd} || exit
cd data/constellations/process_stick_figures || exit
./constellationLines.py

# Make a blurred map of the Milky Way to use as a background for star charts
cd ${cwd} || exit
cd data/milkyWay/process || exit
./prettymake
./bin/galaxymap.bin

# Query the NED website for the redshifts and distances of deep sky objects
# We don't do this automatically, since it takes around 12 hours. Used cached version instead.
cd ${cwd} || exit
cd data/deepSky/ngcDistances || exit
# ./fetch_distances_from_ned.py

# Merge data from various NGC/IC data sources
cd ${cwd} || exit
cd data/deepSky/ngcDistances || exit
./merge.py

# Merge data from various star catalogues
# Below we set a magnitude limit of 10, to minimise memory usage to < 4 GB.
# This can be safely reduced to 14 if you have 16 GB of RAM.
cd ${cwd} || exit
cd data/stars/starCataloguesMerge || exit
./main_catalogue_merge.py --magnitude-limit 10

# Compile the StarCharter code
echo "Compiling code"
cd ${cwd} || exit
./prettymake

# Make a single example, to ensure text input files are converted to binary
cd ${cwd} || exit
cd examples
../bin/starchart.bin jupiter_ephemeris.sch

