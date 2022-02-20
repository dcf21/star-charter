// settings.h
// 
// -------------------------------------------------
// Copyright 2015-2022 Dominic Ford
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
#define SW_PROJECTION_FLAT   1
#define SW_PROJECTION_GNOM   2
#define SW_PROJECTION_SPH    3
#define SW_PROJECTION_ALTAZ  4
#define SW_PROJECTION_PETERS 5

// Options for coordinate systems to use when charting the sky
#define SW_COORDS_RADEC 1
#define SW_COORDS_GAL   2

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
#define SW_LANG_EN 1
#define SW_LANG_FR 2

// Options for designs of constellation stick figures
#define SW_STICKS_SIMPLIFIED 0
#define SW_STICKS_REY 1

//! The maximum number of text labels we can buffer
#define MAX_LABELS 65536

//! The maximum number of exclusion regions we can buffer
#define MAX_EXCLUSION_REGIONS 65536

//! The maximum number of objects we're allowed to draw ephemeris lines for on a single chart
#define N_TRACES_MAX 32

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
    double maximum_angular_size, minimum_phase, brightest_magnitude;
    int point_count;
    ephemeris_point *data;
} ephemeris;

typedef struct chart_config {
    //! Select projection to use. Set to either SW_PROJECTION_FLAT, SW_PROJECTION_GNOM, SW_PROJECTION_SPH,
    //! SW_PROJECTION_ALTAZ or SW_PROJECTION_PETERS.
    int projection;

    //! Select whether to use RA/Dec or galactic coordinates. Set to either SW_COORDS_RADEC or SW_COORDS_GAL.
    int coords;

    //! Boolean indicating whether to write "Right ascension" and "Declination" on the vertical/horizontal axes
    int axis_label;

    //! The language used for the constellation names. Either SW_LANG_EN or SW_LANG_FR
    int language;

    //! The right ascension at the centre of the plot, hours
    double ra0;

    //! The declination at the centre of the plot, degrees
    double dec0;

    //! The position angle of the plot - i.e. the tilt of north, counter-clockwise from up, at the centre of the plot
    double position_angle;

    //! If true, axis labels appear as "5h" or "30 deg". If false, preceded by alpha= or delta=
    int axis_ticks_value_only;

    //! Boolean indicating whether we draw a grid of RA/Dec lines in the background of the star chart
    int ra_dec_lines;

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


    double copyright_gap;


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

    //! Boolean flag which we set false if the user manually sets a desired <mag_min> value.
    int mag_min_automatic;

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

    //! The number of ephemeris lines to draw for solar system objects
    int ephemeride_count;

    //! Boolean indicating whether we auto-scale the star chart to the requested ephemerides
    int ephemeris_autoscale;

    //! Boolean indicating whether to include a table of the object's magnitude
    int ephemeris_table;

    //! Boolean indicating whether we must show all ephemeris text labels, even if they collide with other text
    int must_show_all_ephemeris_labels;

    //! The definitions supplied on the command line for the ephemerides to draw
    char ephemeris_definitions[N_TRACES_MAX][FNAME_LENGTH];

    //! The path to the binary tool `ephemeris_compute`, used to compute paths for solar system objects.
    //! See <https://github.com/dcf21/ephemeris-compute-de430>
    char ephemeris_compute_path[FNAME_LENGTH];

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

    //! Colour to use when drawing ephemerides for solar system objects
    colour ephemeris_col;

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

    // ----------------------------------------
    // Settings which we don't currently expose
    // ----------------------------------------

    //! The font family we should use for text output
    char font_family[64];

    //! The line width to use when tracing great circles
    double great_circle_line_width;

    //! The line width to use when plotting the coordinate grid in the background of the star chart
    double coordinate_grid_line_width;

    //! Scaling factor to apply to the point size used to represent deep sky objects
    double dso_point_size_scaling;

    //! Line width to use for constellation stick figures
    double constellation_sticks_line_width;

    //! Line width to use for the edge of the star chart
    double chart_edge_line_width;

    // ---------------
    // Calculated data
    // ---------------

    //! Ephemeris data for solar system objects
    ephemeris *ephemeris_data;

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

void config_init(chart_config *i);

void config_close(chart_config *i);

#endif
