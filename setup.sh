#!/bin/bash
#
# -------------------------------------------------
# Copyright 2015-2026 Dominic Ford
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

cd "$(dirname "$0")" || exit 1
cwd=`pwd`

# Download all of the data we need from the internet
echo "[`date`] Downloading required data files"
cd ${cwd} || exit 1
./dataFetch.py "$@" || exit 1

# Delete old binary ephemeris files
echo "[`date`] Cleaning old binary files"
cd ${cwd} || exit 1
rm -f data/binary_*.bin

# Convert the constellation stick figures, defined by the HIP numbers of stars, into RA/Dec
echo "[`date`] Creating constellation stick figures"
cd ${cwd} || exit 1
cd data/constellations/process_stick_figures || exit 1
./constellationLines.py || exit 1

# Make a blurred map of the Milky Way to use as a background for star charts
echo "[`date`] Generating map of the Milky Way"
cd ${cwd} || exit 1
cd data_processing/milky_way_map || exit 1
make clean || exit 1
./prettymake || exit 1
./bin/galaxymap.bin || exit 1

# Check output was correctly generated
cd ${cwd} || exit 1
if [ -f data_generated/milky_way_map/galaxymap.dat ]; then
   echo ">> Output correctly generated"
else
   echo ">> Output is missing. Aborting."
   exit 1
fi

# Query the NED website for the redshifts and distances of deep sky objects
# We don't do this automatically, since it takes around 12 hours. Used cached version instead.
echo "[`date`] Fetching NGC distance data"
cd ${cwd} || exit 1
cd data_processing/deep_sky_merge_catalogues || exit 1
# ./fetch_distances_from_ned.py || exit 1

# Check output was correctly generated
cd ${cwd} || exit 1
if [ -f data_generated/deep_sky_merged_catalogue/NED_distances.dat ]; then
   echo ">> Output correctly generated"
else
   echo ">> Output is missing. Aborting."
   exit 1
fi

# Merge data from various NGC/IC data sources
echo "[`date`] Merging deep-sky catalogues"
cd ${cwd} || exit 1
cd data_processing/deep_sky_merge_catalogues || exit 1
./merge.py || exit 1

# Check output was correctly generated
cd ${cwd} || exit 1
if [ -f data_generated/deep_sky_merged_catalogue/ngc_merged.dat ]; then
   echo ">> Output correctly generated"
else
   echo ">> Output is missing. Aborting."
   exit 1
fi

# Merge data from various star catalogues
# Below it is possible to set a magnitude limit of 10
echo "[`date`] Merging star catalogues"
cd ${cwd} || exit 1
cd data_processing/star_catalogue_merged || exit 1
./main_catalogue_merge.py || exit 1 # --magnitude-limit 10

# Check output was correctly generated
cd ${cwd} || exit 1
if [ -f data_generated/star_catalogue_merged/star_charter_stars.dat.gz ]; then
   echo ">> Output correctly generated"
else
   echo ">> Output is missing. Aborting."
   exit 1
fi

# Compile the StarCharter code
echo "[`date`] Compiling code"
cd ${cwd} || exit 1
./prettymake clean || exit 1
./prettymake || exit 1

# Make some test charts of the paths of solar system objects, to ensure all ASCII input ephemeris files are converted
# to binary for rapid access on subsequent queries
echo "[`date`] Generating test charts"
cd ${cwd} || exit 1
./runExamples.py "$@" || exit 1

# Finished
cd ${cwd} || exit 1
echo "[`date`] Finishing DoAll script"
