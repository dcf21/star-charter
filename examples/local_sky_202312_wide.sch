# Demonstration configuration file which produces a map of the local sky
# seen from London on 1 Dec 2023
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
INCLUDE include_files/twilight_colour_scheme.sch
INCLUDE include_files/twilight_ephemeris_style.sch
INCLUDE include_files/meteor_showers_december.sch
alt_central=50
angular_width=180.0
width=28.0
aspect=0.7
projection=stereographic
horizon_latitude=52
horizon_longitude=0
horizon_cardinal_points_marker_col=0,0,0
horizon_cardinal_points_labels_col=0,0,0
copyright=The sky from London at 11pm on 1 Dec 2023
julian_date=2460280.4583  # Fri 2023 Dec 1 23:00:00

# Chart at 11pm on 1 Dec 2023
CHART
az_central=180
output_filename=output/local_sky_202312_wide_00.png

CHART
az_central=180
output_filename=output/local_sky_202312_wide_00.pdf

CHART
az_central=180
output_filename=output/local_sky_202312_wide_00.svg

CHART
az_central=0
output_filename=output/local_sky_202312_wide_01.png

CHART
az_central=0
output_filename=output/local_sky_202312_wide_01.pdf

CHART
az_central=0
output_filename=output/local_sky_202312_wide_01.svg
