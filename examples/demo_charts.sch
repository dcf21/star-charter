# Demonstration configuration file to produce a selection of example star
# charts
# 
# -------------------------------------------------
# Copyright 2015-2022 Dominic Ford
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

# A demo chart which shows the Bayer designations of stars in and around Orion
CHART
output_filename=output/orion_bayer.png
ra_central=5.5
dec_central=0.0
ra_dec_lines=0
angular_width=40.0
width=20
aspect=1.41421356
constellation_boundaries=1
constellation_sticks=1
coords=ra_dec
projection=gnomonic
star_names=0
star_bayer_labels=1
star_flamsteed_labels=0
star_mag_labels=0
plot_dso=0
dso_mags=1
dso_mag_min=10
constellation_names=1

# A demo chart which shows the HR numbers of stars in the constellation Orion
CHART
output_filename=output/ybsn.png
ra_central=5.5
dec_central=0.0
angular_width=25.0
width=20.0
constellation_boundaries=1
constellation_sticks=1
coords=ra_dec
projection=flat
star_names=0
constellation_names=0
star_catalogue_numbers=1
star_catalogue=ybsc

# A demo chart which shows the constellation of Ursa Major, with simplified stick figure
CHART
output_filename=output/dipper_1.png
title=Simplified constellation stick figures
ra_central=11.5
dec_central=50.0
angular_width=65.0
width=20
aspect=0.70711
constellation_boundaries=1
constellation_sticks=1
coords=ra_dec
projection=gnomonic
star_names=1
constellation_names=1
great_circle_key=0
maximum_star_label_count=20
dso_mag_min=10
constellation_stick_col=0,0.25,1

# A demo chart which shows the constellation of Ursa Major, with stick figure by H.A. Rey
CHART
output_filename=output/dipper_2.png
title=Constellation stick figures by H.A. Rey
ra_central=11.5
dec_central=50.0
angular_width=65.0
width=20
aspect=0.70711
constellation_boundaries=1
constellation_sticks=1
constellation_stick_design=rey
coords=ra_dec
projection=gnomonic
star_names=1
constellation_names=1
great_circle_key=0
maximum_star_label_count=20
dso_mag_min=10
constellation_stick_col=0,0.25,1

