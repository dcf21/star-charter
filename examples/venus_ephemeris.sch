# Demonstration configuration file to show the path of Venus across the sky
# in 2025
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
INCLUDE include_files/colour_scheme_dark_bg.sch
width=24.0
show_grid_lines=1
dso_names=0
constellation_boundaries=1
constellation_sticks=1
star_names=1
star_flamsteed_labels=0
constellation_names=1
plot_galaxy_map=1
ephemeris_autoscale=1
draw_ephemeris=venus,2460676.5,2460827.5

# Produce a PNG copy of this star chart
CHART
output_filename=output/venus_2025.png

# Produce a SVG copy of this star chart
CHART
output_filename=output/venus_2025.svg

# Produce a PDF copy of this star chart
CHART
output_filename=output/venus_2025.pdf

# Produce an EPS copy of this star chart
CHART
output_filename=output/venus_2025.eps
