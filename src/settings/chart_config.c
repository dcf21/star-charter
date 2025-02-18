// settings.c
// 
// -------------------------------------------------
// Copyright 2015-2025 Dominic Ford
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <gsl/gsl_math.h>

#include "coreUtils/errorReport.h"

#include "chart_config.h"

#include "astroGraphics/stars.h"
#include "mathsTools/julianDate.h"
#include "mathsTools/projection.h"
#include "mathsTools/sphericalTrig.h"

//! default_config - Populate a chart configuration data structure with a set of default configuration values
//! \param i - The data structure to be populated

void default_config(chart_config *i) {
    i->language = SW_LANG_ENGLISH;
    i->projection = SW_PROJECTION_STEREOGRAPHIC;
    i->coords = SW_COORDS_RADEC;
    i->grid_coords = SW_COORDS_RADEC;
    i->axis_label = 0;
    i->ra0 = 0.0;
    i->dec0 = 0.0;
    i->l0 = 0.0;
    i->b0 = 0.0;
    i->alt0 = 0.0;
    i->az0 = 0.0;
    i->position_angle = 0.0;
    i->text_labels_default_count = 0;
    i->text_labels_custom_count = 0;
    i->text_labels_final_count = 0;
    i->arrow_labels_default_count = 0;
    i->arrow_labels_custom_count = 0;
    i->arrow_labels_final_count = 0;
    i->show_horizon = 0;
    i->horizon_latitude = 0;
    i->horizon_longitude = 0;
    i->horizon_cardinal_points_marker_colour = (colour) {0, 0, 0};
    i->horizon_cardinal_points_labels_colour = (colour) {0, 0, 0};
    i->horizon_cardinal_points_marker_size = 1;
    i->horizon_cardinal_points_marker_count = 8;
    i->horizon_cardinal_points_marker_elevate = 0;
    i->julian_date = 2451544.5;
    i->show_solar_system = 0;
    strcpy(i->solar_system_labels[0], "Mercury");
    strcpy(i->solar_system_labels[1], "Venus");
    strcpy(i->solar_system_labels[2], "Mars");
    strcpy(i->solar_system_labels[3], "Jupiter");
    strcpy(i->solar_system_labels[4], "Saturn");
    strcpy(i->solar_system_labels[5], "Uranus");
    strcpy(i->solar_system_labels[6], "Neptune");
    strcpy(i->solar_system_ids[0], "P1");
    strcpy(i->solar_system_ids[1], "P2");
    strcpy(i->solar_system_ids[2], "P4");
    strcpy(i->solar_system_ids[3], "P5");
    strcpy(i->solar_system_ids[4], "P6");
    strcpy(i->solar_system_ids[5], "P7");
    strcpy(i->solar_system_ids[6], "P8");
    i->solar_system_default_count = 7;
    i->solar_system_ids_custom_count = 0;
    i->solar_system_labels_custom_count = 0;
    i->solar_system_final_count = 0;
    i->solar_system_colour[0] = (colour) {1, 0.9, 0.75};
    i->solar_system_colour_default_count = 1;
    i->solar_system_colour_custom_count = 0;
    i->solar_system_colour_final_count = 0;
    i->solar_system_label_colour[0] = (colour) {1, 0.9, 0.75};
    i->solar_system_label_colour_default_count = 1;
    i->solar_system_label_colour_custom_count = 0;
    i->solar_system_label_colour_final_count = 0;
    i->solar_system_minimum_size = 999.;
    i->solar_system_show_moon_phase = 1;
    i->solar_system_sun_actual_size = 0;
    i->solar_system_sun_col = (colour) {1, 1, 0};
    i->solar_system_moon_earthshine_intensity = 0.12;
    i->solar_system_moon_colour = (colour) {1, 1, 0.8};
    i->solar_system_topocentric_correction = 0;
    i->shade_twilight = 0;
    i->shade_near_sun = 0;
    i->shade_not_observable = 0;
    i->twilight_zenith_col = (colour) {0.337, 0.547, 0.820};
    i->twilight_horizon_col = (colour) {0.506, 0.765, 0.929};
    i->font_size = 1.0;
    i->axis_ticks_value_only = 1;
    i->show_grid_lines = 1;
    i->grid_line_density = 1;
    i->x_label_slant = 0;
    i->y_label_slant = 0;
    i->constellation_boundaries = 1;
    i->constellation_sticks = 1;
    i->constellation_show_below_horizon = 0;
    i->constellation_stick_design = SW_STICKS_IAU;
    i->constellation_names = 1;
    i->constellations_label_size = 1;
    i->plot_stars = 1;
    i->messier_only = 0;
    i->plot_dso = 1;
    i->zodiacal_only = 0;
    i->star_names = 1;
    i->star_catalogue_numbers = 0;
    i->star_catalogue = SW_CAT_HIP;
    i->star_bayer_labels = 0;
    i->star_flamsteed_labels = 0;
    i->star_variable_labels = 0;
    i->star_allow_multiple_labels = 0;
    i->star_mag_labels = SW_MAG_LABEL_OFF;
    i->star_label_mag_min = 9999;
    i->star_clip_outline = 1;
    i->dso_style = SW_DSO_STYLE_COLOURED;
    i->dso_label_mag_min = 9999;
    i->dso_names = 1;
    i->dso_mags = 0;
    i->dso_mag_min = 14;
    i->must_label_all_dsos = 0;
    i->angular_width = 25.0;
    i->width = 16.5;
    i->aspect = 1.41421356;
    i->dpi = 0; // automatic
    i->ephemeris_default_count = 0;
    i->ephemeris_custom_count = 0;
    i->ephemeris_final_count = 0;
    i->ephemeris_epochs_default_count = 0;
    i->ephemeris_epochs_custom_count = 0;
    i->ephemeris_epochs_final_count = 0;
    i->ephemeris_epoch_labels_custom_count = 0;
    i->ephemeris_label_interval_default_count = 0;
    i->ephemeris_label_interval_custom_count = 0;
    i->ephemeris_label_interval_final_count = 0;
    i->scale_bars_default_count = 0;
    i->scale_bars_custom_count = 0;
    i->scale_bars_final_count = 0;
    i->scale_bar_colour = (colour) {0, 0, 0};
    i->ephemeris_autoscale = 0;
    i->ephemeris_coords = SW_COORDS_EPHEMERIS_RADEC;
    i->ephemeris_resolution = 0.5;
    i->ephemeris_table = 0;
    i->must_show_all_ephemeris_labels = 0;
    i->mag_min = 6.0;
    i->mag_max = 0.0;
    i->mag_step = 0.5;
    i->mag_alpha = 1.1727932;
    i->mag_size_norm = 1.0;
    i->mag_size_maximum_permitted = 1e4; // effectively no limit
    i->maximum_star_count = 1693;
    i->minimum_star_count = 0;
    i->maximum_star_label_count = 1000;
    i->maximum_dso_count = 500;
    i->maximum_dso_label_count = 100;
    i->copyright_gap = 0;
    i->copyright_gap_2 = 0;
    i->dso_cluster_col = (colour) {0.8, 0.8, 0.25};
    i->dso_galaxy_col = (colour) {0.75, 0.15, 0.15};
    i->dso_nebula_col = (colour) {0.25, 0.75, 0.25};
    i->dso_label_col = (colour) {0, 0, 0};
    i->dso_outline_col = (colour) {0.25, 0.25, 0.25};
    i->plot_ecliptic = 1;
    i->label_ecliptic = 0;
    i->plot_galactic_plane = 1;
    i->plot_equator = 1;
    i->ecliptic_col = (colour) {0.8, 0.65, 0};
    i->galactic_plane_col = (colour) {0, 0, 0.75};
    i->equator_col = (colour) {0.65, 0, 0.65};
    i->constellation_stick_col = (colour) {0, 0.6, 0};
    i->grid_col = (colour) {0.7, 0.7, 0.7};
    i->constellation_boundary_col = (colour) {0.5, 0.5, 0.5};
    i->ephemeris_col[0] = (colour) {0, 0, 0};
    i->ephemeris_col_default_count = 1;
    i->ephemeris_col_custom_count = 0;
    i->ephemeris_col_final_count = 0;
    i->ephemeris_arrow_col[0] = (colour) {0, 0, 0};
    i->ephemeris_arrow_col_default_count = 1;
    i->ephemeris_arrow_col_custom_count = 0;
    i->ephemeris_arrow_col_final_count = 0;
    i->ephemeris_label_col[0] = (colour) {0, 0, 0};
    i->ephemeris_label_col_default_count = 1;
    i->ephemeris_label_col_custom_count = 0;
    i->ephemeris_label_col_final_count = 0;
    i->constellation_label_col = (colour) {0.1, 0.1, 0.1};
    i->plot_galaxy_map = 1;
    i->galaxy_map_width_pixels = 2048;
    i->galaxy_col = (colour) {0.68, 0.76, 1};
    i->galaxy_col0 = (colour) {1, 1, 1};
    i->star_col = (colour) {0, 0, 0};
    i->star_label_col = (colour) {0, 0, 0};
    i->magnitude_key = 1;
    i->great_circle_key = 1;
    i->dso_symbol_key = 1;
    i->cardinals = 1;
    i->label_font_size_scaling = 1;
    i->output_multiple_pages = 0;
    strcpy(i->constellation_highlight, "---");
    strcpy(i->dso_catalogue_file, SRCDIR "../data/deepSky/ngcDistances/output/ngc_merged.txt");
    strcpy(i->galaxy_map_filename, SRCDIR "../data/milkyWay/process/output/galaxymap.dat");
    strcpy(i->photo_filename, "");
    strcpy(i->output_filename, "starchart.png");
    strcpy(i->copyright, "Produced with StarCharter. https://github.com/dcf21/star-charter");
    strcpy(i->title, "");

    i->show_horizon_zenith = 0;
    i->show_poles = 0;
    i->horizon_zenith_marker_size = 1;
    i->horizon_zenith_colour = (colour) {1, 1, 1};
    i->ephemeris_style = SW_EPHEMERIS_TRACK;
    i->ephemeris_show_arrow_shadow = 1;
    i->ephemeris_minimum_size = 3.;
    i->meteor_radiants_default_count = 0;
    i->meteor_radiants_custom_count = 0;
    i->meteor_radiants_final_count = 0;
    i->meteor_radiant_marker_size = 1;
    i->meteor_radiant_colour = (colour) {0.75, 0.75, 1};
    strcpy(i->font_family, "Ubuntu");
    i->great_circle_line_width = 1.75;
    i->great_circle_dotted = 0;
    i->coordinate_grid_line_width = 1.3;
    i->dso_point_size_scaling = 1;
    i->constellations_capitalise = 0;
    i->constellations_label_shadow = 1;
    i->constellations_sticks_line_width = 1.4;
    i->chart_edge_line_width = 2.5;

    // Boolean flags indicating which settings have been manually overridden
    i->mag_min_is_set = 0;
    i->dso_mag_min_is_set = 0;
    i->minimum_star_count_is_set = 0;  // not exposed
    i->ra0_is_set = 0;
    i->dec0_is_set = 0;
    i->star_flamsteed_labels_is_set = 0;
    i->star_bayer_labels_is_set = 0;
    i->constellation_names_is_set = 0;
    i->star_catalogue_is_set = 0;
    i->star_catalogue_numbers_is_set = 0;
    i->maximum_star_label_count_is_set = 0;
    i->projection_is_set = 0;
    i->angular_width_is_set = 0;
    i->dso_names_is_set = 0;
    i->aspect_is_set = 0;
    i->constellations_capitalise_is_set = 0;
    i->constellations_label_size_is_set = 0;

    // Pointers default to NULL
    i->ephemeris_data = NULL;
    i->solar_system_ephemeris_data = NULL;
    i->cairo_surface = NULL;
    i->cairo_draw = NULL;
}

//! config_init_arrays - Populate computed quantities into a chart configuration data structure.
//! Phase 1: finalise lengths of arrays.
//! \param i - The data structure to be populated

void config_init_arrays(chart_config *i) {
    // Update counts of number of items in lists
    // Number of ephemeris tracks to display
    if (i->ephemeris_custom_count > 0) {
        i->ephemeris_final_count = i->ephemeris_custom_count;
    } else {
        i->ephemeris_final_count = i->ephemeris_default_count;
    }

    // Number of text labels to display
    if (i->text_labels_custom_count > 0) {
        i->text_labels_final_count = i->text_labels_custom_count;
    } else {
        i->text_labels_final_count = i->text_labels_default_count;
    }

    // Number of arrow/line labels to display
    if (i->arrow_labels_custom_count > 0) {
        i->arrow_labels_final_count = i->arrow_labels_custom_count;
    } else {
        i->arrow_labels_final_count = i->arrow_labels_default_count;
    }

    // Number of meteor shower radiants to display
    if (i->meteor_radiants_custom_count > 0) {
        i->meteor_radiants_final_count = i->meteor_radiants_custom_count;
    } else {
        i->meteor_radiants_final_count = i->meteor_radiants_default_count;
    }

    // Number of solar-system objects to display at the epoch <julian_date>
    if (i->solar_system_ids_custom_count != i->solar_system_labels_custom_count) {
        snprintf(temp_err_string, FNAME_LENGTH,
                 "Bad input file. There are %d entries for <solar_system_ids>, but %d entries for "
                 "<solar_system_labels>. These numbers should be equal.",
                 i->solar_system_ids_custom_count, i->solar_system_labels_custom_count);
        stch_fatal(__FILE__, __LINE__, temp_err_string);
        exit(1);
    }

    if (i->solar_system_ids_custom_count > 0) {
        i->solar_system_final_count = i->solar_system_ids_custom_count;
    } else {
        i->solar_system_final_count = i->solar_system_default_count;
    }

    // Number of custom ephemeris time points in the lists <ephemeris_epochs> and <ephemeris_epoch_labels>
    if (i->ephemeris_epochs_custom_count != i->ephemeris_epoch_labels_custom_count) {
        snprintf(temp_err_string, FNAME_LENGTH,
                 "Bad input file. There are %d entries for <ephemeris_epochs>, but %d entries for "
                 "<ephemeris_epoch_labels>. These numbers should be equal.",
                 i->ephemeris_epochs_custom_count, i->ephemeris_epoch_labels_custom_count);
        stch_fatal(__FILE__, __LINE__, temp_err_string);
        exit(1);
    }

    if (i->ephemeris_epochs_custom_count > 0) {
        i->ephemeris_epochs_final_count = i->ephemeris_epochs_custom_count;
    } else {
        i->ephemeris_epochs_final_count = i->ephemeris_epochs_default_count;
    }

    if (i->ephemeris_label_interval_custom_count > 0) {
        i->ephemeris_label_interval_final_count = i->ephemeris_label_interval_custom_count;
    } else {
        i->ephemeris_label_interval_final_count = i->ephemeris_label_interval_default_count;
    }

    // Number of colour to use in cyclic loop when drawing solar system objects
    if (i->solar_system_colour_custom_count > 0) {
        i->solar_system_colour_final_count = i->solar_system_colour_custom_count;
    } else {
        i->solar_system_colour_final_count = i->solar_system_colour_default_count;
    }

    // Number of colour to use in cyclic loop when labelling solar system objects
    if (i->solar_system_label_colour_custom_count > 0) {
        i->solar_system_label_colour_final_count = i->solar_system_label_colour_custom_count;
    } else {
        i->solar_system_label_colour_final_count = i->solar_system_label_colour_default_count;
    }

    // Number of colour to use in cyclic loop for drawing ephemerides for solar system objects.
    if (i->ephemeris_col_custom_count > 0) {
        i->ephemeris_col_final_count = i->ephemeris_col_custom_count;
    } else {
        i->ephemeris_col_final_count = i->ephemeris_col_default_count;
    }

    // Number of colour to use in cyclic loop for drawing ephemeris arrows for solar system objects.
    if (i->ephemeris_arrow_col_custom_count > 0) {
        i->ephemeris_arrow_col_final_count = i->ephemeris_arrow_col_custom_count;
    } else {
        i->ephemeris_arrow_col_final_count = i->ephemeris_arrow_col_default_count;
    }

    // Number of colour to use in cyclic loop for labelling ephemerides for solar system objects.
    if (i->ephemeris_label_col_custom_count > 0) {
        i->ephemeris_label_col_final_count = i->ephemeris_label_col_custom_count;
    } else {
        i->ephemeris_label_col_final_count = i->ephemeris_label_col_default_count;
    }

    // Number of scale bars to display
    if (i->scale_bars_custom_count > 0) {
        i->scale_bars_final_count = i->scale_bars_custom_count;
    } else {
        i->scale_bars_final_count = i->scale_bars_default_count;
    }

    // Check that colour lists have at least one member
    if (i->solar_system_colour_final_count < 1) {
        char buffer[FNAME_LENGTH];
        snprintf(buffer, FNAME_LENGTH,
                 "Bad input file. There zero entries for <solar_system_col>, but this list must have at "
                 "least one member.");
        stch_fatal(__FILE__, __LINE__, buffer);
        exit(1);
    }

    if (i->solar_system_label_colour_final_count < 1) {
        char buffer[FNAME_LENGTH];
        snprintf(buffer, FNAME_LENGTH,
                 "Bad input file. There zero entries for <solar_system_label_col>, but this list must have at "
                 "least one member.");
        stch_fatal(__FILE__, __LINE__, buffer);
        exit(1);
    }

    if (i->ephemeris_col_final_count < 1) {
        char buffer[FNAME_LENGTH];
        snprintf(buffer, FNAME_LENGTH,
                 "Bad input file. There zero entries for <ephemeris_col>, but this list must have at "
                 "least one member.");
        stch_fatal(__FILE__, __LINE__, buffer);
        exit(1);
    }

    if (i->ephemeris_arrow_col_final_count < 1) {
        char buffer[FNAME_LENGTH];
        snprintf(buffer, FNAME_LENGTH,
                 "Bad input file. There zero entries for <ephemeris_arrow_col>, but this list must have at "
                 "least one member.");
        stch_fatal(__FILE__, __LINE__, buffer);
        exit(1);
    }

    if (i->ephemeris_label_col_final_count < 1) {
        char buffer[FNAME_LENGTH];
        snprintf(buffer, FNAME_LENGTH,
                 "Bad input file. There zero entries for <ephemeris_label_col>, but this list must have at "
                 "least one member.");
        stch_fatal(__FILE__, __LINE__, buffer);
        exit(1);
    }
}

//! config_init_pointing - Populate computed quantities into a chart configuration data structure.
//! Phase 2: finalise central coordinates of the plot.
//! \param i - The data structure to be populated

void config_init_pointing(chart_config *i) {
    // Calculate the central coordinates of the chart
    if ((i->coords == SW_COORDS_GALACTIC) && !i->ephemeris_autoscale) {
        // Convert galactic coordinates of centre of field of view into (RA, Dec) J2000
        convert_selected_coordinates_to_ra_dec(i, i->coords, i->l0 * M_PI / 180, i->b0 * M_PI / 180,
                                               &(i->ra0_final), &(i->dec0_final));

        // Fix position angle of plot
        const double ra_ngp_j2000 = 192.85948 * M_PI / 180;
        const double dec_ngp_j2000 = 27.12825 * M_PI / 180;

        // Work out the position angle of the galactic pole, counterclockwise from north, as measured at centre of frame
        // radians, for J2000 north pole
        const double zenith_position_angle = position_angle(
                i->ra0_final, i->dec0_final, ra_ngp_j2000, dec_ngp_j2000);

        // Apply fix
        if ((i->projection != SW_PROJECTION_FLAT) && (i->projection != SW_PROJECTION_PETERS)) {
            i->position_angle -= zenith_position_angle * 180 / M_PI;
        }
    } else if ((i->coords == SW_COORDS_ALTAZ) && !i->ephemeris_autoscale) {
        // Convert alt/az of centre of field of view into (RA, Dec) at epoch <julian_date>
        convert_selected_coordinates_to_ra_dec(i, i->coords, i->az0 * M_PI / 180, i->alt0 * M_PI / 180,
                                               &(i->ra0_final), &(i->dec0_final));

        // Fix position angle of plot
        double ra_zenith_at_epoch, dec_zenith_at_epoch;
        double ra_zenith_j2000, dec_zenith_j2000;
        get_zenith_position(i->horizon_latitude, i->horizon_longitude, i->julian_date,
                            &ra_zenith_at_epoch, &dec_zenith_at_epoch);
        ra_dec_to_j2000(ra_zenith_at_epoch, dec_zenith_at_epoch, i->julian_date,
                        &ra_zenith_j2000, &dec_zenith_j2000);

        // Work out the position angle of the zenith, counterclockwise from north, as measured at centre of frame
        // radians, for J2000 north pole
        const double zenith_position_angle = position_angle(
                i->ra0_final, i->dec0_final, ra_zenith_j2000, dec_zenith_j2000);

        // Apply fix
        if ((i->projection != SW_PROJECTION_FLAT) && (i->projection != SW_PROJECTION_PETERS)) {
            i->position_angle -= zenith_position_angle * 180 / M_PI;
        }
    } else {
        // Centre of field-of-view is already specified in (RA, Dec), so just convert units to radians
        convert_selected_coordinates_to_ra_dec(i, i->coords, i->ra0 * M_PI / 12, i->dec0 * M_PI / 180,
                                               &(i->ra0_final), &(i->dec0_final));
    }

    // Convert angular width in degrees to radians
    i->angular_width *= M_PI / 180;

    // Calculate the width of the field of view in the tangent plane
    if (i->projection == SW_PROJECTION_FLAT) {
        i->wlin = i->angular_width;
    } else if (i->projection == SW_PROJECTION_PETERS) {
        i->wlin = i->angular_width;
    } else if (i->projection == SW_PROJECTION_GNOMONIC) {
        i->wlin = 2 * tan(i->angular_width / 2);
    } else if (i->projection == SW_PROJECTION_STEREOGRAPHIC) {
        i->wlin = 2 * tan(i->angular_width / 4);
    } else if (i->projection == SW_PROJECTION_MULTILATITUDE) {
        i->wlin = i->angular_width;
    } else if (i->projection == SW_PROJECTION_SPHERICAL) {
        i->wlin = 2 * sin(i->angular_width / 2) * 1.12; // margin specified here
    } else if (i->projection == SW_PROJECTION_ALTAZ) {
        i->wlin = i->angular_width / (M_PI / 2) * 1.12; // margin specified here
    }

    // Convert angular field-of-view into min-max bounding-box in the tangent plane
    i->x_min = -i->wlin / 2;
    i->x_max = i->wlin / 2;
    i->y_min = -i->wlin / 2 * i->aspect;
    i->y_max = i->wlin / 2 * i->aspect;

    // Tweak magnitude limits to restrict maximum number of stars visible
    tweak_magnitude_limits(i);
    i->mag_highest = i->mag_max;
}

//! config_close - Free up a chart configuration data structure

void config_close(chart_config *i) {
}
