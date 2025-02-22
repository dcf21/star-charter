# Demonstration configuration file to show the path of the Moon over three
# days, with and without topocentric correction
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

# Default settings which affect all the charts below
DEFAULTS
INCLUDE include_files/colour_scheme_pastel.sch
width=16.0
aspect=1
mag_min=5
plot_dso=0
plot_ecliptic=0
plot_equator=0
draw_ephemeris=p301,2460557.9479,2460560.9479
ephemeris_style=side_by_side
must_show_all_ephemeris_labels=1
solar_system_ids=P2
solar_system_labels=Venus
ephemeris_resolution=0.02
horizon_latitude=25.0375  # Taipei
horizon_longitude=121.5625
shade_twilight=1
julian_date=2460558.9479  # 18:45 CST, 5 Sept
show_horizon=1
show_solar_system=1
horizon_cardinal_points_marker_count=16
coords=alt_az
grid_coords=alt_az
alt_central=13
az_central=260
angular_width=32
scale_bar=0.05,0.5,0,5
scale_bar_col=1,1,1

# Produce track of the Moon without topocentric correction
CHART
solar_system_topocentric_correction=0
output_filename=output/moon_track_00.png

# Produce track of the Moon with topocentric correction
CHART
solar_system_topocentric_correction=1
output_filename=output/moon_track_01.png

