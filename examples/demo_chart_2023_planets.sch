# Demonstration configuration file which produces charts of the paths
# of the planets in 2023
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
coords=ra_dec
width=20.0
show_horizon=0
constellation_sticks=1
star_names=0
constellation_names=1
shade_twilight=0
shade_not_observable=0
horizon_latitude=49
horizon_longitude=0
plot_equator=0
plot_ecliptic=0
plot_galactic_plane=0
plot_dso=0
plot_galaxy_map=1
dso_symbol_key=0
great_circle_key=0
magnitude_key=0
show_solar_system=0
ephemeris_style=side_by_side_with_arrow

ephemeris_col=1,1,0.7
ephemeris_arrow_col=1,1,0.7
solar_system_col=1,1,0.7

ephemeris_epochs=2459945.5  # 2023 Jan 1 00:00:00
ephemeris_epoch_labels=01/2023
ephemeris_epochs=2459976.5  # 2023 Feb 1 00:00:00
ephemeris_epoch_labels=02
ephemeris_epochs=2460004.5  # 2023 Mar 1 00:00:00
ephemeris_epoch_labels=03
ephemeris_epochs=2460035.5  # 2023 Apr 1 00:00:00
ephemeris_epoch_labels=04
ephemeris_epochs=2460065.5  # 2023 May 1 00:00:00
ephemeris_epoch_labels=05
ephemeris_epochs=2460096.5  # 2023 Jun 1 00:00:00
ephemeris_epoch_labels=06
ephemeris_epochs=2460126.5  # 2023 Jul 1 00:00:00
ephemeris_epoch_labels=07
ephemeris_epochs=2460157.5  # 2023 Aug 1 00:00:00
ephemeris_epoch_labels=08
ephemeris_epochs=2460188.5  # 2023 Sep 1 00:00:00
ephemeris_epoch_labels=09

# Chart for Mars
CHART
projection=flat
title=The path of Mars from January to September 2023
text=page,0.52,0.4,0,0,1.5,1,0,1,1,0.7,MARS
draw_ephemeris=P4,2459945.5,2460192.5  # Mars
ra_central=8.5
dec_central=10
angular_width=140
aspect=0.4
mag_size_norm=0.75
mag_min=4.9
output_filename=output/demo_chart_2023_mars.png

# Remaining ephemerides have more labels
DEFAULTS
constellation_boundaries=1
star_names=1
star_bayer_labels=1
star_flamsteed_labels=0

# Chart for Jupiter
CHART
title=The path of Jupiter from January to September 2023
text=page,0.54,0.31,0,0,1.5,1,0,1,1,0.7,JUPITER
draw_ephemeris=P5,2459945.5,2460320.5  # Jupiter
ra_central=1.5
dec_central=5
angular_width=60
aspect=0.4
mag_size_norm=0.5
mag_min=6.8
output_filename=output/demo_chart_2023_jupiter.png

# Shrink plot size for outermost planets
DEFAULTS
width=16

# Chart for Saturn
CHART
title=The path of Saturn from January to September 2023
text=page,0.52,0.6,0,0,1.5,1,0,1,1,0.7,SATURN
draw_ephemeris=P6,2459945.5,2460320.5  # Saturn
ra_central=22.2
dec_central=-12
angular_width=20
aspect=0.75
mag_min=7.6
mag_size_norm=0.75
output_filename=output/demo_chart_2023_saturn.png

# Chart for Uranus
CHART
title=The path of Uranus from January to September 2023
text=page,0.52,0.36,0,0,1.5,1,0,1,1,0.7,URANUS
draw_ephemeris=P7,2459945.5,2460320.5  # Uranus
ra_central=3.1
dec_central=17
angular_width=10
aspect=0.8
mag_min=9.0
mag_size_norm=0.5
output_filename=output/demo_chart_2023_uranus.png

# Chart for Neptune
CHART
title=The path of Neptune from January to September 2023
text=page,0.52,0.36,0,0,1.5,1,0,1,1,0.7,NEPTUNE
draw_ephemeris=P8,2459945.5,2460320.5  # Neptune
ra_central=23.74
dec_central=-3
angular_width=8.5
aspect=0.8
mag_min=9.4
mag_size_norm=0.75
star_flamsteed_labels=1
output_filename=output/demo_chart_2023_neptune.png
