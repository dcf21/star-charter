# Demonstration configuration file which produces charts of the paths
# of two minor planets (4 Vesta and 1 Ceres) in 2023
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
aspect=0.5
mag_min=7.5
show_horizon=0
constellation_sticks=1
star_names=1
constellation_names=1
shade_twilight=0
shade_not_observable=0
plot_equator=0
plot_ecliptic=0
plot_galactic_plane=0
dso_symbol_key=0
great_circle_key=0
plot_dso=1
dso_names=1
plot_galaxy_map=1
show_solar_system=0
constellation_boundaries=1
star_bayer_labels=1
star_flamsteed_labels=1
ephemeris_style=side_by_side_with_arrow
ephemeris_autoscale=1
ephemeris_col=1,1,0.7
ephemeris_arrow_col=1,1,0.7
solar_system_col=1,1,0.7
must_show_all_ephemeris_labels=1

# Chart for 4 Vesta
CHART
title=The path of 4 Vesta
text=page,0.52,0.45,0,0,1.5,1,0,1,1,0.7,VESTA
draw_ephemeris=A4,2460249.5,2460364.5  # Vesta
mag_size_norm=0.7
output_filename=output/demo_chart_2023_vesta.png

ephemeris_epochs=2460249.5  # 2023 Nov 1 00:00:00
ephemeris_epoch_labels=1 Nov
ephemeris_epochs=2460279.5  # 2023 Dec 1 00:00:00
ephemeris_epoch_labels=1 Dec
ephemeris_epochs=2460310.5  # 2024 Jan 1 00:00:00
ephemeris_epoch_labels=1 Jan
ephemeris_epochs=2460354.5  # 2024 Feb 14 00:00:00
ephemeris_epoch_labels=14 Feb

# Chart for 1 Ceres
CHART
title=The path of 1 Ceres
text=page,0.52,0.36,0,0,1.5,1,0,1,1,0.7,CERES
draw_ephemeris=A1,2459989.5,2460074.5 # Ceres
mag_size_norm=0.55
output_filename=output/demo_chart_2023_ceres.png

ephemeris_epochs=2459989.5  # 2023 Feb 14 00:00:00
ephemeris_epoch_labels=14 Feb
ephemeris_epochs=2460004.5  # 2023 Mar 1 00:00:00
ephemeris_epoch_labels=1 Mar
ephemeris_epochs=2460035.5  # 2023 Apr 1 00:00:00
ephemeris_epoch_labels=1 Apr
ephemeris_epochs=2460064.5  # 2023 Apr 30 00:00:00
ephemeris_epoch_labels=30 Apr
