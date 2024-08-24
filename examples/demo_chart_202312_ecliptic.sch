# Demonstration configuration file which produces a diagram of the
# paths of the planets along the ecliptic 2023
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
coords=ra_dec
show_horizon=0
projection=flat
ra_central=6
dec_central=0
angular_width=360.0
width=28.0
aspect=0.22
constellation_sticks=1
star_names=0
constellation_names=0
shade_twilight=0
shade_not_observable=1
horizon_latitude=49
horizon_longitude=0
plot_equator=1
plot_ecliptic=1
plot_dso=0
plot_galaxy_map=0
show_solar_system=0
mag_min=4.5
ephemeris_style=side_by_side_with_arrow

# Chart at midnight on 15 Dec 2023
CHART
copyright=The path of the Sun and planets along the ecliptic in December
julian_date=2460293.5  # Fri 2023 Dec 15 00:00:00
output_filename=output/demo_chart_202312_ecliptic_00.png

ephemeris_epochs=2460279.5  # 2023 Dec 1 00:00:00
ephemeris_epoch_labels=
ephemeris_epochs=2460310.5  # 2024 Jan 1 00:00:00
ephemeris_epoch_labels=

draw_ephemeris=P1,2460279.5,2460310.59  # Mercury
draw_ephemeris=P2,2460279.5,2460310.59  # Venus
draw_ephemeris=P4,2460279.5,2460310.59  # Mars
draw_ephemeris=P5,2460279.5,2460310.59  # Jupiter
draw_ephemeris=P6,2460279.5,2460310.59  # Saturn
