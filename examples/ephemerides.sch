# Demonstration configuration file to show the paths of a selection of objects
# across the sky in 2025
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
INCLUDE include_files/colour_scheme_pastel.sch
width=32.0
show_grid_lines=1
constellation_boundaries=1
constellation_sticks=1
star_names=1
star_flamsteed_labels=0
constellation_names=1
plot_galaxy_map=1
ephemeris_autoscale=1
ephemeris_table=1

# Produce Jupiter chart
CHART
draw_ephemeris=jupiter,2460676.5,2461041.5
output_filename=output/jupiter_2025.png

# Produce Saturn chart
CHART
draw_ephemeris=saturn,2460676.5,2461041.5
output_filename=output/saturn_2025.png

# Produce Moon chart
CHART
draw_ephemeris=p301,2460678.5,2460685.5  # Use 'P301' here, not 'Moon', to ensure phases are drawn
ephemeris_style=side_by_side
ephemeris_table=0
must_show_all_ephemeris_labels=1
star_clip_outline=1  # This makes the Pleiades stand out more clearly, but can be slow on wide-field charts
output_filename=output/moon_2025_01.png

# Produce A1 finder chart
CHART
draw_ephemeris=A1,2460676.5,2460766.5
mag_min=7.5
output_filename=output/a1_2025.png

# Produce 1P/Halley finder chart
CHART
draw_ephemeris=0001P,2460676.5,2460766.5
mag_min=12
ephemeris_style=side_by_side_with_track
output_filename=output/c1_2025.png
