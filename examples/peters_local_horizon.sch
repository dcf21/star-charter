# Demonstration configuration file which produces a chart of the whole sky
# in a Peters projection, with the local horizon drawn
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

# Common settings which apply all the charts we produce
DEFAULTS
INCLUDE include_files/colour_scheme_pastel.sch
ra_central=12.0
dec_central=0.0
angular_width=360.0
width=24.0
constellation_boundaries=0
constellation_sticks=1
mag_min=4.5
dso_mag_min=4
coords=ra_dec
projection=peters
star_names=0
dso_names=0
constellation_names=1
shade_twilight=1
show_horizon=1
horizon_latitude=52
horizon_longitude=0

# Chart at 52N at midnight on 1 Jan 2000
CHART
output_filename=output/peters_local_N_00:00.png
julian_date=2451544.5  # Sat 2000 Jan 1 00:00:00

# Chart at 52N at midday on 1 Jan 2000
CHART
output_filename=output/peters_local_N_12:00.png
julian_date=2451545  # Sat 2000 Jan 1 12:00:00

# Chart at 30S at midnight on 1 Jan 2000
CHART
output_filename=output/peters_local_S_00:00.png
horizon_latitude=-30
julian_date=2451544.5  # Sat 2000 Jan 1 00:00:00

# Chart at 30S at midday on 1 Jan 2000
CHART
output_filename=output/peters_local_S_12:00.png
horizon_latitude=-30
julian_date=2451545  # Sat 2000 Jan 1 12:00:00
