# Demonstration of creating quadrant views of the night sky with interesting
# DSOs highlighted (i.e. those brighter than a threshold magnitude, and not open
# clusters).

DEFAULTS
coords=alt_az
horizon_latitude=56.30
horizon_longitude=-3.71
# date -d 'tomorrow 00:00' +%s | awk '{printf "%.1f\n", $1/86400 + 2440587.5}'
julian_date=2460942.5 # Wed 24 Sep 00:00:00 BST 2025

# A4 page
angular_width=110
width=29.7
aspect=0.707

# show the horizon but no other curves
show_horizon=1
cardinals=1
grid_coords=ra_dec
show_grid_lines=1
plot_galactic_plane=0
plot_ecliptic=0
plot_equator=0
plot_galaxy_map=0

# stars, show them but don't label them
mag_min=9.1
star_label_mag_min=0

# don't show solar system objects, so we can reuse every year
show_solar_system=0

# show all DSOs except open clusters
# (uses colours that are visible with a red light)
#
# Unfortunately this also shows things that turned out to
# not be DSOs, e.g. NGC 7748, so those need a DB update.
#
# Note that _mag_min values are exclusive.
plot_dso=1
#dso_display_style=fuzzy
dso_mag_min=12.1
dso_label_mag_min=11.1
dso_cluster_col=0.5,0.5,0.5
dso_galaxy_col=0,0,0
dso_nebula_col=0.8,0.8,0.8
dso_names_openclusters=0
# if we don't force render, then almost nothing gets shown
must_label_all_dsos=1

# minimise constellation impact
constellation_boundaries=0
constellation_names=0
constellation_stick_design=simplified
constellation_stick_col=0.5,0.5,0.5
constellation_sticks_line_width=0.7

# turn off all legends
copyright=
magnitude_key=0
dso_symbol_key=0

# draw the zenith
text=alt_az,90,0,0,0,1,1,0,0,0,1,+

# turn off arbitrary limits, so we use magnitudes exclusively
maximum_star_count=10000
maximum_star_label_count=10000
maximum_dso_count=10000
maximum_dso_label_count=10000

CHART
az_central=0
alt_central=60
output_filename=output/dso_planner_N.pdf

CHART
az_central=90
alt_central=60
output_filename=output/dso_planner_E.pdf

CHART
az_central=180
alt_central=60
output_filename=output/dso_planner_S.pdf

CHART
az_central=270
alt_central=60
output_filename=output/dso_planner_W.pdf
