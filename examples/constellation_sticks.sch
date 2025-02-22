# A demo configuration file which shows the constellation of Ursa Major,
# with stick figures in all the available schemes.
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

DEFAULTS
INCLUDE include_files/colour_scheme_pastel.sch
ra_central=11.5
dec_central=50.0
angular_width=70.0
width=12
aspect=0.70711
constellation_boundaries=0
constellation_sticks=1
constellation_sticks_line_width=2
coords=ra_dec
projection=stereographic
star_names=1
constellation_names=1
dso_symbol_key=0
magnitude_key=0
great_circle_key=0
maximum_star_label_count=4
dso_mag_min=8
mag_min=5.5

# A demo chart which shows the constellation of Ursa Major, with simplified stick figures
CHART
output_filename=output/constellation_ursa_major_1.png
title=Simplified constellation stick figures
constellation_stick_design=simplified

# A demo chart which shows the constellation of Ursa Major, with stick figures by H.A. Rey
CHART
output_filename=output/constellation_ursa_major_2.png
title=Constellation stick figures by H.A. Rey
constellation_stick_design=rey

# A demo chart which shows the constellation of Ursa Major, with IAU stick figures
CHART
output_filename=output/constellation_ursa_major_3.png
title=Constellation stick figures created by the IAU
constellation_stick_design=iau
