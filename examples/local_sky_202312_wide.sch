# Demonstration configuration file which produces a map of the local sky
# seen from London on 1 Dec 2023
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
INCLUDE include_files/colour_scheme_pastel.sch
INCLUDE include_files/meteor_showers_12_december.sch
coords=alt_az
grid_coords=alt_az
show_horizon=1
horizon_cardinal_points_marker_count=16
az_central=180
alt_central=42
angular_width=120.0
width=21.0
aspect=0.7
projection=stereographic
mag_min=4
dso_mag_min=3
horizon_latitude=52
horizon_longitude=0
shade_twilight=1
copyright=The sky from London at 11pm on 1 Dec 2023
julian_date=2460280.4583  # Fri 2023 Dec 1 23:00:00

# Chart at 11pm on 1 Dec 2023
CHART
output_filename=output/local_sky_202312_wide.png

CHART
output_filename=output/local_sky_202312_wide.pdf

CHART
output_filename=output/local_sky_202312_wide.svg
