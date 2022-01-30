#!/bin/bash
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

# Do all of the tasks we need to get StarCharter up and running

cd "$(dirname "$0")"
cwd=`pwd`

# Download all of the data we need from the internet
echo "Downloading required data files"
cd ${cwd}
./dataFetch.py

# Convert the constellation stick figures, defined by the HIP numbers of stars, into RA/Dec
cd ${cwd}
cd data/constellations/process_stick_figures
./constellationLines.py

# Make a blurred map of the Milky Way to use as a background for star charts
cd ${cwd}
cd data/milkyWay/process
./prettymake
./bin/galaxymap.bin

# Query the NED website for the redshifts and distances of deep sky objects
# We don't do this automatically, since it takes around 12 hours. Used cached version instead.
cd ${cwd}
cd data/deepSky/ngcDistances
# ./fetch_distances_from_ned.py

# Merge data from various NGC/IC data sources
cd ${cwd}
cd data/deepSky/ngcDistances
./merge.py

# Merge data from various star catalogues
cd ${cwd}
cd data/stars/starCataloguesMerge
./main_catalogue_merge.py

# Compile the StarCharter code
echo "Compiling code"
cd ${cwd}
./prettymake
