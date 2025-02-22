# Demonstration configuration file which produces a chart of the whole sky,
# in equatorial and galactic coordinates
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
angular_width=360.0
width=36.0
aspect=0.5
projection=flat
star_names=1
constellation_names=1
constellation_boundaries=0
mag_min=5.0
mag_max=0.5
mag_step=0.5
dso_mag_min=7
maximum_star_label_count=20
axis_ticks_value_only=1
plot_equator=1
plot_galactic_plane=1
plot_ecliptic=1

# Demo chart in ecliptic coordinates
CHART
output_filename=output/coordinates_ecliptic.png
coords=ra_dec
ra_central=12.0
dec_central=0.0
grid_coords=ra_dec

# Demo chart in galactic coordinates
CHART
output_filename=output/coordinates_galactic.png
coords=galactic
galactic_l_central=0.0
galactic_b_central=0.0
grid_coords=galactic
