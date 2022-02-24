// settings.c
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#include <gsl/gsl_math.h>

#include "chart_config.h"

#include "astroGraphics/stars.h"

void default_config(chart_config *i) {
    i->language = SW_LANG_EN;
    i->projection = SW_PROJECTION_GNOM;
    i->coords = SW_COORDS_RADEC;
    i->axis_label = 0;
    i->ra0 = 0.0;
    i->dec0 = 0.0;
    i->position_angle = 0.0;
    i->font_size = 1.0;
    i->axis_ticks_value_only = 1;
    i->ra_dec_lines = 1;
    i->x_label_slant = 0;
    i->y_label_slant = 0;
    i->constellation_boundaries = 1;
    i->constellation_sticks = 1;
    i->constellation_stick_design = SW_STICKS_SIMPLIFIED;
    i->constellation_names = 1;
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
    i->star_mag_labels = 0;
    i->star_label_mag_min = 9999;
    i->dso_label_mag_min = 9999;
    i->dso_names = 1;
    i->dso_mags = 0;
    i->dso_mag_min = 14;
    i->angular_width = 25.0;
    i->width = 16.5;
    i->aspect = 1.41421356;
    i->ephemeride_count = 0;
    i->ephemeris_autoscale = 0;
    i->ephemeris_table = 0;
    i->must_show_all_ephemeris_labels = 0;
    i->mag_min = 6.0;
    i->mag_min_automatic = 1;
    i->mag_max = 0.0;
    i->mag_step = 0.5;
    i->mag_alpha = 1.1727932;
    i->mag_size_norm = 0.4;
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
    i->ephemeris_col = (colour) {0, 0, 0};
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
    strcpy(i->constellation_highlight, "---");
    strcpy(i->ephemeris_compute_path, SRCDIR "../../ephemeris-compute-de430/bin/ephem.bin");
    strcpy(i->galaxy_map_filename, SRCDIR "../data/milkyWay/process/output/galaxymap.dat");
    strcpy(i->photo_filename, "");
    strcpy(i->output_filename, "chart");
    strcpy(i->copyright, "Produced with StarCharter. https://github.com/dcf21/star-charter");
    strcpy(i->title, "");

    // ----------------------------------------
    // Settings which we don't currently expose
    // ----------------------------------------

    strcpy(i->font_family, "Roboto");
    i->great_circle_line_width = 1.75;
    i->coordinate_grid_line_width = 1.3;
    i->dso_point_size_scaling = 1;
    i->constellation_sticks_line_width = 1.4;
    i->chart_edge_line_width = 2.5;
}

void config_init(chart_config *i) {
    if (i->coords == SW_COORDS_GAL) i->ra0 *= M_PI / 180; // Specify galactic longitude in degrees
    else i->ra0 *= M_PI / 12;  // Specify RA in hours
    i->dec0 *= M_PI / 180; // Specify declination and galactic latitude in degrees
    i->angular_width *= M_PI / 180; // Specify angular width in degrees
    if (i->projection == SW_PROJECTION_FLAT) i->wlin = i->angular_width;
    else if (i->projection == SW_PROJECTION_PETERS) i->wlin = i->angular_width;
    else if (i->projection == SW_PROJECTION_GNOM) i->wlin = 2 * tan(i->angular_width / 2);
    else if (i->projection == SW_PROJECTION_SPH)
        i->wlin = 2 * sin(i->angular_width / 2) * 1.12; // margin specified here
    else if (i->projection == SW_PROJECTION_ALTAZ)
        i->wlin = i->angular_width / (M_PI / 2) * 1.12; // margin specified here
    i->x_min = -i->wlin / 2;
    i->x_max = i->wlin / 2;
    i->y_min = -i->wlin / 2 * i->aspect;
    i->y_max = i->wlin / 2 * i->aspect;
    tweak_magnitude_limits(i);
    i->mag_highest = i->mag_max;
}

void config_close(chart_config *i) {
}

