# Demonstration configuration file to show the constellation of Orion
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

# Default settings which affect all the charts below
DEFAULTS
ra_central=5.5
dec_central=4.0
angular_width=29.0
mag_min=7
width=15.0
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
plot_equator=0
plot_ecliptic=0
plot_galactic_plane=1
font_size=1.2

# Produce a PNG copy of this star chart
CHART
output_filename=output/orion.png

# Produce a SVG copy of this star chart
CHART
output_filename=output/orion.svg

# Produce a PDF copy of this star chart
CHART
output_filename=output/orion.pdf

# Produce an EPS copy of this star chart
CHART
output_filename=output/orion.eps

# Produce a smaller version of this chart
CHART
width=12
font_size=1.1
output_filename=output/orion_mini.png

