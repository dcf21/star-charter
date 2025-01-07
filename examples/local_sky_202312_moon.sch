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
projection=stereographic
horizon_latitude=52
horizon_longitude=0
width=14.0
aspect=1
horizon_latitude=52
horizon_longitude=0
plot_ecliptic=0
plot_dso=0
plot_galaxy_map=0
constellation_sticks=0
constellation_names=0
mag_min=2.5
horizon_cardinal_points_marker_count=16
scale_bar=0.1,0.4,0,10
scale_bar_col=1,1,1
ephemeris_style=side_by_side

# Chart at 07:00 on 1 Dec 2023
CHART
copyright=2023 December 1, the Moon is lining up with Pollux and Castor, shortly before sunrise
text=page,0.05,0.05,-1,0,3,0,0,0,Morning 07:00, 1 Dec 2023
alt_central=35
az_central=265
angular_width=35.0
julian_date=2460279.79167  # Sat 2023 Dec 1 07:00:00
draw_ephemeris=P301,2460279.79167,2460280.791679
ephemeris_epochs=2460279.79167
ephemeris_epoch_labels=Moon
output_filename=output/local_sky_202312_moon_00.png

# Chart at 07:00 on 8-9 Dec 2023
CHART
copyright=2023 December 8-9, in morning twilight, the Moon passes Spica and the very bright Venus
text=page,0.05,0.05,-1,0,3,0,0,0,Morning 07:00, 1 Dec 2023
alt_central=22
az_central=155
angular_width=25.0
julian_date=2460286.79167  # Sat 2023 Dec 8 07:00:00
draw_ephemeris=P301,2460286.79167,2460287.791679
ephemeris_epochs=2460286.79167
ephemeris_epoch_labels=8
ephemeris_epochs=2460287.79167
ephemeris_epoch_labels=9
output_filename=output/local_sky_202312_moon_01.png

# Chart at 07:00 on 17-18 Dec 2023
CHART
copyright=2023 December 17-18, the Moon passes between Saturn and Fomalhaut
text=page,0.05,0.05,-1,0,3,0,0,0,Evening 18:00, 1 Dec 2023
alt_central=17
az_central=200
angular_width=30.0
julian_date=2460296.25  # Sat 2023 Dec 17 18:00:00
draw_ephemeris=P301,2460296.25,2460297.259
ephemeris_epochs=2460296.25
ephemeris_epoch_labels=17
ephemeris_epochs=2460297.25
ephemeris_epoch_labels=18
output_filename=output/local_sky_202312_moon_02.png

# Chart at 17:00 on 22-24 Dec 2023
CHART
copyright=2023 December 22-24, the Moon passes Jupiter and Uranus
text=page,0.05,0.05,-1,0,3,0,0,0,Evening 17:00, 1 Dec 2023
alt_central=30
az_central=100
angular_width=40.0
julian_date=2460302.2083  # Sat 2023 Dec 23 17:00:00
draw_ephemeris=P301,2460301.20833333,2460303.208333339
ephemeris_epochs=2460301.20833333
ephemeris_epoch_labels=22
ephemeris_epochs=2460302.20833333
ephemeris_epoch_labels=23
ephemeris_epochs=2460303.20833333
ephemeris_epoch_labels=24
output_filename=output/local_sky_202312_moon_03.png

# Chart at 20:00 on 27-28 Dec 2023
CHART
copyright=2023 December 27-28, the Full Moon passes below Pollux and Castor
text=page,0.05,0.05,-1,0,3,0,0,0,Evening 20:00, 1 Dec 2023
alt_central=26
az_central=83
angular_width=38.0
julian_date=2460306.3333  # Wed 2023 Dec 27 20:00:00
draw_ephemeris=P301,2460306.33333,2460307.333339
ephemeris_epochs=2460306.33333
ephemeris_epoch_labels=27
ephemeris_epochs=2460307.33333
ephemeris_epoch_labels=28
output_filename=output/local_sky_202312_moon_04.png
