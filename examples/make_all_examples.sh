#!/bin/bash
# make_all_examples.sh
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

./all_constellations.py
../bin/starchart.bin all_sky_maps.sch
../bin/starchart.bin alt_az.sch
../bin/starchart.bin background_image_demo.sch
../bin/starchart.bin cygnus_photo_overlay.sch
../bin/starchart.bin demo_charts.sch
../bin/starchart.bin galactic_coordinates.sch
../bin/starchart.bin jupiter_ephemeris.sch
../bin/starchart.bin orion.sch
../bin/starchart.bin peters.sch
../bin/starchart.bin venus_ephemeris.sch

