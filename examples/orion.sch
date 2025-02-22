# Demonstration configuration file to show the constellation of Orion
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

# Default settings which affect all the charts below
DEFAULTS
INCLUDE include_files/colour_scheme_light_bg.sch
ra_central=5.5
dec_central=4.0
angular_width=29.0
mag_min=6
dso_mag_min=8
width=13.0
aspect=1.41421356
show_grid_lines=1
constellation_boundaries=1
constellation_sticks=1
coords=ra_dec
projection=stereographic
star_names=1
star_flamsteed_labels=0
constellation_names=1
plot_galaxy_map=1
plot_equator=1
plot_ecliptic=0
plot_galactic_plane=0

# Produce a default version of this star chart
CHART
output_filename=output/orion_default.png

# Show Bayer designation
CHART
star_names=0
star_bayer_labels=1
star_flamsteed_labels=0
star_mag_labels=off
mag_min=5
dso_mag_min=8
output_filename=output/orion_bayer.png

# Show star catalogue numbers
CHART
star_names=0
constellation_names=0
star_catalogue_numbers=1
star_catalogue=hipparcos
mag_min=4.5
dso_mag_min=8
output_filename=output/orion_hip.png

# Show star magnitudes
CHART
star_names=0
star_mag_labels=on
mag_min=5
dso_mag_min=8
output_filename=output/orion_magnitudes.png
