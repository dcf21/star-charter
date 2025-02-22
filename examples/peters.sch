# Demonstration configuration file which produces a chart of the whole sky
# in a Peters projection
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

# Common settings which apply to all the charts we produce
DEFAULTS
INCLUDE include_files/colour_scheme_light_bg.sch
ra_central=12.0
dec_central=0.0
angular_width=360.0
width=36.0
constellation_boundaries=1
constellation_sticks=1
constellation_boundaries=0
coords=ra_dec
projection=peters
star_names=1
maximum_star_label_count=20
dso_names=0
constellation_names=1

# Chart with a grid of equatorial coordinates (RA/Dec)
CHART
output_filename=output/peters_equatorial.png
grid_coords=ra_dec

# Chart with a grid of galactic coordinates
CHART
output_filename=output/peters_galactic.png
grid_coords=galactic
