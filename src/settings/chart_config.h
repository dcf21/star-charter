// settings.h
// 
// -------------------------------------------------
// Copyright 2015-2024 Dominic Ford
//
// This file is part of StarCharter.
//
// StarCharter is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// StarCharter is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with StarCharter.  If not, see <http://www.gnu.org/licenses/>.
// -------------------------------------------------

#ifndef SETTINGS_H
#define SETTINGS_H 1

#include <cairo/cairo.h>

#include "coreUtils/strConstants.h"

// Options for projections to use to represent curved sky on a flat chart
#define SW_PROJECTION_FLAT          1
#define SW_PROJECTION_GNOMONIC      2
#define SW_PROJECTION_SPHERICAL     3
#define SW_PROJECTION_ALTAZ         4
#define SW_PROJECTION_PETERS        5
#define SW_PROJECTION_STEREOGRAPHIC 6

// Options for coordinate systems to use when charting the sky
#define SW_COORDS_RADEC     1
#define SW_COORDS_GALACTIC  2
#define SW_COORDS_ALTAZ     3

// Options for output image formats
#define SW_FORMAT_SVG 1
#define SW_FORMAT_PNG 2
#define SW_FORMAT_EPS 3
#define SW_FORMAT_PDF 4

// Options for catalogue to use when labelling the catalogue numbers of stars
#define SW_CAT_HIP  1
#define SW_CAT_YBSC 2
#define SW_CAT_HD   3

// Language options
#define SW_LANG_ENGLISH 1
#define SW_LANG_FRENCH  2

// Options for designs of constellation stick figures
#define SW_STICKS_SIMPLIFIED 0
#define SW_STICKS_REY 1

// Options for the display of ephemerides for solar system objects
#define SW_EPHEMERIS_TRACK 0
#define SW_EPHEMERIS_SIDE_BY_SIDE 1
#define SW_EPHEMERIS_SIDE_BY_SIDE_WITH_TRACK 2
#define SW_EPHEMERIS_SIDE_BY_SIDE_WITH_ARROW 3

// Options for designs of constellation stick figures
#define SW_DSO_STYLE_COLOURED 0
#define SW_DSO_STYLE_FUZZY 1

//! The maximum number of text labels we can buffer
#define MAX_LABELS 65536

//! The maximum number of exclusion regions we can buffer
#define MAX_EXCLUSION_REGIONS 65536

//! The maximum number of objects we're allowed to draw ephemeris lines for on a single chart
#define N_TRACES_MAX 32

//! Maximum allowed iteration depth when including configuration files
#define MAX_INCLUSION_DEPTH 8

//! Define an RGB colour to use to draw a particular item on a chart
typedef struct colour {
    double red, grn, blu;
} colour;

//! A structure defining a point in an ephemeris
typedef struct ephemeris_point {
    double jd;
    double ra, dec; // radians, J2000
    double mag;
    double phase; // 0-1
    double angular_size; // arcseconds
    char *text_label;
    int sub_month_label;
} ephemeris_point;

//! A structure defining an ephemeris of a solar system object
typedef struct ephemeris {
    double jd_start, jd_end, jd_step; // Julian day numbers
    char obj_id[FNAME_LENGTH]; // DE450 ID for the object
    double maximum_angular_size, minimum_phase, brightest_magnitude;
    int point_count;
    ephemeris_point *data;
} ephemeris;

typedef struct chart_config {
    //! Select projection to use. Set to SW_PROJECTION_STEREOGRAPHIC, SW_PROJECTION_FLAT, SW_PROJECTION_GNOMONIC,
    //! SW_PROJECTION_SPHERICAL, SW_PROJECTION_ALTAZ or SW_PROJECTION_PETERS.
    int projection;

    //! Select whether to specify centre of field of view in RA/Dec, galactic coordinates, or local alt/az.
    //! Set to SW_COORDS_RADEC, SW_COORDS_GALACTIC or SW_COORDS_ALTAZ.
    int coords;

    //! Select whether to show a grid of RA/Dec, galactic coordinates, or local alt/az.
    //! Set to SW_COORDS_RADEC, SW_COORDS_GALACTIC or SW_COORDS_ALTAZ.
    int grid_coords;

    //! Boolean indicating whether to write "Right ascension" and "Declination" on the vertical/horizontal axes
    int axis_label;

    //! The language used for the constellation names. Either SW_LANG_ENGLISH or SW_LANG_FRENCH
    int language;

    //! The right ascension of the centre of the plot, hours
    //! This setting is only used if <coords=SW_COORDS_RADEC>
    double ra0;

    //! The declination of the centre of the plot, degrees
    //! This setting is only used if <coords=SW_COORDS_RADEC>
    double dec0;

    //! The galactic longitude of the centre of the plot, degrees
    //! This setting is only used if <coords=SW_COORDS_GALACTIC>
    double l0;

    //! The galactic latitude of the centre of the plot, degrees
    //! This setting is only used if <coords=SW_COORDS_GALACTIC>
    double b0;

    //! The altitude of the centre of the plot, degrees
    //! This setting is only used if <coords=SW_COORDS_ALTAZ>
    double alt0;

    //! The azimuth of the centre of the plot, degrees
    //! This setting is only used if <coords=SW_COORDS_ALTAZ>
    double az0;

    //! The position angle of the plot - i.e. the tilt of north in degrees, counter-clockwise from up, at the centre
    //! of the plot
    double position_angle;

    //! The list of text labels to overlay over the star chart
    char text_labels[N_TRACES_MAX][FNAME_LENGTH];

    //! The number of default text labels to overlay over the star chart
    int text_labels_default_count;

    //! The number of custom text labels to overlay over the star chart
    int text_labels_custom_count;

    //! Boolean indicating whether we show the local horizon and clip objects below the horizon at `julian_date`
    int show_horizon;

    //! Terrestrial latitude for which to show the local horizon; degrees
    double horizon_latitude;

    //! Terrestrial longitude for which to show the local horizon; degrees
    double horizon_longitude;

    //! Julian date for which to show local horizon, with which to measure alt/az, and for which to show the positions
    //! of solar system bodies.
    double julian_date;

    //! Colour to use when drawing cardinal point markers on the horizon
    colour horizon_cardinal_points_marker_colour;

    //! Colour to use when drawing cardinal point labels on the horizon
    colour horizon_cardinal_points_labels_colour;

    //! Size of markers to use when drawing cardinal points on the horizon
    double horizon_cardinal_points_marker_size;

    //! Number of markers to use when drawing cardinal points on the horizon. Sensible values are 4, 8, 16.
    int horizon_cardinal_points_marker_count;

    //! Boolean flag (0 or 1) indicating whether to elevate cardinal point markers to the bottom of the field of view if they fall off the bottom of the chart.
    int horizon_cardinal_points_marker_elevate;

    //! Boolean indicating whether we show the positions of solar system bodies at `julian_date`
    int show_solar_system;

    //! The list of labels to show next to the selected solar system bodies
    char solar_system_labels[N_TRACES_MAX][FNAME_LENGTH];

    //! The list of the ID strings of the solar system bodies to show (e.g. `P1` for Mercury)
    char solar_system_ids[N_TRACES_MAX][FNAME_LENGTH];

    //! The number of default solar system objects listed within the data structures `solar_system_labels` and
    //! `solar_system_ids`
    int solar_system_default_count;

    //! The number of custom solar system objects listed within the data structures `solar_system_labels`
    int solar_system_labels_custom_count;

    //! The number of custom solar system objects listed within the data structures `solar_system_ids`
    int solar_system_ids_custom_count;

    //! The colours to use when drawing solar system objects. These colours are used in a cyclic loop.
    colour solar_system_colour[N_TRACES_MAX];

    //! The number of colours in the default sequence used for solar system objects.
    int solar_system_colour_default_count;

    //! The number of colours in the custom sequence used for solar system objects.
    int solar_system_colour_custom_count;

    //! Boolean flag (0 or 1) indicating whether to show the Moon's phase (1), or show a simple marker (0).
    int solar_system_show_moon_phase;

    //! The fractional intensity of Earthshine on the Moon's unilluminated portion, compared to the illuminated Moon.
    double solar_system_moon_earthshine_intensity;

    //! The colour to use to represent the illuminated portion of the Moon.
    colour solar_system_moon_colour;

    //! Boolean flag indicating whether we shade the sky according to the altitude in the local sky at `julian_date`.
    int shade_twilight;

    //! Boolean flag indicating whether to shade the region of sky that is close to the Sun at `julian_date`.
    int shade_near_sun;

    //! Boolean flag indicating whether to shade the region of sky that is not observable at any time of day at `julian_date`.
    int shade_not_observable;

    //! The colour to use to shade twilight at the zenith
    colour twilight_zenith_col;

    //! The colour to use to shade twilight at the horizon
    colour twilight_horizon_col;

    //! Boolean flag indicating whether we mark the zenith
    int show_horizon_zenith;

    //! Colour to use when marking the zenith
    colour horizon_zenith_colour;

    //! Scaling factor to apply to the size of the marker used at the zenith. Default 1.
    double horizon_zenith_marker_size;

    //! If true, axis labels appear as "5h" or "30 deg". If false, preceded by alpha= or delta=
    int axis_ticks_value_only;

    //! Boolean indicating whether we draw a grid of RA/Dec lines in the background of the star chart
    int show_grid_lines;

    //! Boolean indicating whether we draw constellation boundaries
    int constellation_boundaries;

    //! Boolean indicating whether we draw constellation stick figures
    int constellation_sticks;

    //! Select which design of constellation stick figures we should use
    //! Either SW_STICKS_SIMPLIFIED or SW_STICKS_REY
    int constellation_stick_design;

    //! Optionally select a constellation to highlight
    char constellation_highlight[8];

    //! Boolean indicating whether we label the English names of stars
    int star_names;

    //! Boolean indicating whether we label the catalogue numbers of stars
    int star_catalogue_numbers;

    //! Boolean indicating whether we label the Bayer numbers of stars
    int star_bayer_labels;

    //! Boolean indicating whether we label the Flamsteed designations of stars
    int star_flamsteed_labels;

    //! Boolean indicating whether we label the variable-star designations of stars
    int star_variable_labels;

    //! Boolean indicating whether we allow multiple labels next to a single star
    int star_allow_multiple_labels;

    //! Select the star catalogue to use when showing the catalogue numbers of stars
    //! Either SW_CAT_HIP, SW_CAT_YBSC or SW_CAT_HD.
    int star_catalogue;

    //! Boolean indicating whether we label the magnitudes of stars
    int star_mag_labels;

    //! Select which style to use for deep sky objects
    //! Set to either SW_DSO_STYLE_COLOURED or SW_DSO_STYLE_FUZZY
    int dso_style;

    //! Boolean indicating whether we label the names of NGC objects
    int dso_names;

    //! Boolean indicating whether we label the magnitudes of NGC objects
    int dso_mags;

    //! Boolean indicating whether we label the names of constellations
    int constellation_names;

    //! Boolean indicating whether we plot any stars
    int plot_stars;

    //! Boolean indicating whether we plot only Messier objects and no other deep sky objects
    int messier_only;

    //! Boolean indicating whether we plot any deep sky objects
    int plot_dso;

    //! Boolean indicating whether we plot only the zodiacal constellations
    int zodiacal_only;

    //! A slant to apply to all labels on the horizontal axes
    double x_label_slant;

    //! A slant to apply to all labels on the vertical axes
    double y_label_slant;

    //! Vertical spacing of the copyright text at the bottom of the chart
    double copyright_gap;

    //! Vertical spacing of the copyright text at the bottom of the chart
    double copyright_gap_2;

    //! A normalisation factor to apply to the font size of all text
    double font_size;

    //! The angular width of the star chart on the sky, degrees
    double angular_width;

    //! The width of the star chart, in cm
    double width;

    //! The aspect ratio of the star chart: i.e. the ratio height/width
    double aspect;

    //! The maximum number of stars to draw. If this is exceeded, only the brightest stars are shown.
    int maximum_star_count;

    //! The minimum number of stars to draw. If there are fewer stars, then tweak magnitude limits to fainter stars.
    int minimum_star_count;

    //! The maximum number of stars which may be labelled
    int maximum_star_label_count;

    //! The maximum number of DSOs to draw. If this is exceeded, only the brightest objects are shown.
    int maximum_dso_count;

    //! The maximum number of DSOs which may be labelled
    int maximum_dso_label_count;

    //! The faintest magnitude of star which we draw
    double mag_min;

    //! The magnitude interval between the samples shown on the magnitude key under the chart
    double mag_step;

    //! Used to regulate the size of stars. A star of this magnitude is drawn with size mag_size_norm
    double mag_max;

    //! Computed quantity: the magnitude of the brightest star in the field
    double mag_highest;

    //! The multiplicative scaling factor to apply to the radii of stars differing in magnitude by one <mag_step>
    double mag_alpha;

    //! The radius of a star of magnitude <mag_max>
    double mag_size_norm;

    //! Only show deep sky objects down to this faintest magnitude
    double dso_mag_min;

    //! Do not label stars fainter than this magnitude limit
    double star_label_mag_min;

    //! Do not label DSOs fainter than this magnitude limit
    double dso_label_mag_min;

    //! Computed quantity: the number of rows of star magnitudes we can fit under the chart
    int magnitude_key_rows;

    //! The number of default ephemeris lines to draw for solar system objects
    int ephemeris_default_count;

    //! The number of custom ephemeris lines to draw for solar system objects
    int ephemeris_custom_count;

    //! Boolean indicating whether we auto-scale the star chart to the requested ephemerides
    int ephemeris_autoscale;

    //! Enum indicating how ephemeris tracks should be drawn.
    //! Allowed settings are SW_EPHEMERIS_TRACK, SW_EPHEMERIS_SIDE_BY_SIDE, SW_EPHEMERIS_SIDE_BY_SIDE_WITH_TRACK
    int ephemeris_style;

    //! Boolean indicating whether to draw a shadow around ephemeris arrows
    int ephemeris_show_arrow_shadow;

    //! Boolean indicating whether to include a table of the object's magnitude
    int ephemeris_table;

    //! Boolean indicating whether we must show all ephemeris text labels, even if they collide with other text
    int must_show_all_ephemeris_labels;

    //! The definitions of ephemerides to draw. Each definition should take the form of:
    //! `<bodyId>,<jdMin>,<jdMax>` where bodyId is an object ID string, and jdMin and jdMax are Julian dates
    //! If multiple ephemerides are to be drawn on a single chart, this argument should be specified multiple
    //! times, once for each ephemeris that is to be drawn.
    char ephemeris_definitions[N_TRACES_MAX][FNAME_LENGTH];

    //! List of JD time epochs for which we should create points along each solar system ephemeris. If empty, then
    //! points are created automatically. This list must have the same length as <ephemeris_epoch_labels>.
    char ephemeris_epochs[N_TRACES_MAX][FNAME_LENGTH];

    //! Length of default values in list <ephemeris_epochs>
    int ephemeris_epochs_default_count;

    //! Length of custom values in list <ephemeris_epochs>
    int ephemeris_epochs_custom_count;

    //! List of text labels for the points we create along each solar system ephemeris. If empty, then points are
    //! created automatically. This list must have the same length as <ephemeris_epochs>.
    char ephemeris_epoch_labels[N_TRACES_MAX][FNAME_LENGTH];

    //! Length of custom values in list <ephemeris_epoch_labels>
    int ephemeris_epoch_labels_custom_count;

    //! List of scale bars we should super-impose over the star chart. Each should be specified as:
    //! <x_pos>,<y_pos>,<degrees>
    //! Where <x_pos> and <y_pos> are 0-1, and degrees is the length of the scale bar.
    char scale_bars[N_TRACES_MAX][FNAME_LENGTH];

    //! Length of default values in list <scale_bars>
    int scale_bars_default_count;

    //! Length of custom values in list <scale_bars>
    int scale_bars_custom_count;

    //! Colour to use for scale bars
    colour scale_bar_colour;

    //! The target filename for the star chart. The file type (svg, png, eps or pdf) is inferred from the file extension.
    char output_filename[FNAME_LENGTH];

    //! The copyright string to write under the star chart
    char copyright[FNAME_LENGTH];

    //! The heading to write at the top of the star chart
    char title[FNAME_LENGTH];

    //! Colour to use when drawing constellation stick figures
    colour constellation_stick_col;

    //! Colour to use when drawing grid of RA/Dec lines
    colour grid_col;

    //! Colour to use when drawing constellation boundaries
    colour constellation_boundary_col;

    //! Colours to use when drawing ephemerides for solar system objects. These colours are used in a cyclic loop.
    colour ephemeris_col[N_TRACES_MAX];

    //! The number of colours in the default sequence used for drawing ephemerides for solar system objects.
    int ephemeris_col_default_count;

    //! The number of colours in the custom sequence used for drawing ephemerides for solar system objects.
    int ephemeris_col_custom_count;

    //! Colours to use when drawing ephemeris arrows for solar system objects. These colours are used in a cyclic loop.
    colour ephemeris_arrow_col[N_TRACES_MAX];

    //! The number of colours in the default sequence used for drawing ephemeris arrows for solar system objects.
    int ephemeris_arrow_col_default_count;

    //! The number of colours in the custom sequence used for drawing ephemeris arrows for solar system objects.
    int ephemeris_arrow_col_custom_count;

    //! Colours to use when labelling ephemerides for solar system objects. These colours are used in a cyclic loop.
    colour ephemeris_label_col[N_TRACES_MAX];

    //! The number of colours in the default sequence used for labelling ephemerides for solar system objects.
    int ephemeris_label_col_default_count;

    //! The number of colours in the custom sequence used for labelling ephemerides for solar system objects.
    int ephemeris_label_col_custom_count;

    //! Colour to use when writing constellation names
    colour constellation_label_col;

    //! Colour to use when drawing star clusters
    colour dso_cluster_col;

    //! Colour to use when drawing galaxies
    colour dso_galaxy_col;

    //! Colour to use when drawing nebulae
    colour dso_nebula_col;

    //! Colour to use when writing the labels for deep sky objects
    colour dso_label_col;

    //! Colour to use when drawing the outlines of deep sky objects
    colour dso_outline_col;

    //! Colour to use when drawing stars
    colour star_col;

    //! Colour to use when labelling stars
    colour star_label_col;

    //! Colour to use when drawing a line along the ecliptic
    colour ecliptic_col;

    //! Colour to use when drawing a line along the galactic plane
    colour galactic_plane_col;

    //! Colour to use when drawing a line along the equator
    colour equator_col;

    //! Boolean indicating whether to draw a line along the ecliptic
    int plot_ecliptic;

    //! Boolean indicating whether to draw a line along the galactic plane
    int plot_galactic_plane;

    //! Boolean indicating whether to draw a line along the equator
    int plot_equator;

    //! Boolean indicating whether to label the months along the ecliptic, showing the Sun's annual progress
    int label_ecliptic;

    //! Boolean indicating whether to draw a shaded map of the Milky Way behind the star chart
    int plot_galaxy_map;

    //! The number of horizontal pixels across the shaded map of the Milky Way
    int galaxy_map_width_pixels;

    //! The binary file from which to read the shaded map of the Milky Way
    char galaxy_map_filename[FNAME_LENGTH];

    //! The colour to use to shade the bright parts of the map of the Milky Way
    colour galaxy_col;

    //! The colour to use to shade the dark parts of the map of the Milky Way
    colour galaxy_col0;

    //! The filename of a PNG image to render behind the star chart. Leave blank to show no image.
    char photo_filename[FNAME_LENGTH];

    //! Boolean indicating whether to draw a key to the magnitudes of stars under the star chart
    int magnitude_key;

    //! Boolean indicating whether to draw a key to the great circles under the star chart
    int great_circle_key;

    //! Boolean indicating whether to draw a key to the deep sky symbols under the star chart
    int dso_symbol_key;

    //! Boolean indicating whether to write the cardinal points around the edge of alt/az star charts
    int cardinals;

    //! Scaling factor to be applied to the font size of all star and DSO labels
    double label_font_size_scaling;

    //! List of meteor radiants to show and label
    char meteor_radiants[N_TRACES_MAX][FNAME_LENGTH];

    //! Colour to use when drawing meteor shower radiants
    colour meteor_radiant_colour;

    //! Scaling of the size of the marker used to label meteor shower radiants
    double meteor_radiant_marker_size;

    //! Number of default meteor radiants
    int meteor_radiants_default_count;

    //! Number of custom meteor radiants
    int meteor_radiants_custom_count;

    //! The font family we should use for text output
    char font_family[64];

    //! The line width to use when tracing great circles
    double great_circle_line_width;

    //! Boolean indicating whether to use a dotted line when tracing great circles
    double great_circle_dotted;

    //! The line width to use when plotting the coordinate grid in the background of the star chart
    double coordinate_grid_line_width;

    //! Scaling factor to apply to the point size used to represent deep sky objects
    double dso_point_size_scaling;

    //! Boolean indicating whether to capitalise the names of constellations
    int constellations_capitalise;

    //! Boolean indicating whether to apply shadowing to the names of constellations
    int constellations_label_shadow;

    //! Line width to use for constellation stick figures
    double constellation_sticks_line_width;

    //! Line width to use for the edge of the star chart
    double chart_edge_line_width;

    //! Boolean flags indicating which settings have been manually overridden
    //! (so that automatic scaling does not overwrite them).
    int mag_min_is_set;
    int dso_mag_min_is_set;
    int minimum_star_count_is_set;
    int ra0_is_set;
    int dec0_is_set;
    int star_flamsteed_labels_is_set;
    int star_bayer_labels_is_set;
    int constellation_names_is_set;
    int star_catalogue_is_set;
    int star_catalogue_numbers_is_set;
    int maximum_star_label_count_is_set;
    int projection_is_set;
    int angular_width_is_set;
    int dso_names_is_set;
    int aspect_is_set;

    // ---------------
    // Calculated data
    // ---------------

    //! The final, computed, right ascension of the centre of the plot, radians
    //! (regardless of which coordinate system is in use)
    double ra0_final;

    //! The final, computed, declination of the centre of the plot, radians
    //! (regardless of which coordinate system is in use)
    double dec0_final;

    //! The final number of text labels to overlay over the star chart
    int text_labels_final_count;

    //! The final number of solar system objects listed within the data structures `solar_system_labels` and
    //! `solar_system_ids`
    int solar_system_final_count;

    //! The final number of colours in the sequence used for solar system objects.
    int solar_system_colour_final_count;

    //! The final number of colours in the sequence used for drawing ephemerides for solar system objects.
    int ephemeris_col_final_count;

    //! The final number of colours in the sequence used for drawing ephemeris arrows for solar system objects.
    int ephemeris_arrow_col_final_count;

    //! The final number of colours in the sequence used for labelling ephemerides for solar system objects.
    int ephemeris_label_col_final_count;

    //! The final number of custom ephemeris lines to draw for solar system objects
    int ephemeris_final_count;

    //! The final number of values in list <ephemeris_epochs>
    int ephemeris_epochs_final_count;

    //! The final number of values in list <scale_bars>
    int scale_bars_final_count;

    //! The final number of meteor radiants to show
    int meteor_radiants_final_count;

    //! Ephemeris data for the tracks of solar system objects we are to plot
    ephemeris *ephemeris_data;

    //! The definitions of ephemerides for the solar system bodies whose positions we will show.
    //! Each definition should take the form of: `<bodyId>,<jdMin>,<jdMax>` where bodyId is an object ID string,
    //! and jdMin and jdMax are Julian dates.
    char solar_system_ephemeris_definitions[N_TRACES_MAX][FNAME_LENGTH];

    //! Ephemeris data for solar system objects
    ephemeris *solar_system_ephemeris_data;

    //! Image format to use for the output. One of SW_FORMAT_SVG, SW_FORMAT_PNG, SW_FORMAT_EPS or SW_FORMAT_PDF
    int output_format;

    double canvas_width, canvas_height, canvas_offset_x, canvas_offset_y, dpi, pt, cm, mm, line_width_base;
    double wlin, x_min, x_max, y_min, y_max;

    //! Width of the right-hand column of the legend under the finder chart
    double legend_right_column_width;

    //! Cairo surface for drawing star chart onto
    cairo_surface_t *cairo_surface;

    //! Cairo drawing context
    cairo_t *cairo_draw;

} chart_config;

void default_config(chart_config *i);

void config_init_arrays(chart_config *i);

void config_init_pointing(chart_config *i);

void config_close(chart_config *i);

#endif
