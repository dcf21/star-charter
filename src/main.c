// main.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#include <gsl/gsl_const_mksa.h>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_math.h>

#include "coreUtils/asciiDouble.h"
#include "coreUtils/strConstants.h"
#include "coreUtils/errorReport.h"

#include "listTools/ltMemory.h"

#include "settings/chart_config.h"

#include "astroGraphics/constellations.h"
#include "astroGraphics/ephemeris.h"
#include "astroGraphics/galaxyMap.h"
#include "astroGraphics/greatCircles.h"
#include "astroGraphics/deepSky.h"
#include "astroGraphics/deepSkyOutlines.h"
#include "astroGraphics/raDecLines.h"
#include "astroGraphics/stars.h"
#include "vectorGraphics/lineDraw.h"
#include "vectorGraphics/cairo_page.h"

//! render_chart - Main entry point to render a single star chart
//! \param s - The configuration for the star chart to be rendered

void render_chart(chart_config *s) {
    int i;
    cairo_page page;
    line_drawer ld;
    char line[FNAME_LENGTH];

    // If we're plotting ephemerides for solar system objects, fetch the data now
    // We do this first, as auto-scaling plots use this data to determine which sky area to show
    ephemerides_fetch(s);

    // Check star chart configuration, and insert any computed quantities
    config_init(s);

    // Create a cairo surface object to render the star chart onto
    cairo_init(&page, s);

    // If we're shading the Milky Way behind the star chart, do that first
    if (s->plot_galaxy_map) {
        if (DEBUG) {
            snprintf(line, FNAME_LENGTH, "Starting work on galaxy map image.");
            stch_log(line);
        }
        plot_galaxy_map(s);
    }

    // If we're showing a PNG image behind the star chart, insert that next
    plot_background_image(s);

    // Initialise module for tracing lines on the star chart
    ld_init(&ld, s, page.x_labels, page.x2_labels, page.y_labels, page.y2_labels);

    // Draw the line of the equator
    if (s->plot_equator) plot_equator(s, &ld, &page);

    // Draw the line of the galactic plane
    if (s->plot_galactic_plane) plot_galactic_plane(s, &ld, &page);

    // Draw the line of the ecliptic
    if (s->plot_ecliptic) plot_ecliptic(s, &ld, &page);

    // Draw a grid of lines of constant RA and Dec
    if (s->ra_dec_lines) plot_ra_dec_lines(s, &ld);

    // Draw constellation boundaries
    if (s->constellation_boundaries) plot_constellation_boundaries(s, &ld);

    // Draw stick figures to represent the constellations
    if (s->constellation_sticks) plot_constellation_sticks(s, &ld);

    // Draw deep sky object outlines
    if (s->plot_dso) {
        plot_deep_sky_outlines(s, &page);
    }

    // Draw deep sky objects
    if (s->plot_dso) {
        plot_deep_sky_objects(s, &page, s->messier_only);
    }

    // Draw stars
    if (s->plot_stars) plot_stars(s, &page);

    // Write the names of the constellations
    if (s->constellation_names) plot_constellation_names(s, &page);

    // If we're plotting ephemerides for solar system objects, draw these now
    for (i = 0; i < s->ephemeride_count; i++) plot_ephemeris(s, &ld, &page, i);

    // Render labels onto the chart while the clipping region is still in force
    chart_label_unbuffer(&page);

    // Draw axes around the edge of the star chart
    draw_chart_edging(&page, s);

    // Vertical position of top of legends at the bottom of the star chart
    const double legend_y_pos_baseline = s->canvas_offset_y + s->width * s->aspect + 0.7 + (s->ra_dec_lines ? 0.5 : 0);
    double legend_y_pos_left = legend_y_pos_baseline + 0.8 + s->copyright_gap;
    double legend_y_pos_right = legend_y_pos_baseline - 0.2;

    // If we're to show a key below the chart indicating the magnitudes of stars, draw this now
    if (s->magnitude_key) legend_y_pos_left = draw_magnitude_key(s, legend_y_pos_left);

    // If we're to show a key below the chart indicating the colours of the lines, draw this now
    if (s->great_circle_key) legend_y_pos_left = draw_great_circle_key(s, legend_y_pos_left);

    // If we're to show a key below the chart indicating the deep sky object symbols, draw this now
    if (s->dso_symbol_key) legend_y_pos_left = draw_dso_symbol_key(s, legend_y_pos_left);

    // If we're showing a table of the object's magnitude, draw that now
    if (s->ephemeris_table) legend_y_pos_right = draw_ephemeris_table(s, legend_y_pos_right, 1, NULL);

    // Finish up and write output
    if (DEBUG) {
        stch_log("Finished rendering chart");
    }
    if (chart_finish(&page, s)) { stch_fatal(__FILE__, __LINE__, "cairo close fail."); }

    // Free up storage
    ephemerides_free(s);
    config_close(s);
}

// Macro to check that a parameter in a configuration file has a numeric value
#define CHECK_KEYVALNUM(NAME) \
    if (!key_val_num_valid) { \
        snprintf(temp_err_string, FNAME_LENGTH, \
                "Bad input file. Setting '%s' should be a numeric value on line %d.", \
                NAME, file_line_number); \
        stch_error(temp_err_string); \
        return 1; \
    }

//! colour_from_string - Convert a string representation of a colour in the form r,g,b into a colour structure.
//! \param input - String representation of a colour, in the form r,g,b, with each component in range 0-1.
//! \return - A colour structure

colour colour_from_string(const char *input) {
    colour output;
    const char *in_scan = input;
    char buffer[FNAME_LENGTH];
    str_comma_separated_list_scan(&in_scan, buffer);
    output.red = get_float(buffer, NULL);
    str_comma_separated_list_scan(&in_scan, buffer);
    output.grn = get_float(buffer, NULL);
    str_comma_separated_list_scan(&in_scan, buffer);
    output.blu = get_float(buffer, NULL);

    return output;
}

//! Main entry point for rendering a single star chart, or a sequence of star charts, as described in a configuration
//! file. On the command line, the user should either supply a single filename for a configuration file to read, or
//! else the configuration is expected to be supplied on stdin.
//! \param argc - Command line arguments
//! \param argv - Command line arguments
//! \return - Exit status

int main(int argc, char **argv) {
    char help_string[LSTR_LENGTH], version_string[FNAME_LENGTH], version_string_underline[FNAME_LENGTH];
    char line[LSTR_LENGTH], key[LSTR_LENGTH], key_val[LSTR_LENGTH];
    char *filename = NULL;
    int i, have_filename = 0, file_line_number;
    FILE *infile;
    chart_config this_chart_config, chart_defaults, *settings_destination = NULL;
    unsigned char got_chart = 0, key_val_num_valid;
    double key_val_num;

    // Initialise sub-modules
    if (DEBUG) stch_log("Initialising StarCharter");
    lt_memoryInit(&stch_error, &stch_log);

    // Turn off GSL's automatic error handler
    gsl_set_error_handler_off();

    // Make help and version strings
    snprintf(version_string, FNAME_LENGTH, "StarCharter %s", DCFVERSION);

    snprintf(help_string, FNAME_LENGTH, "StarCharter %s\n"
                                        "%s\n\n"
                                        "Usage: starchart.bin <filename>\n"
                                        "-h, --help:       Display this help.\n"
                                        "-v, --version:    Display version number.",
             DCFVERSION, str_underline(version_string, version_string_underline));

    // Scan command line options for any switches
    have_filename = 0;
    for (i = 1; i < argc; i++) {
        // Ignore empty arguments
        if (strlen(argv[i]) == 0) continue;

        if (argv[i][0] != '-') {
            // If the switch doesn't start with a -, then treat it as a filename of a configuration file
            have_filename++;
            filename = argv[i];
            continue;
        }

        if ((strcmp(argv[i], "-v") == 0) || (strcmp(argv[i], "-version") == 0) || (strcmp(argv[i], "--version") == 0)) {
            // Switches -v and --version cause the version number to be printed
            stch_report(version_string);
            return 0;
        } else if ((strcmp(argv[i], "-h") == 0) || (strcmp(argv[i], "-help") == 0) ||
                   (strcmp(argv[i], "--help") == 0)) {
            // Switches -h and --help cause the usage string to be displayed
            stch_report(help_string);
            return 0;
        } else {
            // Return an error if an unknown switch is received
            snprintf(temp_err_string, FNAME_LENGTH,
                     "Received switch '%s' which was not recognised.\nType 'starchart.bin -help' for a list of "
                     "available commandline options.",
                     argv[i]);
            stch_error(temp_err_string);
            return 1;
        }
    }

    // Check that we have been provided with no more than one filename on the command line
    if (have_filename > 1) {
        snprintf(temp_err_string, FNAME_LENGTH,
                 "starchart.bin should be provided with only one filename on the command line to act upon. "
                 "Multiple filenames appear to have been supplied. Type 'starchart.bin -help' for a list of "
                 "available commandline options.");
        stch_error(temp_err_string);
        return 1;
    }

    // Set up default settings for star charts
    if (DEBUG) stch_log("Setting up default star chart parameters.");
    default_config(&chart_defaults);

    // Open the input configuration file
    if (have_filename) {
        if ((infile = fopen(filename, "r")) == NULL) {
            snprintf(temp_err_string, FNAME_LENGTH, "StarCharter could not open input file '%s'.", filename);
            stch_error(temp_err_string);
            return 1;
        }
    } else {
        // If no filename was supplied on the command line, read configuration from stdin
        infile = stdin;
    }

    // Go through command script line by line
    file_line_number = 0;
    while (!feof(infile)) {
        file_readline(infile, line);
        file_line_number++;
        str_strip(line, line);

        // Ignore blank lines
        if (strlen(line) == 0) continue;

        // Ignore comment lines
        if (line[0] == '#') continue;

        if (strcmp(line, "DEFAULTS") == 0) {
            // The heading "DEFAULTS" means that we're receiving settings which should apply to all star charts which
            // follow

            // If this follows a CHART definition, then we have all the settings for that chart, and should render
            // it now
            if (got_chart) render_chart(&this_chart_config);
            got_chart = 0;

            // Feed subsequent settings into the default chart configuration
            settings_destination = &chart_defaults;
            continue;
        } else if (strcmp(line, "CHART") == 0) {
            // The heading "CHART" means that we're receiving settings which should apply to a new star chart

            // If this follows a previous CHART definition, then we have all the settings for that chart, and should
            // render it now
            if (got_chart) render_chart(&this_chart_config);

            // Feed subsequent settings into the this_chart_config
            got_chart = 1;
            settings_destination = &this_chart_config;
            this_chart_config = chart_defaults;
            continue;
        }

        // If line was not either DEFAULTS or CHART, it is a configuration setting.
        // If <settings_destination> is NULL, that means we haven't seen a heading DEFAULTS or CHART to tell us
        // where to put this configuration setting.
        if (settings_destination == NULL) {
            snprintf(temp_err_string, FNAME_LENGTH, "Bad input file. Must begin with either 'DEFAULTS', or 'CHART'.");
            stch_error(temp_err_string);
            return 1;
        }

        // Parse the configuration setting, which should be of the form <key = value>
        readConfig_fetchKey(line, key);
        readConfig_fetchValue(line, key_val);

        // Test whether the setting value is a valid floating point string
        key_val_num_valid = valid_float(key_val, NULL);
        key_val_num = get_float(key_val, NULL);

        if (strcmp(key, "ra_central") == 0) {
            //! ra_central - The right ascension at the centre of the plot, hours
            CHECK_KEYVALNUM("ra_central")
            settings_destination->ra0 = fmod(key_val_num, 24);
            while (settings_destination->ra0 < 0) settings_destination->ra0 += 24;
            while (settings_destination->ra0 >= 24) settings_destination->ra0 -= 24;
            continue;
        } else if (strcmp(key, "dec_central") == 0) {
            //! dec_central - The declination at the centre of the plot, degrees
            CHECK_KEYVALNUM("dec_central")
            settings_destination->dec0 = key_val_num;
            if (settings_destination->dec0 > 90) settings_destination->dec0 = 90;
            if (settings_destination->dec0 < -90) settings_destination->dec0 = -90;
            continue;
        } else if (strcmp(key, "axis_ticks_value_only") == 0) {
            //! axis_ticks_value_only - If 1, axis labels will appear as simply "5h" or "30 deg". If 0, these labels
            //! will be preceded by alpha= or delta=
            CHECK_KEYVALNUM("axis_ticks_value_only")
            settings_destination->axis_ticks_value_only = (int) key_val_num;
            continue;
        } else if (strcmp(key, "axis_label") == 0) {
            //! axis_label - Boolean (0 or 1) indicating whether to write "Right ascension" and "Declination" on the
            //! vertical/horizontal axes
            CHECK_KEYVALNUM("axis_label")
            settings_destination->axis_label = (int) key_val_num;
            continue;
        } else if (strcmp(key, "position_angle") == 0) {
            //! position_angle - The position angle of the plot - i.e. the tilt of north, counter-clockwise from up, at
            //! the centre of the plot
            CHECK_KEYVALNUM("position_angle")
            settings_destination->position_angle = key_val_num;
            continue;
        } else if (strcmp(key, "output_filename") == 0) {
            //! output_filename - The target filename for the star chart. The file type (svg, png, eps or pdf) is
            //! inferred from the file extension.
            strcpy(settings_destination->output_filename, key_val);
            continue;
        } else if (strcmp(key, "galaxy_map_filename") == 0) {
            //! galaxy_map_filename - The binary file from which to read the shaded map of the Milky Way
            strcpy(settings_destination->galaxy_map_filename, key_val);
            continue;
        } else if (strcmp(key, "photo_filename") == 0) {
            //! photo_filename - The filename of a PNG image to render behind the star chart. Leave blank to show no
            //! image.
            strcpy(settings_destination->photo_filename, key_val);
            settings_destination->star_col = (colour) {0.75, 0.75, 0.25};
            settings_destination->grid_col = (colour) {0.5, 0.5, 0.5};
            settings_destination->aspect = 2. / 3.;
            settings_destination->angular_width = 46; // degrees
            settings_destination->plot_galaxy_map = 0;
            settings_destination->projection = SW_PROJECTION_GNOM;
            continue;
        } else if (strcmp(key, "copyright") == 0) {
            //! copyright - The copyright string to write under the star chart
            strcpy(settings_destination->copyright, key_val);
            continue;
        } else if (strcmp(key, "title") == 0) {
            //! title - The heading to write at the top of the star chart
            strcpy(settings_destination->title, key_val);
            continue;
        } else if (strcmp(key, "language") == 0) {
            //! language - The language used for the constellation names. Either "english" or "french".
            if (strcmp(key_val, "english") == 0) settings_destination->language = SW_LANG_EN;
            else if (strcmp(key_val, "french") == 0) settings_destination->language = SW_LANG_FR;
            else {
                snprintf(temp_err_string, FNAME_LENGTH,
                         "Bad input file. language should equal 'english', or 'french'.");
                stch_error(temp_err_string);
                return 1;
            }
            continue;
        } else if (strcmp(key, "font_size") == 0) {
            //! font_size - A normalisation factor to apply to the font size of all text (default 1.0)
            CHECK_KEYVALNUM("font_size")
            settings_destination->font_size = key_val_num;
            continue;
        } else if (strcmp(key, "copyright_gap") == 0) {
            //! copyright_gap - Spacing of the copyright text beneath the plot
            CHECK_KEYVALNUM("copyright_gap")
            settings_destination->copyright_gap = key_val_num;
            continue;
        } else if (strcmp(key, "copyright_gap_2") == 0) {
            //! copyright_gap_2 - Spacing of the copyright text beneath the plot
            CHECK_KEYVALNUM("copyright_gap_2")
            settings_destination->copyright_gap_2 = key_val_num;
            continue;
        } else if (strcmp(key, "constellation_stick_col") == 0) {
            //! constellation_stick_col - Colour to use when drawing constellation stick figures
            settings_destination->constellation_stick_col = colour_from_string(key_val);
            continue;
        } else if (strcmp(key, "grid_col") == 0) {
            //! grid_col - Colour to use when drawing grid of RA/Dec lines
            settings_destination->grid_col = colour_from_string(key_val);
            continue;
        } else if (strcmp(key, "constellation_boundary_col") == 0) {
            //! constellation_boundary_col - Colour to use when drawing constellation boundaries
            settings_destination->constellation_boundary_col = colour_from_string(key_val);
            continue;
        } else if (strcmp(key, "ephemeris_col") == 0) {
            //! ephemeris_col - Colour to use when drawing ephemerides for solar system objects
            settings_destination->ephemeris_col = colour_from_string(key_val);
            continue;
        } else if (strcmp(key, "dso_cluster_col") == 0) {
            //! dso_cluster_col - Colour to use when drawing star clusters
            settings_destination->dso_cluster_col = colour_from_string(key_val);
            continue;
        } else if (strcmp(key, "dso_galaxy_col") == 0) {
            //! dso_galaxy_col - Colour to use when drawing galaxies
            settings_destination->dso_galaxy_col = colour_from_string(key_val);
            continue;
        } else if (strcmp(key, "dso_nebula_col") == 0) {
            //! dso_nebula_col - Colour to use when drawing nebulae
            settings_destination->dso_nebula_col = colour_from_string(key_val);
            continue;
        } else if (strcmp(key, "dso_label_col") == 0) {
            //! dso_label_col - Colour to use when labelling deep sky objects
            settings_destination->dso_label_col = colour_from_string(key_val);
            continue;
        } else if (strcmp(key, "dso_outline_col") == 0) {
            //! dso_outline_col - Colour to use when drawing the outline of deep sky objects
            settings_destination->dso_outline_col = colour_from_string(key_val);
            continue;
        } else if (strcmp(key, "constellation_label_col") == 0) {
            //! constellation_label_col - Colour to use when writing constellation names
            settings_destination->constellation_label_col = colour_from_string(key_val);
            continue;
        } else if (strcmp(key, "zodiacal_only") == 0) {
            //! zodiacal_only - Boolean (0 or 1) indicating whether we plot only the zodiacal constellations
            CHECK_KEYVALNUM("zodiacal_only")
            settings_destination->zodiacal_only = (int) key_val_num;
            continue;
        } else if (strcmp(key, "projection") == 0) {
            //! projection - Select projection to use. Set to either flat, peters, gnomonic, sphere or alt_az
            if (strcmp(key_val, "flat") == 0) settings_destination->projection = SW_PROJECTION_FLAT;
            else if (strcmp(key_val, "peters") == 0) {
                settings_destination->projection = SW_PROJECTION_PETERS;
                settings_destination->aspect = 4 / (2 * M_PI);
                settings_destination->angular_width = 360.;
            } else if (strcmp(key_val, "gnomonic") == 0) settings_destination->projection = SW_PROJECTION_GNOM;
            else if (strcmp(key_val, "sphere") == 0) {
                settings_destination->projection = SW_PROJECTION_SPH;
                settings_destination->aspect = 1.;
                settings_destination->angular_width = 180.;
            } else if (strcmp(key_val, "alt_az") == 0) {
                settings_destination->projection = SW_PROJECTION_ALTAZ;
                settings_destination->aspect = 1.;
                settings_destination->angular_width = 180.;
            } else {
                snprintf(temp_err_string, FNAME_LENGTH,
                         "Bad input file. projection should equal 'flat', 'gnomonic', 'sphere' or 'alt_az'.");
                stch_error(temp_err_string);
                return 1;
            }
            continue;
        } else if (strcmp(key, "coords") == 0) {
            //! coords - Select whether to use RA/Dec or galactic coordinates. Set to either 'ra_dec' or 'galactic'.
            if (strcmp(key_val, "ra_dec") == 0) settings_destination->coords = SW_COORDS_RADEC;
            else if (strcmp(key_val, "galactic") == 0) settings_destination->coords = SW_COORDS_GAL;
            else {
                snprintf(temp_err_string, FNAME_LENGTH, "Bad input file. coords should equal 'ra_dec' or 'galactic'.");
                stch_error(temp_err_string);
                return 1;
            }
            continue;
        } else if (strcmp(key, "angular_width") == 0) {
            //! angular_width - The angular width of the star chart on the sky, degrees
            CHECK_KEYVALNUM("angular_width")
            settings_destination->angular_width = key_val_num;
            continue;
        } else if (strcmp(key, "width") == 0) {
            //! width - The width of the star chart, in cm
            CHECK_KEYVALNUM("width")
            settings_destination->width = key_val_num;
            continue;
        } else if (strcmp(key, "aspect") == 0) {
            //! aspect - The aspect ratio of the star chart: i.e. the ratio height/width
            CHECK_KEYVALNUM("aspect")
            settings_destination->aspect = key_val_num;
            continue;
        } else if (strcmp(key, "ra_dec_lines") == 0) {
            //! ra_dec_lines - Boolean (0 or 1) indicating whether we draw a grid of RA/Dec lines in the background of
            //! the star chart
            CHECK_KEYVALNUM("ra_dec_lines")
            settings_destination->ra_dec_lines = (int) key_val_num;
            continue;
        } else if (strcmp(key, "x_label_slant") == 0) {
            //! x_label_slant - A slant to apply to all labels on the horizontal axes
            CHECK_KEYVALNUM("x_label_slant")
            settings_destination->x_label_slant = key_val_num;
            continue;
        } else if (strcmp(key, "y_label_slant") == 0) {
            //! y_label_slant - A slant to apply to all labels on the vertical axes
            CHECK_KEYVALNUM("y_label_slant")
            settings_destination->y_label_slant = key_val_num;
            continue;
        } else if (strcmp(key, "constellation_boundaries") == 0) {
            //! constellation_boundaries - Boolean (0 or 1) indicating whether we draw constellation boundaries
            CHECK_KEYVALNUM("constellation_boundaries")
            settings_destination->constellation_boundaries = (int) key_val_num;
            continue;
        } else if (strcmp(key, "constellation_sticks") == 0) {
            //! constellation_sticks - Boolean (0 or 1) indicating whether we draw constellation stick figures
            CHECK_KEYVALNUM("constellation_sticks")
            settings_destination->constellation_sticks = (int) key_val_num;
            continue;
        } else if (strcmp(key, "constellation_stick_design") == 0) {
            //! constellation_stick_design - Select which design of constellation stick figures we should draw. Set to
            //! either 'simplified' or 'rey'. See <https://github.com/dcf21/constellation-stick-figures> for more
            //! information.
            if (strcmp(key_val, "simplified") == 0) {
                settings_destination->constellation_stick_design = SW_STICKS_SIMPLIFIED;
            } else if (strcmp(key_val, "rey") == 0) {
                settings_destination->constellation_stick_design = SW_STICKS_REY;
            } else {
                snprintf(temp_err_string, FNAME_LENGTH, "Bad input file. "
                                                        "constellation_stick_design should equal 'simplified' or 'rey'.");
                stch_error(temp_err_string);
                return 1;
            }
            continue;
        } else if (strcmp(key, "constellation_highlight") == 0) {
            //! constellation_highlight - Optionally highlight the boundary of a particular constellation, referenced
            //! by its three-letter abbreviation
            strncpy(settings_destination->constellation_highlight, key_val, 6);
            settings_destination->constellation_highlight[6] = '\0';
            continue;
        } else if (strcmp(key, "plot_stars") == 0) {
            //! plot_stars - Boolean (0 or 1) indicating whether we plot any stars
            CHECK_KEYVALNUM("plot_stars")
            settings_destination->plot_stars = (int) key_val_num;
            continue;
        } else if (strcmp(key, "messier_only") == 0) {
            //! messier_only - Boolean (0 or 1) indicating whether we plot only Messier objects and not other DSOs
            CHECK_KEYVALNUM("messier_only")
            settings_destination->messier_only = (int) key_val_num;
            continue;
        } else if (strcmp(key, "plot_dso") == 0) {
            //! plot_dso - Boolean (0 or 1) indicating whether we plot any deep sky objects
            CHECK_KEYVALNUM("plot_dso")
            settings_destination->plot_dso = (int) key_val_num;
            continue;
        } else if (strcmp(key, "constellation_names") == 0) {
            //! constellation_names - Boolean (0 or 1) indicating whether we label the names of constellations
            CHECK_KEYVALNUM("constellation_names")
            settings_destination->constellation_names = (int) key_val_num;
            continue;
        } else if (strcmp(key, "star_names") == 0) {
            //! star_names - Boolean (0 or 1) indicating whether we label the English names of stars
            CHECK_KEYVALNUM("star_names")
            settings_destination->star_names = (int) key_val_num;
            continue;
        } else if (strcmp(key, "star_catalogue_numbers") == 0) {
            //! star_catalogue_numbers - Boolean (0 or 1) indicating whether we label the catalogue numbers of stars
            CHECK_KEYVALNUM("star_catalogue_numbers")
            settings_destination->star_catalogue_numbers = (int) key_val_num;
            continue;
        } else if (strcmp(key, "star_bayer_labels") == 0) {
            //! star_bayer_labels - Boolean (0 or 1) indicating whether we label the Bayer numbers of stars
            CHECK_KEYVALNUM("star_bayer_labels")
            settings_destination->star_bayer_labels = (int) key_val_num;
            continue;
        } else if (strcmp(key, "star_flamsteed_labels") == 0) {
            //! star_flamsteed_labels - Boolean (0 or 1) indicating whether we label the Flamsteed designations of stars
            CHECK_KEYVALNUM("star_flamsteed_labels")
            settings_destination->star_flamsteed_labels = (int) key_val_num;
            continue;
        } else if (strcmp(key, "star_variable_labels") == 0) {
            //! star_variable_labels - Boolean (0 or 1) indicating whether we label the variable-star designations of stars
            CHECK_KEYVALNUM("star_variable_labels")
            settings_destination->star_variable_labels = (int) key_val_num;
            continue;
        } else if (strcmp(key, "star_allow_multiple_labels") == 0) {
            //! star_allow_multiple_labels - Boolean (0 or 1) indicating whether we allow multiple labels next to a single star
            CHECK_KEYVALNUM("star_allow_multiple_labels")
            settings_destination->star_allow_multiple_labels = (int) key_val_num;
            continue;
        } else if (strcmp(key, "star_catalogue") == 0) {
            //! star_catalogue - Select the star catalogue to use when showing the catalogue numbers of stars. Set to
            //! 'hipparcos', 'ybsc' or 'hd'.
            if (strcmp(key_val, "hipparcos") == 0) settings_destination->star_catalogue = SW_CAT_HIP;
            else if (strcmp(key_val, "ybsc") == 0) settings_destination->star_catalogue = SW_CAT_YBSC;
            else if (strcmp(key_val, "hd") == 0) settings_destination->star_catalogue = SW_CAT_HD;
            else {
                snprintf(temp_err_string, FNAME_LENGTH,
                         "Bad input file. star_catalogue should equal 'hipparcos', 'ybsc' or 'hd'.");
                stch_error(temp_err_string);
                return 1;
            }
            continue;
        } else if (strcmp(key, "star_mag_labels") == 0) {
            //! star_mag_labels - Boolean (0 or 1) indicating whether we label the magnitudes of stars
            CHECK_KEYVALNUM("star_mag_labels")
            settings_destination->star_mag_labels = (int) key_val_num;
            continue;
        } else if (strcmp(key, "star_label_mag_min") == 0) {
            //! star_label_mag_min - Do not label stars fainter than this magnitude limit
            CHECK_KEYVALNUM("star_label_mag_min")
            settings_destination->star_label_mag_min = key_val_num;
            continue;
        } else if (strcmp(key, "dso_label_mag_min") == 0) {
            //! dso_label_mag_min - Do not label DSOs fainter than this magnitude limit
            CHECK_KEYVALNUM("dso_label_mag_min")
            settings_destination->dso_label_mag_min = key_val_num;
            continue;
        } else if (strcmp(key, "dso_names") == 0) {
            //! dso_names - Boolean (0 or 1) indicating whether we label the names of deep sky objects
            CHECK_KEYVALNUM("dso_names")
            settings_destination->dso_names = (int) key_val_num;
            continue;
        } else if (strcmp(key, "dso_mags") == 0) {
            //! dso_mags - Boolean (0 or 1) indicating whether we label the magnitudes of deep sky objects
            CHECK_KEYVALNUM("dso_mags")
            settings_destination->dso_mags = (int) key_val_num;
            continue;
        } else if (strcmp(key, "dso_mag_min") == 0) {
            //! dso_mag_min - Only show deep sky objects down to this faintest magnitude
            CHECK_KEYVALNUM("dso_mag_min")
            settings_destination->dso_mag_min = key_val_num;
            continue;
        } else if (strcmp(key, "mag_min") == 0) {
            //! mag_min - The faintest magnitude of star which we draw
            CHECK_KEYVALNUM("mag_min")
            settings_destination->mag_min = key_val_num;
            settings_destination->mag_min_automatic = 0;
            continue;
        } else if (strcmp(key, "mag_max") == 0) {
            //! mag_max - Used to regulate the size of stars. A star of this magnitude is drawn with size mag_size_norm.
            //! Also, this is the brightest magnitude of star which is shown in the magnitude key below the chart.
            CHECK_KEYVALNUM("mag_max")
            settings_destination->mag_max = key_val_num;
            continue;
        } else if (strcmp(key, "mag_step") == 0) {
            //! mag_step - The magnitude interval between the samples shown on the magnitude key under the chart
            CHECK_KEYVALNUM("mag_step")
            settings_destination->mag_step = key_val_num;
            continue;
        } else if (strcmp(key, "mag_alpha") == 0) {
            //! mag_alpha - The multiplicative scaling factor to apply to the radii of stars differing in magnitude by
            //! one <mag_step>
            CHECK_KEYVALNUM("mag_alpha")
            settings_destination->mag_alpha = key_val_num;
            continue;
        } else if (strcmp(key, "mag_size_norm") == 0) {
            //! mag_size_norm - The radius of a star of magnitude <mag_max> (default 1.0)
            CHECK_KEYVALNUM("mag_size_norm")
            settings_destination->mag_size_norm = key_val_num;
            continue;
        } else if (strcmp(key, "maximum_star_count") == 0) {
            //! maximum_star_count - The maximum number of stars to draw. If this is exceeded, only the brightest stars
            //! are shown.
            CHECK_KEYVALNUM("maximum_star_count")
            settings_destination->maximum_star_count = (int) key_val_num;
            continue;
        } else if (strcmp(key, "maximum_star_label_count") == 0) {
            //! maximum_star_label_count - The maximum number of stars which may be labelled
            CHECK_KEYVALNUM("maximum_star_label_count")
            settings_destination->maximum_star_label_count = (int) key_val_num;
            continue;
        } else if (strcmp(key, "maximum_dso_count") == 0) {
            //! maximum_dso_count - The maximum number of DSOs to draw. If this is exceeded, only the brightest objects
            //! are shown.
            CHECK_KEYVALNUM("maximum_dso_count")
            settings_destination->maximum_dso_count = (int) key_val_num;
            continue;
        } else if (strcmp(key, "maximum_dso_label_count") == 0) {
            //! maximum_dso_label_count - The maximum number of DSOs which may be labelled
            CHECK_KEYVALNUM("maximum_dso_label_count")
            settings_destination->maximum_dso_label_count = (int) key_val_num;
            continue;
        } else if (strcmp(key, "plot_ecliptic") == 0) {
            //! plot_ecliptic - Boolean (0 or 1) indicating whether to draw a line along the ecliptic
            CHECK_KEYVALNUM("plot_ecliptic")
            settings_destination->plot_ecliptic = (int) key_val_num;
            continue;
        } else if (strcmp(key, "label_ecliptic") == 0) {
            //! label_ecliptic - Boolean (0 or 1) indicating whether to label the months along the ecliptic, showing
            //! the Sun's annual progress
            CHECK_KEYVALNUM("label_ecliptic")
            settings_destination->label_ecliptic = (int) key_val_num;
            continue;
        } else if (strcmp(key, "plot_galactic_plane") == 0) {
            //! plot_galactic_plane - Boolean (0 or 1) indicating whether to draw a line along the galactic plane
            CHECK_KEYVALNUM("plot_galactic_plane")
            settings_destination->plot_galactic_plane = (int) key_val_num;
            continue;
        } else if (strcmp(key, "plot_equator") == 0) {
            //! plot_equator - Boolean (0 or 1) indicating whether to draw a line along the equator
            CHECK_KEYVALNUM("plot_equator")
            settings_destination->plot_equator = (int) key_val_num;
            continue;
        } else if (strcmp(key, "ecliptic_col") == 0) {
            //! ecliptic_col - Colour to use when drawing a line along the ecliptic
            settings_destination->ecliptic_col = colour_from_string(key_val);
            continue;
        } else if (strcmp(key, "galactic_plane_col") == 0) {
            //! galactic_plane_col - Colour to use when drawing a line along the galactic plane
            settings_destination->galactic_plane_col = colour_from_string(key_val);
            continue;
        } else if (strcmp(key, "equator_col") == 0) {
            //! equator_col - Colour to use when drawing a line along the equator
            settings_destination->equator_col = colour_from_string(key_val);
            continue;
        } else if (strcmp(key, "plot_galaxy_map") == 0) {
            //! plot_galaxy_map - Boolean (0 or 1) indicating whether to draw a shaded map of the Milky Way behind
            //! the star chart
            CHECK_KEYVALNUM("plot_galaxy_map")
            settings_destination->plot_galaxy_map = (int) key_val_num;
            continue;
        } else if (strcmp(key, "galaxy_map_width_pixels") == 0) {
            //! galaxy_map_width_pixels - The number of horizontal pixels across the shaded map of the Milky Way
            CHECK_KEYVALNUM("galaxy_map_width_pixels")
            settings_destination->galaxy_map_width_pixels = (int) key_val_num;
            continue;
        } else if (strcmp(key, "galaxy_col") == 0) {
            //! galaxy_col - The colour to use to shade the bright parts of the map of the Milky Way
            settings_destination->galaxy_col = colour_from_string(key_val);
            continue;
        } else if (strcmp(key, "galaxy_col0") == 0) {
            //! galaxy_col0 - The colour to use to shade the dark parts of the map of the Milky Way
            settings_destination->galaxy_col0 = colour_from_string(key_val);
            continue;
        } else if (strcmp(key, "star_col") == 0) {
            //! star_col - Colour to use when drawing stars
            settings_destination->star_col = colour_from_string(key_val);
            continue;
        } else if (strcmp(key, "star_label_col") == 0) {
            //! star_label_col - Colour to use when labelling stars
            settings_destination->star_label_col = colour_from_string(key_val);
            continue;
        } else if (strcmp(key, "magnitude_key") == 0) {
            //! magnitude_key - Boolean (0 or 1) indicating whether to draw a key to the magnitudes of stars under
            //! the star chart
            CHECK_KEYVALNUM("magnitude_key")
            settings_destination->magnitude_key = (int) key_val_num;
            continue;
        } else if (strcmp(key, "great_circle_key") == 0) {
            //! great_circle_key - Boolean (0 or 1) indicating whether to draw a key to the great circles under the
            //! star chart
            CHECK_KEYVALNUM("great_circle_key")
            settings_destination->great_circle_key = (int) key_val_num;
            continue;
        } else if (strcmp(key, "dso_symbol_key") == 0) {
            //! dso_symbol_key - Boolean (0 or 1) indicating whether to draw a key to the deep sky object symbols
            CHECK_KEYVALNUM("dso_symbol_key")
            settings_destination->dso_symbol_key = (int) key_val_num;
            continue;
        } else if (strcmp(key, "cardinals") == 0) {
            //! cardinals - Boolean (0 or 1) indicating whether to write the cardinal points around the edge of
            //! alt/az star charts
            CHECK_KEYVALNUM("cardinals")
            settings_destination->cardinals = (int) key_val_num;
            continue;
        } else if (strcmp(key, "label_font_size_scaling") == 0) {
            //! label_font_size_scaling - Scaling factor to be applied to the font size of all star and DSO labels
            CHECK_KEYVALNUM("label_font_size_scaling")
            settings_destination->label_font_size_scaling = key_val_num;
            continue;
        } else if (strcmp(key, "draw_ephemeris") == 0) {
            //! draw_ephemeris - Definitions of ephemerides to draw
            strcpy(settings_destination->ephemeris_definitions[settings_destination->ephemeride_count], key_val);
            settings_destination->ephemeride_count++;
            continue;
        } else if (strcmp(key, "ephemeris_autoscale") == 0) {
            //! ephemeris_autoscale - Boolean (0 or 1) indicating whether to auto-scale the star chart to contain the
            //! requested ephemerides. This overrides settings for ra_central, dec_central and angular_width.
            CHECK_KEYVALNUM("ephemeris_autoscale")
            settings_destination->ephemeris_autoscale = (int) key_val_num;
            continue;
        } else if (strcmp(key, "ephemeris_table") == 0) {
            //! ephemeris_table - Boolean (0 or 1) indicating whether to include a table of the object's magnitude
            CHECK_KEYVALNUM("ephemeris_table")
            settings_destination->ephemeris_table = (int) key_val_num;
            continue;
        } else if (strcmp(key, "must_show_all_ephemeris_labels") == 0) {
            //! ephemeris_autoscale - Boolean (0 or 1) indicating whether we must show all ephemeris text labels,
            //! even if they collide with other text.
            CHECK_KEYVALNUM("must_show_all_ephemeris_labels")
            settings_destination->must_show_all_ephemeris_labels = (int) key_val_num;
            continue;
        } else if (strcmp(key, "ephemeris_compute_path") == 0) {
            //! ephemeris_compute_path - The path to the tool <ephemerisCompute>, used to compute paths for solar
            //! system objects. See <https://github.com/dcf21/ephemeris-compute-de430>. If this tool is installed in the
            //! same directory as StarCharter, the default value should be <../ephemeris-compute-de430/bin/ephem.bin>.
            strcpy(settings_destination->ephemeris_compute_path, key_val);
            continue;
        } else {
            snprintf(temp_err_string, FNAME_LENGTH, "Bad input file. Unrecognised setting '%s'.", key);
            stch_error(temp_err_string);
            return 1;
        }
    }
    if (got_chart) render_chart(&this_chart_config); // Render final star chart

    // If we're reading from a configuration file, close the file handle
    if (have_filename) fclose(infile);

    // Clean up and exit
    lt_freeAll(0);
    lt_memoryStop();
    if (DEBUG) stch_log("Terminating normally.");
    return 0;
}
