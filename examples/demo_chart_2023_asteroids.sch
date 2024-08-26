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
width=22.0
show_horizon=0
constellation_sticks=1
star_names=1
constellation_names=1
shade_twilight=0
shade_not_observable=0
plot_equator=0
plot_ecliptic=0
plot_dso=1
dso_names=1
plot_galaxy_map=1
galaxy_col=0.2,0.2,0.5
galaxy_col0=0.2,0.2,0.5
show_solar_system=0
constellation_boundaries=1
star_bayer_labels=1
star_flamsteed_labels=1
ephemeris_style=side_by_side_with_arrow
ephemeris_autoscale=1
ephemeris_col=0.9,0.9,1
ephemeris_arrow_col=0.9,0.9,1
solar_system_col=0.9,0.9,1

# Chart for 4 Vesta
CHART
copyright=The path of 4 Vesta
text=0.52,0.36,0,0,1.5,0.9,0.9,1,VESTA
draw_ephemeris=A4,2460249.5,2460364.5  # Vesta
# ra_central=6
# dec_central=20
# angular_width=22
aspect=0.5
mag_min=7.5
mag_size_norm=0.7
star_flamsteed_labels=1
output_filename=output/demo_chart_2023_vesta.png

ephemeris_epochs=2460249.5  # 2023 Nov 1 00:00:00
ephemeris_epoch_labels=1 Nov
ephemeris_epochs=2460279.5  # 2023 Dec 1 00:00:00
ephemeris_epoch_labels=1 Dec
ephemeris_epochs=2460310.5  # 2024 Jan 1 00:00:00
ephemeris_epoch_labels=1 Jan
ephemeris_epochs=2460354.5  # 2024 Feb 14 00:00:00
ephemeris_epoch_labels=Feb 14

# Chart for 1 Ceres
CHART
copyright=The path of 1 Ceres
text=0.52,0.36,0,0,1.5,0.9,0.9,1,CERES
draw_ephemeris=A1,2459989.5,2460074.5 # Ceres
# ra_central=12.4
# dec_central=13
# angular_width=19
aspect=0.5
mag_min=7.5
mag_size_norm=0.55
star_flamsteed_labels=1
output_filename=output/demo_chart_2023_ceres.png

ephemeris_epochs=2459989.5  # 2023 Feb 14 00:00:00
ephemeris_epoch_labels=14 Feb
ephemeris_epochs=2460004.5  # 2023 Mar 1 00:00:00
ephemeris_epoch_labels=1 Mar
ephemeris_epochs=2460035.5  # 2023 Apr 1 00:00:00
ephemeris_epoch_labels=1 Apr
ephemeris_epochs=2460064.5  # 2023 Apr 30 00:00:00
ephemeris_epoch_labels=30 Apr
