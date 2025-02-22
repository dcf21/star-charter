# Demonstration configuration file which produces a diagram of the
# paths of the planets along the ecliptic in December 2023
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
show_horizon=0
projection=flat
ra_central=6
dec_central=0
angular_width=360.0
width=24.0
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
plot_galactic_plane=0
plot_dso=0
plot_galaxy_map=0
dso_symbol_key=0
magnitude_key=0
mag_min=4.5
mag_size_norm=0.8
ephemeris_style=side_by_side_with_arrow
ephemeris_arrow_col=1,1,0.7

# Chart at midnight on 15 Dec 2023
CHART
title=The path of the Sun and planets along the ecliptic in December 2023
julian_date=2460293.5  # Fri 2023 Dec 15 00:00:00
output_filename=output/demo_chart_ecliptic_202312.png

solar_system_ids=sun
solar_system_labels=Sun
solar_system_col=1,1,0.8

ephemeris_epochs=2460279.5  # 2023 Dec 1 00:00:00
ephemeris_epoch_labels=
ephemeris_epochs=2460310.5  # 2024 Jan 1 00:00:00
ephemeris_epoch_labels=

draw_ephemeris=P1,2460279.5,2460310.59  # Mercury
draw_ephemeris=P2,2460279.5,2460310.59  # Venus
draw_ephemeris=P4,2460279.5,2460310.59  # Mars
draw_ephemeris=P5,2460279.5,2460310.59  # Jupiter
draw_ephemeris=P6,2460279.5,2460310.59  # Saturn

text=page,0.5,0.94,0,0,1.75,1,0,1,1,0.7,around midnight
text=page,0.2,0.94,0,0,1.75,1,0,1,1,0.7,morning
text=page,0.8,0.94,0,0,1.75,1,0,1,1,0.7,evening
text=page,0.015,0.88,0,0,1.5,1,0,1,1,0.7,Sun
text=page,0.032,0.705,0,0,1.5,1,0,1,1,0.7,Mercury
text=page,0.125,0.62,0,0,1.5,1,0,1,1,0.7,Venus
text=page,0.049,0.84,0,0,1.5,1,0,1,1,0.7,Mars
text=page,0.68,0.3,0,0,1.5,1,0,1,1,0.7,Jupiter
text=page,0.84,0.6,0,0,1.5,1,0,1,1,0.7,Saturn
