# Demonstration configuration file which produces charts of the paths
# of the planets in 2023
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
coords=ra_dec
width=20.0
show_horizon=0
constellation_sticks=1
star_names=1
constellation_names=1
shade_twilight=0
shade_not_observable=0
horizon_latitude=49
horizon_longitude=0
plot_equator=0
plot_ecliptic=0
plot_dso=0
plot_galaxy_map=1
galaxy_col=0.2,0.2,0.5
galaxy_col0=0.2,0.2,0.5
show_solar_system=0
ephemeris_style=side_by_side_with_arrow

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
copyright=The path of Mars from January to September 2023
text=page,0.52,0.36,0,0,1.5,1,0.7,0.6,MARS
draw_ephemeris=P4,2459945.5,2460192.5  # Mars
ra_central=8.5
dec_central=10
angular_width=140
aspect=0.4
mag_size_norm=0.75
mag_min=4.9
output_filename=output/demo_chart_2023_mars.png
ephemeris_col=1,0.7,0.6  # Mars
ephemeris_arrow_col=1,0.7,0.6  # Mars
solar_system_col=1,0.7,0.6  # Mars

# Remaining ephemerides extend to the end of 2023
DEFAULTS
width=20.0
constellation_boundaries=1
star_bayer_labels=1
star_flamsteed_labels=1
ephemeris_epochs=2460218.5  # 2023 Oct 1 00:00:00
ephemeris_epoch_labels=10
ephemeris_epochs=2460249.5  # 2023 Nov 1 00:00:00
ephemeris_epoch_labels=11
ephemeris_epochs=2460279.5  # 2023 Dec 1 00:00:00
ephemeris_epoch_labels=12
ephemeris_epochs=2460310.5  # 2024 Jan 1 00:00:00
ephemeris_epoch_labels=01/2024

# Chart for Jupiter
CHART
copyright=The path of Jupiter in 2023
text=page,0.52,0.33,0,0,1.5,1,0.7,1,JUPITER
draw_ephemeris=P5,2459945.5,2460320.5  # Jupiter
ra_central=1.5
dec_central=5
angular_width=60
aspect=0.4
mag_size_norm=0.5
mag_min=6.8
output_filename=output/demo_chart_2023_jupiter.png
ephemeris_col=1,0.7,1  # Jupiter
ephemeris_arrow_col=1,0.7,1  # Jupiter
solar_system_col=1,0.7,1  # Jupiter

# Chart for Saturn
CHART
copyright=The path of Saturn in 2023
text=page,0.52,0.6,0,0,1.5,1,1,0.6,SATURN
draw_ephemeris=P6,2459945.5,2460320.5  # Saturn
ra_central=22.2
dec_central=-12
angular_width=20
aspect=0.75
mag_min=7.6
mag_size_norm=0.75
output_filename=output/demo_chart_2023_saturn.png
ephemeris_col=1,1,0.6  # Saturn
ephemeris_arrow_col=1,1,0.6  # Saturn
solar_system_col=1,1,0.6  # Saturn

# Chart for Uranus
CHART
copyright=The path of Uranus in 2023
text=page,0.52,0.36,0,0,1.5,0.9,0.9,1,URANUS
draw_ephemeris=P7,2459945.5,2460320.5  # Uranus
ra_central=3.1
dec_central=17
angular_width=10
aspect=0.8
mag_min=9.0
mag_size_norm=0.5
output_filename=output/demo_chart_2023_uranus.png
ephemeris_col=0.9,0.9,1
ephemeris_arrow_col=0.9,0.9,1
solar_system_col=0.9,0.9,1

# Chart for Neptune
CHART
copyright=The path of Neptune in 2023
text=page,0.52,0.36,0,0,1.5,0.9,0.9,1,NEPTUNE
draw_ephemeris=P8,2459945.5,2460320.5  # Neptune
ra_central=23.74
dec_central=-3
angular_width=8.5
aspect=0.8
mag_min=9.4
mag_size_norm=0.75
star_flamsteed_labels=1
output_filename=output/demo_chart_2023_neptune.png
ephemeris_col=0.9,0.9,1
ephemeris_arrow_col=0.9,0.9,1
solar_system_col=0.9,0.9,1
