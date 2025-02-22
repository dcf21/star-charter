# Demonstration configuration file to produce an alt/az projection
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

# Default chart settings
DEFAULTS
INCLUDE include_files/colour_scheme_pastel.sch
ra_central=5.5
dec_central=52.0
position_angle=15
width=18.0
constellation_sticks=1
mag_min=4.0
dso_mag_min=6
star_names=1
star_label_mag_min=1.5
plot_equator=1
plot_galactic_plane=1
plot_ecliptic=1
magnitude_key=0
great_circle_key=0

# Produce a copy of this chart using alt/az projection of the whole sky, with specified central point at the zenith
CHART
output_filename=output/alt_az_projection.png
projection=alt_az

# Produce a copy of this chart which shows the sky wrapped onto a sphere viewed from the outside
CHART
output_filename=output/alt_az_sphere_projection.png
projection=sphere

