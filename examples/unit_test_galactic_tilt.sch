# A unit test which produces a map of in galactic coordinates, and applies a small tilt
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

# Common settings which apply to all the charts we produce
DEFAULTS
coords=galactic
grid_coords=galactic
galactic_b_central=50
galactic_l_central=0
angular_width=130.0
width=36.0
aspect=0.7
constellation_boundaries=1
constellation_sticks=1
projection=stereographic
star_names=1
maximum_star_label_count=20
dso_names=0
constellation_names=1

CHART
output_filename=output/unit_test_galactic_tilt_00.png
position_angle=-15

CHART
output_filename=output/unit_test_galactic_tilt_01.png
position_angle=0

CHART
output_filename=output/unit_test_galactic_tilt_02.png
position_angle=15
