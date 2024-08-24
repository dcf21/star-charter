// read_config.c
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

#include "settings/chart_config.h"
#include "settings/read_config.h"
#include "settings/render_chart.h"


// Macro to check that a parameter in a configuration file has a numeric value
#define CHECK_VALUE_NUMERIC(NAME) \
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

#pragma clang diagnostic push
#pragma ide diagnostic ignored "misc-no-recursion"

//! process_configuration_file_line - Process a single line from an input configuration file.
//! \param [in] line - The line of configuration text we should parse.
//! \param [in] filename - The filename of the current file we are reading (shown in errors).
//! \param [in] iteration_depth - The depth of iterative opening of included config files
//! \param [in] file_line_number - The number of the current line in the file we are reading (shown in errors).
//! \param [in|out] got_chart - Boolean indicating whether we have passed a "CHART" config block that we've not yet rendered.
//! \param [in] chart_defaults - chart_config data structure containing all our default chart settings.
//! \param [out] this_chart_config - The configuration for the chart described by the current "CHART" config block.
//! \param [in|out] settings_destination - The configuration structure we are currently reading settings into.
//! \return - Boolean; 0 means line was parsed without error; 1 means there was a fatal error

int process_configuration_file_line(char *line, const char *filename, const int iteration_depth, int file_line_number,
                                    int *got_chart, chart_config *chart_defaults,
                                    chart_config *this_chart_config, chart_config **settings_destination) {

    // Check for NULL pointers
    if (line == NULL) {
        stch_fatal(__FILE__, __LINE__, "Received NULL value for <line>.");
        exit(1);
    }

    if (got_chart == NULL) {
        stch_fatal(__FILE__, __LINE__, "Received NULL value for <got_chart>.");
        exit(1);
    }

    if (settings_destination == NULL) {
        stch_fatal(__FILE__, __LINE__, "Received NULL value for <settings_destination>.");
        exit(1);
    }

    if (this_chart_config == NULL) {
        stch_fatal(__FILE__, __LINE__, "Received NULL value for <this_chart_config>.");
        exit(1);
    }

    // Strip spaces from beginning/end of line
    str_strip(line, line);

    // Ignore blank lines
    if (strlen(line) == 0) return 0;

    // Ignore comment lines
    if (line[0] == '#') return 0;

    // Check for included files
    if (strncmp(line, "INCLUDE", 7) == 0) {
        // Throw error if we've exceeded maximum iteration depth
        if (iteration_depth > MAX_INCLUSION_DEPTH) {
            stch_fatal(__FILE__, __LINE__, "Maximum include file iteration depth exceeded.");
            exit(1);
        }

        // Compute the file path of the file we are currently reading. Included files are relative to this.
        char *current_path = NULL, *current_file = NULL;
        if (filename != NULL) {
            split_file_path(&current_path, &current_file, filename);
        }

        // Read path to the file we have been requested to include
        const char *scan = line;
        char included_filename[FNAME_LENGTH];
        scan = next_word(scan);
        get_word(included_filename, scan, FNAME_LENGTH);

        // Append path to filename
        char full_included_path[FNAME_LENGTH];
        if (current_path != NULL) {
            snprintf(full_included_path, FNAME_LENGTH, "%s/%s", current_path, included_filename);
        } else {
            snprintf(full_included_path, FNAME_LENGTH, "%s", included_filename);
        }

        // Parse requested file
        int status = read_configuration_file(full_included_path, iteration_depth + 1, got_chart,
                                             chart_defaults, this_chart_config, settings_destination);

        // Free malloced memory
        if (current_path != NULL) free(current_path);
        if (current_file != NULL) free(current_file);

        // Return status flag
        return status;
    }

    // Check for the special lines <DEFAULTS> and <CHART> which indicate which data structure to direct settings into
    if (strcmp(line, "DEFAULTS") == 0) {
        // The heading "DEFAULTS" means that we're receiving settings which should apply to all star charts which
        // follow

        // If this follows a CHART definition, then we have all the settings for that chart, and should render
        // it now
        if (*got_chart) render_chart(this_chart_config);
        *got_chart = 0;

        // Feed subsequent settings into the default chart configuration
        *settings_destination = chart_defaults;
        return 0;
    } else if (strcmp(line, "CHART") == 0) {
        // The heading "CHART" means that we're receiving settings which should apply to a new star chart

        // If this follows a previous CHART definition, then we have all the settings for that chart, and should
        // render it now
        if (*got_chart) render_chart(this_chart_config);

        // Feed subsequent settings into the this_chart_config
        *got_chart = 1;
        *settings_destination = this_chart_config;
        *this_chart_config = *chart_defaults;
        return 0;
    }

    // If line was not either DEFAULTS or CHART, it is a configuration setting.
    // If <settings_destination> is NULL, that means we haven't seen a heading DEFAULTS or CHART to tell us
    // where to put this configuration setting.
    if ((*settings_destination) == NULL) {
        snprintf(temp_err_string, FNAME_LENGTH, "Bad input file. Must begin with either 'DEFAULTS', or 'CHART'.");
        stch_error(temp_err_string);
        return 1;
    }

    // Parse the configuration setting, which should be of the form <key = value>
    char key[LSTR_LENGTH], key_val[LSTR_LENGTH];
    readConfig_fetchKey(line, key);
    readConfig_fetchValue(line, key_val);

    // Test whether the setting value is a valid floating point string
    const int key_val_num_valid = valid_float(key_val, NULL);
    const double key_val_num = get_float(key_val, NULL);

    if (strcmp(key, "ra_central") == 0) {
        //! ra_central - The right ascension of the centre of the plot, hours
        //! This setting is only used if <coords=SW_COORDS_RADEC>
        CHECK_VALUE_NUMERIC("ra_central")
        (*settings_destination)->ra0 = fmod(key_val_num, 24);
        while ((*settings_destination)->ra0 < 0) (*settings_destination)->ra0 += 24;
        while ((*settings_destination)->ra0 >= 24) (*settings_destination)->ra0 -= 24;
        return 0;
    } else if (strcmp(key, "dec_central") == 0) {
        //! dec_central - The declination of the centre of the plot, degrees
        //! This setting is only used if <coords=SW_COORDS_RADEC>
        CHECK_VALUE_NUMERIC("dec_central")
        (*settings_destination)->dec0 = key_val_num;
        if ((*settings_destination)->dec0 > 90) (*settings_destination)->dec0 = 90;
        if ((*settings_destination)->dec0 < -90) (*settings_destination)->dec0 = -90;
        return 0;
    } else if (strcmp(key, "galactic_l_central") == 0) {
        //! galactic_l_central - The galactic longitude of the centre of the plot, degrees
        //! This setting is only used if <coords=SW_COORDS_GALACTIC>
        CHECK_VALUE_NUMERIC("galactic_l_central")
        (*settings_destination)->l0 = fmod(key_val_num, 360);
        while ((*settings_destination)->l0 < 0) (*settings_destination)->l0 += 360;
        while ((*settings_destination)->l0 >= 360) (*settings_destination)->l0 -= 360;
        return 0;
    } else if (strcmp(key, "galactic_b_central") == 0) {
        //! galactic_b_central - The galactic latitude of the centre of the plot, degrees
        //! This setting is only used if <coords=SW_COORDS_GALACTIC>
        CHECK_VALUE_NUMERIC("galactic_b_central")
        (*settings_destination)->b0 = key_val_num;
        if ((*settings_destination)->b0 > 90) (*settings_destination)->b0 = 90;
        if ((*settings_destination)->b0 < -90) (*settings_destination)->b0 = -90;
        return 0;
    } else if (strcmp(key, "alt_central") == 0) {
        //! alt_central - The altitude of the centre of the plot, degrees
        //! This setting is only used if <coords=SW_COORDS_ALTAZ>
        CHECK_VALUE_NUMERIC("alt_central")
        (*settings_destination)->alt0 = key_val_num;
        if ((*settings_destination)->alt0 > 90) (*settings_destination)->alt0 = 90;
        if ((*settings_destination)->alt0 < -90) (*settings_destination)->alt0 = -90;
        return 0;
    } else if (strcmp(key, "az_central") == 0) {
        //! az_central - The azimuth of the centre of the plot, degrees
        //! This setting is only used if <coords=SW_COORDS_ALTAZ>
        CHECK_VALUE_NUMERIC("az_central")
        (*settings_destination)->az0 = fmod(key_val_num, 360);
        while ((*settings_destination)->az0 < 0) (*settings_destination)->az0 += 360;
        while ((*settings_destination)->az0 >= 360) (*settings_destination)->az0 -= 360;
        return 0;
    } else if (strcmp(key, "position_angle") == 0) {
        //! position_angle - The position angle of the plot - i.e. the tilt of north in degrees, counter-clockwise
        //! from up, at the centre of the plot
        CHECK_VALUE_NUMERIC("position_angle")
        (*settings_destination)->position_angle = key_val_num;
        return 0;
    } else if (strcmp(key, "text") == 0) {
        //! text - Overlay a custom text label on the star chart. Each label should be specified in the format:
        //! `<xpos>,<ypos>,<xalign>,<yalign>,<font_size>,<colour r>,<colour g>,<colour b>,<label string>`
        //! where `xpos` and `ypos` are in the range 0-1, `xalign` and `yalign` are in the range -1 (left) to 1 (right),
        //! and colour components are in the range 0-1. To overlay multiple text labels, specify this setting multiple
        //! times within your configuration file.
        if ((*settings_destination)->text_labels_custom_count > N_TRACES_MAX - 4) {
            snprintf(temp_err_string, FNAME_LENGTH,
                     "Bad input file. Too many entries for <text>.");
            stch_error(temp_err_string);
            return 1;
        }
        strcpy((*settings_destination)->text_labels[(*settings_destination)->text_labels_custom_count],
               key_val);
        (*settings_destination)->text_labels_custom_count++;
        return 0;
    } else if (strcmp(key, "axis_ticks_value_only") == 0) {
        //! axis_ticks_value_only - If 1, axis labels will appear as simply "5h" or "30 deg". If 0, these labels
        //! will be preceded by alpha= or delta=
        CHECK_VALUE_NUMERIC("axis_ticks_value_only")
        (*settings_destination)->axis_ticks_value_only = (int) key_val_num;
        return 0;
    } else if (strcmp(key, "axis_label") == 0) {
        //! axis_label - Boolean (0 or 1) indicating whether to write "Right ascension" and "Declination" on the
        //! vertical/horizontal axes
        CHECK_VALUE_NUMERIC("axis_label")
        (*settings_destination)->axis_label = (int) key_val_num;
        return 0;
    } else if (strcmp(key, "output_dpi") == 0) {
        //! output_dpi - The DPI resolution of the output file. Default 200 DPI for PNG files; 72 DPI for
        //! vector graphics.
        CHECK_VALUE_NUMERIC("output_dpi")
        (*settings_destination)->dpi = key_val_num;
        return 0;
    } else if (strcmp(key, "output_filename") == 0) {
        //! output_filename - The target filename for the star chart. The file type (svg, png, eps or pdf) is
        //! inferred from the file extension.
        strcpy((*settings_destination)->output_filename, key_val);
        return 0;
    } else if (strcmp(key, "galaxy_map_filename") == 0) {
        //! galaxy_map_filename - The binary file from which to read the shaded map of the Milky Way
        strcpy((*settings_destination)->galaxy_map_filename, key_val);
        return 0;
    } else if (strcmp(key, "photo_filename") == 0) {
        //! photo_filename - The filename of a PNG image to render behind the star chart. Leave blank to show no
        //! image.
        strcpy((*settings_destination)->photo_filename, key_val);
        (*settings_destination)->star_col = (colour) {0.75, 0.75, 0.25};
        (*settings_destination)->grid_col = (colour) {0.5, 0.5, 0.5};
        (*settings_destination)->aspect = 2. / 3.;
        (*settings_destination)->angular_width = 46; // degrees
        (*settings_destination)->plot_galaxy_map = 0;
        (*settings_destination)->projection = SW_PROJECTION_GNOMONIC;
        return 0;
    } else if (strcmp(key, "copyright") == 0) {
        //! copyright - The copyright string to write under the star chart
        strcpy((*settings_destination)->copyright, key_val);
        return 0;
    } else if (strcmp(key, "title") == 0) {
        //! title - The heading to write at the top of the star chart
        strcpy((*settings_destination)->title, key_val);
        return 0;
    } else if (strcmp(key, "language") == 0) {
        //! language - The language used for the constellation names. Either "english" or "french".
        if (strcmp(key_val, "english") == 0) (*settings_destination)->language = SW_LANG_ENGLISH;
        else if (strcmp(key_val, "french") == 0) (*settings_destination)->language = SW_LANG_FRENCH;
        else {
            snprintf(temp_err_string, FNAME_LENGTH,
                     "Bad input file. language should equal 'english', or 'french'.");
            stch_error(temp_err_string);
            return 1;
        }
        return 0;
    } else if (strcmp(key, "show_horizon") == 0) {
        //! show_horizon - Boolean (0 or 1) indicating whether we show the local horizon and clip objects below
        //! the horizon at `julian_date`
        CHECK_VALUE_NUMERIC("show_horizon")
        (*settings_destination)->show_horizon = (int) key_val_num;
        return 0;
    } else if (strcmp(key, "show_zenith") == 0) {
        //! show_zenith - Boolean (0 or 1) indicating whether we mark the local zenith at `julian_date`
        CHECK_VALUE_NUMERIC("show_zenith")
        (*settings_destination)->show_horizon_zenith = (int) key_val_num;
        return 0;
    } else if (strcmp(key, "horizon_zenith_marker_size") == 0) {
        //! horizon_zenith_marker_size - Scaling factor to apply to the size of the marker used at the zenith. Default 1.
        CHECK_VALUE_NUMERIC("horizon_zenith_marker_size")
        (*settings_destination)->horizon_zenith_marker_size = (int) key_val_num;
        return 0;
    } else if (strcmp(key, "horizon_zenith_col") == 0) {
        //! horizon_zenith_col - Colour to use for the marker at the zenith.
        (*settings_destination)->horizon_zenith_colour = colour_from_string(key_val);
        return 0;
    } else if (strcmp(key, "horizon_latitude") == 0) {
        //! horizon_latitude - Terrestrial latitude for which to show the local horizon; degrees
        CHECK_VALUE_NUMERIC("horizon_latitude")
        (*settings_destination)->horizon_latitude = key_val_num;
        return 0;
    } else if (strcmp(key, "horizon_latitude") == 0) {
        //! horizon_latitude - Terrestrial latitude for which to show the local horizon; degrees
        CHECK_VALUE_NUMERIC("horizon_latitude")
        (*settings_destination)->horizon_latitude = key_val_num;
        return 0;
    } else if (strcmp(key, "horizon_longitude") == 0) {
        //! horizon_longitude - Terrestrial longitude for which to show the local horizon; degrees
        CHECK_VALUE_NUMERIC("horizon_longitude")
        (*settings_destination)->horizon_longitude = key_val_num;
        return 0;
    } else if (strcmp(key, "horizon_cardinal_points_marker_col") == 0) {
        //! horizon_cardinal_points_marker_col - Colour to use when drawing cardinal-point markers along the horizon.
        (*settings_destination)->horizon_cardinal_points_marker_colour = colour_from_string(key_val);
        return 0;
    } else if (strcmp(key, "horizon_cardinal_points_labels_col") == 0) {
        //! horizon_cardinal_points_labels_col - Colour to use when drawing cardinal-point labels along the horizon.
        (*settings_destination)->horizon_cardinal_points_labels_colour = colour_from_string(key_val);
        return 0;
    } else if (strcmp(key, "horizon_cardinal_points_marker_size") == 0) {
        //! horizon_cardinal_points_marker_size - Size scaling of the cardinal-point markers along the horizon. Default 1.
        CHECK_VALUE_NUMERIC("horizon_cardinal_points_marker_size")
        (*settings_destination)->horizon_cardinal_points_marker_size = key_val_num;
        return 0;
    } else if (strcmp(key, "horizon_cardinal_points_marker_count") == 0) {
        //! horizon_cardinal_points_marker_count - Number of cardinal-point markers to place along the horizon. Sensible values are 4, 8, 16. Default 8.
        CHECK_VALUE_NUMERIC("horizon_cardinal_points_marker_count")
        (*settings_destination)->horizon_cardinal_points_marker_count = (int) key_val_num;
        return 0;
    } else if (strcmp(key, "horizon_cardinal_points_marker_elevate") == 0) {
        //! horizon_cardinal_points_marker_elevate - Boolean flag (0 or 1) indicating whether to elevate cardinal point markers to the bottom of the field of view if they fall off the bottom of the chart.
        CHECK_VALUE_NUMERIC("horizon_cardinal_points_marker_elevate")
        (*settings_destination)->horizon_cardinal_points_marker_elevate = (int) key_val_num;
        return 0;
    } else if (strcmp(key, "julian_date") == 0) {
        //! julian_date - Julian date for which to show local horizon, with which to measure alt/az, and for which
        //! to show the positions of solar system bodies.
        CHECK_VALUE_NUMERIC("julian_date")
        (*settings_destination)->julian_date = key_val_num;
        return 0;
    } else if (strcmp(key, "meteor_radiant") == 0) {
        //! meteor_radiant - Specify that the radiant of a meteor shower should be marked. This should be set to a
        //! string of the form `<shower_label>,<ra_radiant/deg>,<dec_radiant/deg>`. To mark multiple shower
        //! radiants, supply this setting multiple times.
        if ((*settings_destination)->meteor_radiants_custom_count > N_TRACES_MAX - 4) {
            snprintf(temp_err_string, FNAME_LENGTH,
                     "Bad input file. Too many entries for <meteor_radiant>.");
            stch_error(temp_err_string);
            return 1;
        }
        strcpy((*settings_destination)->meteor_radiants[(*settings_destination)->meteor_radiants_custom_count],
               key_val);
        (*settings_destination)->meteor_radiants_custom_count++;
        return 0;
    } else if (strcmp(key, "meteor_radiant_marker_size") == 0) {
        //! meteor_radiant_marker_size - Scaling factor to apply to the size of the markers at the radiants of meteor showers. Default 1.
        CHECK_VALUE_NUMERIC("meteor_radiant_marker_size")
        (*settings_destination)->meteor_radiant_marker_size = (int) key_val_num;
        return 0;
    } else if (strcmp(key, "meteor_radiant_colour") == 0) {
        //! meteor_radiant_colour - Colour to use for the markers at the radiants of meteor showers.
        (*settings_destination)->meteor_radiant_colour = colour_from_string(key_val);
        return 0;
    } else if (strcmp(key, "show_solar_system") == 0) {
        //! show_solar_system - Boolean (0 or 1) indicating whether we show the positions of solar system bodies
        //! at `julian_date`
        CHECK_VALUE_NUMERIC("show_solar_system")
        (*settings_destination)->show_solar_system = (int) key_val_num;
        return 0;
    } else if (strcmp(key, "solar_system_labels") == 0) {
        //! solar_system_labels - The list of labels to show next to the selected solar system bodies. If multiple
        //! solar system bodies are to be displayed/labelled, then specify this setting multiple times, once for
        //! each body. The number of values of <solar_system_ids> must equal the number of values of
        //! <solar_system_labels>.
        if ((*settings_destination)->solar_system_labels_custom_count > N_TRACES_MAX - 4) {
            snprintf(temp_err_string, FNAME_LENGTH,
                     "Bad input file. Too many entries for <solar_system_labels>.");
            stch_error(temp_err_string);
            return 1;
        }
        strcpy((*settings_destination)->solar_system_labels[(*settings_destination)->solar_system_labels_custom_count],
               key_val);
        (*settings_destination)->solar_system_labels_custom_count++;
        return 0;
    } else if (strcmp(key, "solar_system_ids") == 0) {
        //! solar_system_ids - The list of the ID strings of the solar system bodies to show (e.g. `P1` for
        //! Mercury). If multiple solar system bodies are to be displayed/labelled, then specify this setting
        //! multiple times, once for each body. The number of values of <solar_system_ids> must equal the number
        //! of values of <solar_system_labels>.
        if ((*settings_destination)->solar_system_ids_custom_count > N_TRACES_MAX - 4) {
            snprintf(temp_err_string, FNAME_LENGTH,
                     "Bad input file. Too many entries for <solar_system_ids>.");
            stch_error(temp_err_string);
            return 1;
        }
        strcpy((*settings_destination)->solar_system_ids[(*settings_destination)->solar_system_ids_custom_count],
               key_val);
        (*settings_destination)->solar_system_ids_custom_count++;
        return 0;
    } else if (strcmp(key, "solar_system_col") == 0) {
        //! solar_system_col - The colour to use when drawing solar-system objects. If this setting is supplied
        //! multiple times, then the list of supplied colours are used in a cyclic loop for all the solar system
        //! objects to be drawn.
        if ((*settings_destination)->solar_system_colour_custom_count > N_TRACES_MAX - 4) {
            snprintf(temp_err_string, FNAME_LENGTH,
                     "Bad input file. Too many entries for <solar_system_col>.");
            stch_error(temp_err_string);
            return 1;
        }
        (*settings_destination)->solar_system_colour[(*settings_destination)->solar_system_colour_custom_count] =
                colour_from_string(key_val);
        (*settings_destination)->solar_system_colour_custom_count++;
        return 0;
    } else if (strcmp(key, "solar_system_show_moon_phase") == 0) {
        //! solar_system_show_moon_phase - Boolean flag (0 or 1) indicating whether to show the Moon's phase (1), or
        //! show a simple marker (0).
        CHECK_VALUE_NUMERIC("solar_system_show_moon_phase")
        (*settings_destination)->solar_system_show_moon_phase = (int) key_val_num;
        return 0;
    } else if (strcmp(key, "solar_system_moon_earthshine_intensity") == 0) {
        //! solar_system_moon_earthshine_intensity - The fractional intensity of Earthshine on the Moon's
        //! unilluminated portion, compared to the illuminated Moon.
        CHECK_VALUE_NUMERIC("solar_system_moon_earthshine_intensity")
        (*settings_destination)->solar_system_moon_earthshine_intensity = key_val_num;
        return 0;
    } else if (strcmp(key, "solar_system_moon_colour") == 0) {
        //! solar_system_moon_colour - The colour to use to represent the illuminated portion of the Moon.
        (*settings_destination)->solar_system_moon_colour = colour_from_string(key_val);
        return 0;
    } else if (strcmp(key, "shade_twilight") == 0) {
        //! shade_twilight - Boolean (0 or 1) indicating whether to shade the sky according to the altitude in
        //! the local sky at `julian_date`.
        CHECK_VALUE_NUMERIC("shade_twilight")
        (*settings_destination)->shade_twilight = (int) key_val_num;
        return 0;
    } else if (strcmp(key, "shade_near_sun") == 0) {
        //! shade_near_sun - Boolean (0 or 1) indicating whether to shade the region of sky that is close to the
        //! Sun at `julian_date`.
        CHECK_VALUE_NUMERIC("shade_near_sun")
        (*settings_destination)->shade_near_sun = (int) key_val_num;
        return 0;
    } else if (strcmp(key, "shade_not_observable") == 0) {
        //! shade_not_observable - Boolean (0 or 1) indicating whether to shade the region of sky that is not
        //! observable at any time of day at `julian_date`.
        CHECK_VALUE_NUMERIC("shade_not_observable")
        (*settings_destination)->shade_not_observable = (int) key_val_num;
        return 0;
    } else if (strcmp(key, "twilight_zenith_col") == 0) {
        //! twilight_zenith_col - The colour to use to shade twilight at the zenith
        (*settings_destination)->twilight_zenith_col = colour_from_string(key_val);
        return 0;
    } else if (strcmp(key, "twilight_horizon_col") == 0) {
        //! twilight_horizon_col - The colour to use to shade twilight at the horizon
        (*settings_destination)->twilight_horizon_col = colour_from_string(key_val);
        return 0;
    } else if (strcmp(key, "font_size") == 0) {
        //! font_size - A normalisation factor to apply to the font size of all text (default 1.0)
        CHECK_VALUE_NUMERIC("font_size")
        (*settings_destination)->font_size = key_val_num;
        return 0;
    } else if (strcmp(key, "copyright_gap") == 0) {
        //! copyright_gap - Spacing of the copyright text beneath the plot
        CHECK_VALUE_NUMERIC("copyright_gap")
        (*settings_destination)->copyright_gap = key_val_num;
        return 0;
    } else if (strcmp(key, "copyright_gap_2") == 0) {
        //! copyright_gap_2 - Spacing of the copyright text beneath the plot
        CHECK_VALUE_NUMERIC("copyright_gap_2")
        (*settings_destination)->copyright_gap_2 = key_val_num;
        return 0;
    } else if (strcmp(key, "constellation_stick_col") == 0) {
        //! constellation_stick_col - Colour to use when drawing constellation stick figures
        (*settings_destination)->constellation_stick_col = colour_from_string(key_val);
        return 0;
    } else if (strcmp(key, "grid_col") == 0) {
        //! grid_col - Colour to use when drawing grid of RA/Dec lines
        (*settings_destination)->grid_col = colour_from_string(key_val);
        return 0;
    } else if (strcmp(key, "constellation_boundary_col") == 0) {
        //! constellation_boundary_col - Colour to use when drawing constellation boundaries
        (*settings_destination)->constellation_boundary_col = colour_from_string(key_val);
        return 0;
    } else if (strcmp(key, "ephemeris_col") == 0) {
        //! ephemeris_col - Colours to use when drawing ephemerides for solar system objects. If this setting is
        //! supplied multiple times, then the list of supplied colours are used in a cyclic loop for all the solar
        //! system objects to be drawn.
        if ((*settings_destination)->ephemeris_col_custom_count > N_TRACES_MAX - 4) {
            snprintf(temp_err_string, FNAME_LENGTH,
                     "Bad input file. Too many entries for <ephemeris_col>.");
            stch_error(temp_err_string);
            return 1;
        }
        (*settings_destination)->ephemeris_col[(*settings_destination)->ephemeris_col_custom_count] =
                colour_from_string(key_val);
        (*settings_destination)->ephemeris_col_custom_count++;
        return 0;
    } else if (strcmp(key, "ephemeris_label_col") == 0) {
        //! ephemeris_label_col - Colours to use when labelling ephemerides for solar system objects. If this setting
        //! is supplied multiple times, then the list of supplied colours are used in a cyclic loop for all the solar
        //! system objects to be drawn.
        if ((*settings_destination)->ephemeris_label_col_custom_count > N_TRACES_MAX - 4) {
            snprintf(temp_err_string, FNAME_LENGTH,
                     "Bad input file. Too many entries for <ephemeris_label_col>.");
            stch_error(temp_err_string);
            return 1;
        }
        (*settings_destination)->ephemeris_label_col[(*settings_destination)->ephemeris_label_col_custom_count] =
                colour_from_string(key_val);
        (*settings_destination)->ephemeris_label_col_custom_count++;
        return 0;
    } else if (strcmp(key, "dso_cluster_col") == 0) {
        //! dso_cluster_col - Colour to use when drawing star clusters
        (*settings_destination)->dso_cluster_col = colour_from_string(key_val);
        return 0;
    } else if (strcmp(key, "dso_galaxy_col") == 0) {
        //! dso_galaxy_col - Colour to use when drawing galaxies
        (*settings_destination)->dso_galaxy_col = colour_from_string(key_val);
        return 0;
    } else if (strcmp(key, "dso_nebula_col") == 0) {
        //! dso_nebula_col - Colour to use when drawing nebulae
        (*settings_destination)->dso_nebula_col = colour_from_string(key_val);
        return 0;
    } else if (strcmp(key, "dso_label_col") == 0) {
        //! dso_label_col - Colour to use when labelling deep sky objects
        (*settings_destination)->dso_label_col = colour_from_string(key_val);
        return 0;
    } else if (strcmp(key, "dso_outline_col") == 0) {
        //! dso_outline_col - Colour to use when drawing the outline of deep sky objects
        (*settings_destination)->dso_outline_col = colour_from_string(key_val);
        return 0;
    } else if (strcmp(key, "constellation_label_col") == 0) {
        //! constellation_label_col - Colour to use when writing constellation names
        (*settings_destination)->constellation_label_col = colour_from_string(key_val);
        return 0;
    } else if (strcmp(key, "zodiacal_only") == 0) {
        //! zodiacal_only - Boolean (0 or 1) indicating whether we plot only the zodiacal constellations
        CHECK_VALUE_NUMERIC("zodiacal_only")
        (*settings_destination)->zodiacal_only = (int) key_val_num;
        return 0;
    } else if (strcmp(key, "projection") == 0) {
        //! projection - Select projection to use. Set to stereographic, flat, peters, gnomonic, sphere or alt_az.
        if (strcmp(key_val, "flat") == 0) {
            (*settings_destination)->projection = SW_PROJECTION_FLAT;
        } else if (strcmp(key_val, "peters") == 0) {
            (*settings_destination)->projection = SW_PROJECTION_PETERS;
            (*settings_destination)->aspect = 4 / (2 * M_PI);
            (*settings_destination)->angular_width = 360.;
        } else if (strcmp(key_val, "gnomonic") == 0) {
            (*settings_destination)->projection = SW_PROJECTION_GNOMONIC;
        } else if (strcmp(key_val, "stereographic") == 0) {
            (*settings_destination)->projection = SW_PROJECTION_STEREOGRAPHIC;
        } else if (strcmp(key_val, "sphere") == 0) {
            (*settings_destination)->projection = SW_PROJECTION_SPHERICAL;
            (*settings_destination)->aspect = 1.;
            (*settings_destination)->angular_width = 180.;
        } else if (strcmp(key_val, "alt_az") == 0) {
            (*settings_destination)->projection = SW_PROJECTION_ALTAZ;
            (*settings_destination)->aspect = 1.;
            (*settings_destination)->angular_width = 180.;
        } else {
            snprintf(temp_err_string, FNAME_LENGTH,
                     "Bad input file. projection should equal 'stereographic', 'peters', 'flat', "
                     "'gnomonic', 'sphere' or 'alt_az'.");
            stch_error(temp_err_string);
            return 1;
        }
        return 0;
    } else if (strcmp(key, "coords") == 0) {
        //! coords - Select which celestial coordinate system to use when specifying the centre of the plot.
        //! Set to 'ra_dec', 'galactic' or 'alt_az'.
        if (strcmp(key_val, "ra_dec") == 0) (*settings_destination)->coords = SW_COORDS_RADEC;
        else if (strcmp(key_val, "galactic") == 0) (*settings_destination)->coords = SW_COORDS_GALACTIC;
        else if (strcmp(key_val, "alt_az") == 0) (*settings_destination)->coords = SW_COORDS_ALTAZ;
        else {
            snprintf(temp_err_string, FNAME_LENGTH,
                     "Bad input file. coords should equal 'ra_dec', 'galactic' or 'alt_az'.");
            stch_error(temp_err_string);
            return 1;
        }
        return 0;
    } else if (strcmp(key, "grid_coords") == 0) {
        //! grid_coords - Select which celestial coordinate system to trace with grid lines.
        //! Set to 'ra_dec', 'galactic' or 'alt_az'.
        if (strcmp(key_val, "ra_dec") == 0) (*settings_destination)->grid_coords = SW_COORDS_RADEC;
        else if (strcmp(key_val, "galactic") == 0) (*settings_destination)->grid_coords = SW_COORDS_GALACTIC;
        else if (strcmp(key_val, "alt_az") == 0) (*settings_destination)->grid_coords = SW_COORDS_ALTAZ;
        else {
            snprintf(temp_err_string, FNAME_LENGTH,
                     "Bad input file. grid_coords should equal 'ra_dec', 'galactic' or 'alt_az'.");
            stch_error(temp_err_string);
            return 1;
        }
        return 0;
    } else if (strcmp(key, "angular_width") == 0) {
        //! angular_width - The angular width of the star chart on the sky, degrees
        CHECK_VALUE_NUMERIC("angular_width")
        (*settings_destination)->angular_width = key_val_num;
        return 0;
    } else if (strcmp(key, "width") == 0) {
        //! width - The width of the star chart, in cm
        CHECK_VALUE_NUMERIC("width")
        (*settings_destination)->width = key_val_num;
        return 0;
    } else if (strcmp(key, "aspect") == 0) {
        //! aspect - The aspect ratio of the star chart: i.e. the ratio height/width
        CHECK_VALUE_NUMERIC("aspect")
        (*settings_destination)->aspect = key_val_num;
        return 0;
    } else if (strcmp(key, "show_grid_lines") == 0) {
        //! show_grid_lines - Boolean (0 or 1) indicating whether we draw grid lines in the background of
        //! the star chart. The grid lines are either RA/Dec, galactic coordinates, or alt/az, depending on the
        //! value of <coords>.
        CHECK_VALUE_NUMERIC("show_grid_lines")
        (*settings_destination)->show_grid_lines = (int) key_val_num;
        return 0;
    } else if (strcmp(key, "x_label_slant") == 0) {
        //! x_label_slant - A slant to apply to all labels on the horizontal axes
        CHECK_VALUE_NUMERIC("x_label_slant")
        (*settings_destination)->x_label_slant = key_val_num;
        return 0;
    } else if (strcmp(key, "y_label_slant") == 0) {
        //! y_label_slant - A slant to apply to all labels on the vertical axes
        CHECK_VALUE_NUMERIC("y_label_slant")
        (*settings_destination)->y_label_slant = key_val_num;
        return 0;
    } else if (strcmp(key, "constellation_boundaries") == 0) {
        //! constellation_boundaries - Boolean (0 or 1) indicating whether we draw constellation boundaries
        CHECK_VALUE_NUMERIC("constellation_boundaries")
        (*settings_destination)->constellation_boundaries = (int) key_val_num;
        return 0;
    } else if (strcmp(key, "constellation_sticks") == 0) {
        //! constellation_sticks - Boolean (0 or 1) indicating whether we draw constellation stick figures
        CHECK_VALUE_NUMERIC("constellation_sticks")
        (*settings_destination)->constellation_sticks = (int) key_val_num;
        return 0;
    } else if (strcmp(key, "constellation_stick_design") == 0) {
        //! constellation_stick_design - Select which design of constellation stick figures we should draw. Set to
        //! either 'simplified' or 'rey'. See <https://github.com/dcf21/constellation-stick-figures> for more
        //! information.
        if (strcmp(key_val, "simplified") == 0) {
            (*settings_destination)->constellation_stick_design = SW_STICKS_SIMPLIFIED;
        } else if (strcmp(key_val, "rey") == 0) {
            (*settings_destination)->constellation_stick_design = SW_STICKS_REY;
        } else {
            snprintf(temp_err_string, FNAME_LENGTH, "Bad input file. "
                                                    "constellation_stick_design should equal 'simplified' or 'rey'.");
            stch_error(temp_err_string);
            return 1;
        }
        return 0;
    } else if (strcmp(key, "constellation_highlight") == 0) {
        //! constellation_highlight - Optionally highlight the boundary of a particular constellation, referenced
        //! by its three-letter abbreviation
        snprintf((*settings_destination)->constellation_highlight, 6, "%s", key_val);
        (*settings_destination)->constellation_highlight[6] = '\0';
        return 0;
    } else if (strcmp(key, "plot_stars") == 0) {
        //! plot_stars - Boolean (0 or 1) indicating whether we plot any stars
        CHECK_VALUE_NUMERIC("plot_stars")
        (*settings_destination)->plot_stars = (int) key_val_num;
        return 0;
    } else if (strcmp(key, "messier_only") == 0) {
        //! messier_only - Boolean (0 or 1) indicating whether we plot only Messier objects and not other DSOs
        CHECK_VALUE_NUMERIC("messier_only")
        (*settings_destination)->messier_only = (int) key_val_num;
        return 0;
    } else if (strcmp(key, "plot_dso") == 0) {
        //! plot_dso - Boolean (0 or 1) indicating whether we plot any deep sky objects
        CHECK_VALUE_NUMERIC("plot_dso")
        (*settings_destination)->plot_dso = (int) key_val_num;
        return 0;
    } else if (strcmp(key, "constellation_names") == 0) {
        //! constellation_names - Boolean (0 or 1) indicating whether we label the names of constellations
        CHECK_VALUE_NUMERIC("constellation_names")
        (*settings_destination)->constellation_names = (int) key_val_num;
        return 0;
    } else if (strcmp(key, "star_names") == 0) {
        //! star_names - Boolean (0 or 1) indicating whether we label the English names of stars
        CHECK_VALUE_NUMERIC("star_names")
        (*settings_destination)->star_names = (int) key_val_num;
        return 0;
    } else if (strcmp(key, "star_catalogue_numbers") == 0) {
        //! star_catalogue_numbers - Boolean (0 or 1) indicating whether we label the catalogue numbers of stars
        CHECK_VALUE_NUMERIC("star_catalogue_numbers")
        (*settings_destination)->star_catalogue_numbers = (int) key_val_num;
        return 0;
    } else if (strcmp(key, "star_bayer_labels") == 0) {
        //! star_bayer_labels - Boolean (0 or 1) indicating whether we label the Bayer numbers of stars
        CHECK_VALUE_NUMERIC("star_bayer_labels")
        (*settings_destination)->star_bayer_labels = (int) key_val_num;
        return 0;
    } else if (strcmp(key, "star_flamsteed_labels") == 0) {
        //! star_flamsteed_labels - Boolean (0 or 1) indicating whether we label the Flamsteed designations of stars
        CHECK_VALUE_NUMERIC("star_flamsteed_labels")
        (*settings_destination)->star_flamsteed_labels = (int) key_val_num;
        return 0;
    } else if (strcmp(key, "star_variable_labels") == 0) {
        //! star_variable_labels - Boolean (0 or 1) indicating whether we label the variable-star designations of stars
        CHECK_VALUE_NUMERIC("star_variable_labels")
        (*settings_destination)->star_variable_labels = (int) key_val_num;
        return 0;
    } else if (strcmp(key, "star_allow_multiple_labels") == 0) {
        //! star_allow_multiple_labels - Boolean (0 or 1) indicating whether we allow multiple labels next to a single star
        CHECK_VALUE_NUMERIC("star_allow_multiple_labels")
        (*settings_destination)->star_allow_multiple_labels = (int) key_val_num;
        return 0;
    } else if (strcmp(key, "star_catalogue") == 0) {
        //! star_catalogue - Select the star catalogue to use when showing the catalogue numbers of stars. Set to
        //! 'hipparcos', 'ybsc' or 'hd'.
        if (strcmp(key_val, "hipparcos") == 0) (*settings_destination)->star_catalogue = SW_CAT_HIP;
        else if (strcmp(key_val, "ybsc") == 0) (*settings_destination)->star_catalogue = SW_CAT_YBSC;
        else if (strcmp(key_val, "hd") == 0) (*settings_destination)->star_catalogue = SW_CAT_HD;
        else {
            snprintf(temp_err_string, FNAME_LENGTH,
                     "Bad input file. star_catalogue should equal 'hipparcos', 'ybsc' or 'hd'.");
            stch_error(temp_err_string);
            return 1;
        }
        return 0;
    } else if (strcmp(key, "star_mag_labels") == 0) {
        //! star_mag_labels - Boolean (0 or 1) indicating whether we label the magnitudes of stars
        CHECK_VALUE_NUMERIC("star_mag_labels")
        (*settings_destination)->star_mag_labels = (int) key_val_num;
        return 0;
    } else if (strcmp(key, "star_label_mag_min") == 0) {
        //! star_label_mag_min - Do not label stars fainter than this magnitude limit
        CHECK_VALUE_NUMERIC("star_label_mag_min")
        (*settings_destination)->star_label_mag_min = key_val_num;
        return 0;
    } else if (strcmp(key, "dso_label_mag_min") == 0) {
        //! dso_label_mag_min - Do not label DSOs fainter than this magnitude limit
        CHECK_VALUE_NUMERIC("dso_label_mag_min")
        (*settings_destination)->dso_label_mag_min = key_val_num;
        return 0;
    } else if (strcmp(key, "dso_names") == 0) {
        //! dso_names - Boolean (0 or 1) indicating whether we label the names of deep sky objects
        CHECK_VALUE_NUMERIC("dso_names")
        (*settings_destination)->dso_names = (int) key_val_num;
        return 0;
    } else if (strcmp(key, "dso_mags") == 0) {
        //! dso_mags - Boolean (0 or 1) indicating whether we label the magnitudes of deep sky objects
        CHECK_VALUE_NUMERIC("dso_mags")
        (*settings_destination)->dso_mags = (int) key_val_num;
        return 0;
    } else if (strcmp(key, "dso_mag_min") == 0) {
        //! dso_mag_min - Only show deep sky objects down to this faintest magnitude
        CHECK_VALUE_NUMERIC("dso_mag_min")
        (*settings_destination)->dso_mag_min = key_val_num;
        return 0;
    } else if (strcmp(key, "mag_min") == 0) {
        //! mag_min - The faintest magnitude of star which we draw
        CHECK_VALUE_NUMERIC("mag_min")
        (*settings_destination)->mag_min = key_val_num;
        (*settings_destination)->mag_min_automatic = 0;
        return 0;
    } else if (strcmp(key, "mag_max") == 0) {
        //! mag_max - Used to regulate the size of stars. A star of this magnitude is drawn with size mag_size_norm.
        //! Also, this is the brightest magnitude of star which is shown in the magnitude key below the chart.
        CHECK_VALUE_NUMERIC("mag_max")
        (*settings_destination)->mag_max = key_val_num;
        return 0;
    } else if (strcmp(key, "mag_step") == 0) {
        //! mag_step - The magnitude interval between the samples shown on the magnitude key under the chart
        CHECK_VALUE_NUMERIC("mag_step")
        (*settings_destination)->mag_step = key_val_num;
        return 0;
    } else if (strcmp(key, "mag_alpha") == 0) {
        //! mag_alpha - The multiplicative scaling factor to apply to the radii of stars differing in magnitude by
        //! one <mag_step>
        CHECK_VALUE_NUMERIC("mag_alpha")
        (*settings_destination)->mag_alpha = key_val_num;
        return 0;
    } else if (strcmp(key, "mag_size_norm") == 0) {
        //! mag_size_norm - The radius of a star of magnitude <mag_max> (default 1.0)
        CHECK_VALUE_NUMERIC("mag_size_norm")
        (*settings_destination)->mag_size_norm = key_val_num;
        return 0;
    } else if (strcmp(key, "maximum_star_count") == 0) {
        //! maximum_star_count - The maximum number of stars to draw. If this is exceeded, only the brightest stars
        //! are shown.
        CHECK_VALUE_NUMERIC("maximum_star_count")
        (*settings_destination)->maximum_star_count = (int) key_val_num;
        return 0;
    } else if (strcmp(key, "maximum_star_label_count") == 0) {
        //! maximum_star_label_count - The maximum number of stars which may be labelled
        CHECK_VALUE_NUMERIC("maximum_star_label_count")
        (*settings_destination)->maximum_star_label_count = (int) key_val_num;
        return 0;
    } else if (strcmp(key, "maximum_dso_count") == 0) {
        //! maximum_dso_count - The maximum number of DSOs to draw. If this is exceeded, only the brightest objects
        //! are shown.
        CHECK_VALUE_NUMERIC("maximum_dso_count")
        (*settings_destination)->maximum_dso_count = (int) key_val_num;
        return 0;
    } else if (strcmp(key, "maximum_dso_label_count") == 0) {
        //! maximum_dso_label_count - The maximum number of DSOs which may be labelled
        CHECK_VALUE_NUMERIC("maximum_dso_label_count")
        (*settings_destination)->maximum_dso_label_count = (int) key_val_num;
        return 0;
    } else if (strcmp(key, "plot_ecliptic") == 0) {
        //! plot_ecliptic - Boolean (0 or 1) indicating whether to draw a line along the ecliptic
        CHECK_VALUE_NUMERIC("plot_ecliptic")
        (*settings_destination)->plot_ecliptic = (int) key_val_num;
        return 0;
    } else if (strcmp(key, "label_ecliptic") == 0) {
        //! label_ecliptic - Boolean (0 or 1) indicating whether to label the months along the ecliptic, showing
        //! the Sun's annual progress
        CHECK_VALUE_NUMERIC("label_ecliptic")
        (*settings_destination)->label_ecliptic = (int) key_val_num;
        return 0;
    } else if (strcmp(key, "plot_galactic_plane") == 0) {
        //! plot_galactic_plane - Boolean (0 or 1) indicating whether to draw a line along the galactic plane
        CHECK_VALUE_NUMERIC("plot_galactic_plane")
        (*settings_destination)->plot_galactic_plane = (int) key_val_num;
        return 0;
    } else if (strcmp(key, "plot_equator") == 0) {
        //! plot_equator - Boolean (0 or 1) indicating whether to draw a line along the equator
        CHECK_VALUE_NUMERIC("plot_equator")
        (*settings_destination)->plot_equator = (int) key_val_num;
        return 0;
    } else if (strcmp(key, "ecliptic_col") == 0) {
        //! ecliptic_col - Colour to use when drawing a line along the ecliptic
        (*settings_destination)->ecliptic_col = colour_from_string(key_val);
        return 0;
    } else if (strcmp(key, "galactic_plane_col") == 0) {
        //! galactic_plane_col - Colour to use when drawing a line along the galactic plane
        (*settings_destination)->galactic_plane_col = colour_from_string(key_val);
        return 0;
    } else if (strcmp(key, "equator_col") == 0) {
        //! equator_col - Colour to use when drawing a line along the equator
        (*settings_destination)->equator_col = colour_from_string(key_val);
        return 0;
    } else if (strcmp(key, "plot_galaxy_map") == 0) {
        //! plot_galaxy_map - Boolean (0 or 1) indicating whether to draw a shaded map of the Milky Way behind
        //! the star chart
        CHECK_VALUE_NUMERIC("plot_galaxy_map")
        (*settings_destination)->plot_galaxy_map = (int) key_val_num;
        return 0;
    } else if (strcmp(key, "galaxy_map_width_pixels") == 0) {
        //! galaxy_map_width_pixels - The number of horizontal pixels across the shaded map of the Milky Way
        CHECK_VALUE_NUMERIC("galaxy_map_width_pixels")
        (*settings_destination)->galaxy_map_width_pixels = (int) key_val_num;
        return 0;
    } else if (strcmp(key, "galaxy_col") == 0) {
        //! galaxy_col - The colour to use to shade the bright parts of the map of the Milky Way
        (*settings_destination)->galaxy_col = colour_from_string(key_val);
        return 0;
    } else if (strcmp(key, "galaxy_col0") == 0) {
        //! galaxy_col0 - The colour to use to shade the dark parts of the map of the Milky Way
        (*settings_destination)->galaxy_col0 = colour_from_string(key_val);
        return 0;
    } else if (strcmp(key, "star_col") == 0) {
        //! star_col - Colour to use when drawing stars
        (*settings_destination)->star_col = colour_from_string(key_val);
        return 0;
    } else if (strcmp(key, "star_label_col") == 0) {
        //! star_label_col - Colour to use when labelling stars
        (*settings_destination)->star_label_col = colour_from_string(key_val);
        return 0;
    } else if (strcmp(key, "magnitude_key") == 0) {
        //! magnitude_key - Boolean (0 or 1) indicating whether to draw a key to the magnitudes of stars under
        //! the star chart
        CHECK_VALUE_NUMERIC("magnitude_key")
        (*settings_destination)->magnitude_key = (int) key_val_num;
        return 0;
    } else if (strcmp(key, "great_circle_key") == 0) {
        //! great_circle_key - Boolean (0 or 1) indicating whether to draw a key to the great circles under the
        //! star chart
        CHECK_VALUE_NUMERIC("great_circle_key")
        (*settings_destination)->great_circle_key = (int) key_val_num;
        return 0;
    } else if (strcmp(key, "dso_symbol_key") == 0) {
        //! dso_symbol_key - Boolean (0 or 1) indicating whether to draw a key to the deep sky object symbols
        CHECK_VALUE_NUMERIC("dso_symbol_key")
        (*settings_destination)->dso_symbol_key = (int) key_val_num;
        return 0;
    } else if (strcmp(key, "cardinals") == 0) {
        //! cardinals - Boolean (0 or 1) indicating whether to write the cardinal points around the edge of
        //! alt/az star charts
        CHECK_VALUE_NUMERIC("cardinals")
        (*settings_destination)->cardinals = (int) key_val_num;
        return 0;
    } else if (strcmp(key, "label_font_size_scaling") == 0) {
        //! label_font_size_scaling - Scaling factor to be applied to the font size of all star and DSO labels
        CHECK_VALUE_NUMERIC("label_font_size_scaling")
        (*settings_destination)->label_font_size_scaling = key_val_num;
        return 0;
    } else if (strcmp(key, "draw_ephemeris") == 0) {
        //! draw_ephemeris - Definitions of ephemerides to draw. Each definition should take the form of:
        //! `<bodyId>,<jdMin>,<jdMax>` where bodyId is an object ID string, and jdMin and jdMax are Julian dates
        //! If multiple ephemerides are to be drawn on a single chart, this argument should be specified multiple
        //! times, once for each ephemeris that is to be drawn.
        if ((*settings_destination)->ephemeris_custom_count > N_TRACES_MAX - 4) {
            snprintf(temp_err_string, FNAME_LENGTH,
                     "Bad input file. Too many entries for <draw_ephemeris>.");
            stch_error(temp_err_string);
            return 1;
        }
        strcpy((*settings_destination)->ephemeris_definitions[(*settings_destination)->ephemeris_custom_count],
               key_val);
        (*settings_destination)->ephemeris_custom_count++;
        return 0;
    } else if (strcmp(key, "ephemeris_epochs") == 0) {
        //! ephemeris_epochs - List of JD time epochs for which we should create points along each solar system
        //! ephemeris. If empty, then points are created automatically. This list must have the same length as
        //! <ephemeris_epoch_labels>.
        if ((*settings_destination)->ephemeris_epochs_custom_count > N_TRACES_MAX - 4) {
            snprintf(temp_err_string, FNAME_LENGTH,
                     "Bad input file. Too many entries for <ephemeris_epochs>.");
            stch_error(temp_err_string);
            return 1;
        }
        strcpy((*settings_destination)->ephemeris_epochs[(*settings_destination)->ephemeris_epochs_custom_count],
               key_val);
        (*settings_destination)->ephemeris_epochs_custom_count++;
        return 0;
    } else if (strcmp(key, "ephemeris_epoch_labels") == 0) {
        //! ephemeris_epoch_labels - List of text labels for the points we create along each solar system
        //! ephemeris. If empty, then points are created automatically. This list must have the same length as
        //! <ephemeris_epochs>.
        if ((*settings_destination)->ephemeris_epoch_labels_custom_count > N_TRACES_MAX - 4) {
            snprintf(temp_err_string, FNAME_LENGTH,
                     "Bad input file. Too many entries for <ephemeris_epoch_labels>.");
            stch_error(temp_err_string);
            return 1;
        }
        strcpy((*settings_destination)->ephemeris_epoch_labels[(*settings_destination)->ephemeris_epoch_labels_custom_count],
               key_val);
        (*settings_destination)->ephemeris_epoch_labels_custom_count++;
        return 0;
    } else if (strcmp(key, "ephemeris_style") == 0) {
        //! ephemeris_style - Select the style to use when showing the tracks of solar system objects.
        //! Set to `track`, `side_by_side` or `side_by_side_with_track`.
        if (strcmp(key_val, "track") == 0) {
            (*settings_destination)->ephemeris_style = SW_EPHEMERIS_TRACK;
        } else if (strcmp(key_val, "side_by_side") == 0) {
            (*settings_destination)->ephemeris_style = SW_EPHEMERIS_SIDE_BY_SIDE;
        } else if (strcmp(key_val, "side_by_side_with_track") == 0) {
            (*settings_destination)->ephemeris_style = SW_EPHEMERIS_SIDE_BY_SIDE_WITH_TRACK;
        } else if (strcmp(key_val, "side_by_side_with_arrow") == 0) {
            (*settings_destination)->ephemeris_style = SW_EPHEMERIS_SIDE_BY_SIDE_WITH_ARROW;
        } else {
            snprintf(temp_err_string, FNAME_LENGTH,
                     "Bad input file. ephemeris_style should equal `track`, `side_by_side` or `side_by_side_with_track`.");
            stch_error(temp_err_string);
            return 1;
        }
        return 0;
    } else if (strcmp(key, "ephemeris_autoscale") == 0) {
        //! ephemeris_autoscale - Boolean (0 or 1) indicating whether to auto-scale the star chart to contain the
        //! requested ephemerides. This overrides settings for ra_central, dec_central and angular_width.
        CHECK_VALUE_NUMERIC("ephemeris_autoscale")
        (*settings_destination)->ephemeris_autoscale = (int) key_val_num;
        return 0;
    } else if (strcmp(key, "ephemeris_table") == 0) {
        //! ephemeris_table - Boolean (0 or 1) indicating whether to include a table of the object's magnitude
        CHECK_VALUE_NUMERIC("ephemeris_table")
        (*settings_destination)->ephemeris_table = (int) key_val_num;
        return 0;
    } else if (strcmp(key, "must_show_all_ephemeris_labels") == 0) {
        //! ephemeris_autoscale - Boolean (0 or 1) indicating whether we must show all ephemeris text labels,
        //! even if they collide with other text.
        CHECK_VALUE_NUMERIC("must_show_all_ephemeris_labels")
        (*settings_destination)->must_show_all_ephemeris_labels = (int) key_val_num;
        return 0;
    } else if (strcmp(key, "ephemeris_compute_path") == 0) {
        //! ephemeris_compute_path - The path to the tool <ephemerisCompute>, used to compute paths for solar
        //! system objects. See <https://github.com/dcf21/ephemeris-compute-de430>. If this tool is installed in the
        //! same directory as StarCharter, the default value should be <../ephemeris-compute-de430/bin/ephem.bin>.
        strcpy((*settings_destination)->ephemeris_compute_path, key_val);
        return 0;
    } else if (strcmp(key, "scale_bar") == 0) {
        //! scale_bar - List of scale bars we should super-impose over the star chart. Each should be specified as:
        //! <x_pos>,<y_pos>,<position_angle>,<degrees>
        //! Where <x_pos> and <y_pos> are 0-1, the position angle is a clockwise rotation in degrees, and degrees is
        //! the length of the scale bar on the sky.
        if ((*settings_destination)->scale_bars_custom_count > N_TRACES_MAX - 4) {
            snprintf(temp_err_string, FNAME_LENGTH,
                     "Bad input file. Too many entries for <scale_bar>.");
            stch_error(temp_err_string);
            return 1;
        }
        strcpy((*settings_destination)->scale_bars[(*settings_destination)->scale_bars_custom_count], key_val);
        (*settings_destination)->scale_bars_custom_count++;
        return 0;
    } else if (strcmp(key, "scale_bar_col") == 0) {
        //! scale_bar_col - Colour to use for scale bars.
        (*settings_destination)->scale_bar_colour = colour_from_string(key_val);
        return 0;
    } else if (strcmp(key, "font_family") == 0) {
        //! font_family - The font family to use when rendering all text labels.
        strcpy((*settings_destination)->font_family, key_val);
        return 0;
    } else if (strcmp(key, "great_circle_line_width") == 0) {
        //! great_circle_line_width - Line width to use when marking great circles on the sky (e.g. the equator
        //! and the ecliptic). Default 1.75.
        CHECK_VALUE_NUMERIC("great_circle_line_width")
        (*settings_destination)->great_circle_line_width = key_val_num;
        return 0;
    } else if (strcmp(key, "great_circle_dotted") == 0) {
        //! great_circle_dotted - Boolean (0 or 1)  indicating whether to use a dotted line when tracing great circles
        CHECK_VALUE_NUMERIC("great_circle_dotted")
        (*settings_destination)->great_circle_dotted = (int) key_val_num;
        return 0;
    } else if (strcmp(key, "grid_line_width") == 0) {
        //! grid_line_width - Line width to use when drawing grid lines. Default 1.3.
        CHECK_VALUE_NUMERIC("grid_line_width")
        (*settings_destination)->coordinate_grid_line_width = key_val_num;
        return 0;
    } else if (strcmp(key, "dso_point_size_scaling") == 0) {
        //! dso_point_size_scaling - Size scaling of deep sky object symbols. Default 1.
        CHECK_VALUE_NUMERIC("dso_point_size_scaling")
        (*settings_destination)->dso_point_size_scaling = key_val_num;
        return 0;
    } else if (strcmp(key, "constellations_capitalise") == 0) {
        //! constellations_capitalise - Boolean (0 or 1) indicating whether we capitalise the names of
        //! constellations.
        CHECK_VALUE_NUMERIC("constellations_capitalise")
        (*settings_destination)->constellations_capitalise = (int) key_val_num;
        return 0;
    } else if (strcmp(key, "constellations_label_shadow") == 0) {
        //! constellations_label_shadow - Boolean (0 or 1) indicating whether to draw a shadow behind the names
        //! of the constellations.
        CHECK_VALUE_NUMERIC("constellations_label_shadow")
        (*settings_destination)->constellations_label_shadow = (int) key_val_num;
        return 0;
    } else if (strcmp(key, "constellation_sticks_line_width") == 0) {
        //! constellation_sticks_line_width - Line width to use when drawing constellation stick figures. Default 1.4.
        CHECK_VALUE_NUMERIC("constellation_sticks_line_width")
        (*settings_destination)->constellation_sticks_line_width = key_val_num;
        return 0;
    } else if (strcmp(key, "chart_edge_line_width") == 0) {
        //! chart_edge_line_width - Line width to use when marking the edge of the chart. Default 2.5.
        CHECK_VALUE_NUMERIC("chart_edge_line_width")
        (*settings_destination)->chart_edge_line_width = key_val_num;
        return 0;
    } else {
        snprintf(temp_err_string, FNAME_LENGTH, "Bad input file. Unrecognised setting '%s'.", key);
        stch_error(temp_err_string);
        return 1;
    }
}

#pragma clang diagnostic pop

//! read_configuration_file - Read an input configuration file, line by line
//! \param [in] filename - The filename of the file to read, or NULL to read from stdin
//! \param [in] iteration_depth - The depth of iterative opening of included config files
//! \param [in|out] got_chart - Boolean indicating whether we have passed a "CHART" config block that we've not yet rendered.
//! \param [in] chart_defaults - chart_config data structure containing all our default chart settings.
//! \param [out] this_chart_config - The configuration for the chart described by the current "CHART" config block.
//! \param [in|out] settings_destination - The configuration structure we are currently reading settings into.
//! \return - Boolean; 0 means line was parsed without error; 1 means there was a fatal error

int read_configuration_file(const char *filename, const int iteration_depth,
                            int *got_chart, chart_config *chart_defaults,
                            chart_config *this_chart_config, chart_config **settings_destination) {
    int file_line_number = 0;
    FILE *infile;

    // Open the input configuration file
    if (filename != NULL) {
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
    while (!feof(infile)) {
        char line[LSTR_LENGTH];
        file_readline(infile, line);
        file_line_number++;

        // Process line of configuration file
        int status = process_configuration_file_line(line, filename, iteration_depth,
                                                     file_line_number, got_chart, chart_defaults,
                                                     this_chart_config, settings_destination);
        if (status) return 1;
    }

    // If we're reading from a file on disk, close the file handle
    if (filename != NULL) fclose(infile);

    // Exit with success flag
    return 0;
}
