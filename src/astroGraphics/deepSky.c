// deepSky.c
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

#include <gsl/gsl_math.h>

#include "astroGraphics/deepSky.h"
#include "coreUtils/asciiDouble.h"
#include "coreUtils/errorReport.h"
#include "mathsTools/projection.h"
#include "settings/chart_config.h"
#include "vectorGraphics/cairo_page.h"

//! plot_deep_sky_objects - Draw deep sky objects onto a star chart
//! \param s - A <chart_config> structure defining the properties of the star chart to be drawn.
//! \param page - A <cairo_page> structure defining the cairo drawing context.
//! \param messier_only - Boolean flag indicating whether we're only displaying Messier objects

void draw_open_cluster(chart_config *s, double x_canvas, double y_canvas, const double radius) {
    cairo_set_line_width(s->cairo_draw, 1);
    cairo_new_path(s->cairo_draw);
    cairo_arc(s->cairo_draw, x_canvas, y_canvas, radius, 0, 2 * M_PI);
    cairo_set_source_rgb(s->cairo_draw, s->dso_cluster_col.red, s->dso_cluster_col.grn, s->dso_cluster_col.blu);
    cairo_fill_preserve(s->cairo_draw);
    cairo_set_source_rgb(s->cairo_draw, s->dso_outline_col.red, s->dso_outline_col.grn, s->dso_outline_col.blu);
    cairo_stroke(s->cairo_draw);
}

void draw_globular_cluster(chart_config *s, double x_canvas, double y_canvas, const double radius) {
    cairo_set_line_width(s->cairo_draw, 1);
    cairo_new_path(s->cairo_draw);
    cairo_arc(s->cairo_draw, x_canvas, y_canvas, radius, 0, 2 * M_PI);
    cairo_set_source_rgb(s->cairo_draw, s->dso_cluster_col.red, s->dso_cluster_col.grn, s->dso_cluster_col.blu);
    cairo_fill_preserve(s->cairo_draw);
    cairo_set_source_rgb(s->cairo_draw, s->dso_outline_col.red, s->dso_outline_col.grn, s->dso_outline_col.blu);
    cairo_stroke(s->cairo_draw);
    cairo_set_line_width(s->cairo_draw, 0.5);
    cairo_new_path(s->cairo_draw);
    cairo_move_to(s->cairo_draw, x_canvas - radius, y_canvas);
    cairo_line_to(s->cairo_draw, x_canvas + radius, y_canvas);
    cairo_move_to(s->cairo_draw, x_canvas, y_canvas - radius);
    cairo_line_to(s->cairo_draw, x_canvas, y_canvas + radius);
    cairo_stroke(s->cairo_draw);
}

void draw_galaxy(chart_config *s, double axis_pa, double x_canvas, double y_canvas, const double radius_major,
                 const double radius_minor) {
    cairo_new_path(s->cairo_draw);
    cairo_save(s->cairo_draw);
    cairo_translate(s->cairo_draw, x_canvas, y_canvas);
    cairo_rotate(s->cairo_draw, (90 - axis_pa) * M_PI / 180);
    cairo_scale(s->cairo_draw, radius_major, radius_minor);
    cairo_arc(s->cairo_draw, 0, 0, 1, 0, 2 * M_PI);
    cairo_restore(s->cairo_draw);
    cairo_set_source_rgb(s->cairo_draw, s->dso_galaxy_col.red, s->dso_galaxy_col.grn, s->dso_galaxy_col.blu);
    cairo_fill_preserve(s->cairo_draw);
    cairo_set_line_width(s->cairo_draw, 0.5);
    cairo_set_source_rgb(s->cairo_draw, s->dso_outline_col.red, s->dso_outline_col.grn, s->dso_outline_col.blu);
    cairo_stroke(s->cairo_draw);
}

void draw_generic_nebula(chart_config *s, double x_canvas, double y_canvas, double point_size) {
    cairo_set_line_width(s->cairo_draw, 0.5);
    cairo_new_path(s->cairo_draw);
    cairo_move_to(s->cairo_draw, x_canvas - point_size, y_canvas - point_size);
    cairo_line_to(s->cairo_draw, x_canvas + point_size, y_canvas - point_size);
    cairo_line_to(s->cairo_draw, x_canvas + point_size, y_canvas + point_size);
    cairo_line_to(s->cairo_draw, x_canvas - point_size, y_canvas + point_size);
    cairo_close_path(s->cairo_draw);
    cairo_set_source_rgb(s->cairo_draw, s->dso_nebula_col.red, s->dso_nebula_col.grn, s->dso_nebula_col.blu);
    cairo_fill_preserve(s->cairo_draw);
    cairo_set_source_rgb(s->cairo_draw, s->dso_outline_col.red, s->dso_outline_col.grn, s->dso_outline_col.blu);
    cairo_stroke(s->cairo_draw);
}

void plot_deep_sky_objects(chart_config *s, cairo_page *page, int messier_only) {
    // Path to where deep sky object catalogue is stored
    const char *dso_object_catalogue = SRCDIR "../data/deepSky/ngcDistances/output/ngc_merged.txt";

    // Open data file listing the positions of the NGC and IC objects
    FILE *file = fopen(dso_object_catalogue, "r");
    if (file == NULL) stch_fatal(__FILE__, __LINE__, "Could not open deep sky catalogue");

    // Count the number of DSOs we have drawn and labelled, and make sure it doesn't exceed user-specified limits
    int dso_counter = 0;
    int label_counter = 0;

    // Loop over the lines of the data file
    while ((!feof(file)) && (!ferror(file))) {
        char line[FNAME_LENGTH];
        const char *line_ptr = line;

        file_readline(file, line);

        // Ignore comment lines
        if ((line[0] == '#') || (line[0] == '\n') || (line[0] == '\0')) continue;

        // Extract data from line of text
        while (*line_ptr == ' ') line_ptr++;
        int messier_num = (int) get_float(line_ptr, NULL);
        line_ptr = next_word(line_ptr);
        int ngc_num = (int) get_float(line_ptr, NULL);
        line_ptr = next_word(line_ptr);
        int ic_num = (int) get_float(line_ptr, NULL);
        line_ptr = next_word(line_ptr);
        double ra = get_float(line_ptr, NULL); // hours; J2000
        line_ptr = next_word(line_ptr);
        double dec = get_float(line_ptr, NULL); // degrees; J2000
        line_ptr = next_word(line_ptr);
        double mag = get_float(line_ptr, NULL); // magnitude
        line_ptr = next_word(line_ptr);
        double axis_major = get_float(line_ptr, NULL); // arcminutes
        line_ptr = next_word(line_ptr);
        double axis_minor = get_float(line_ptr, NULL); // arcminutes
        line_ptr = next_word(line_ptr);
        double axis_pa = get_float(line_ptr, NULL); // position angle; degrees
        const char *type_string = next_word(line_ptr);

        // If we're only showing Messier objects; only show them
        if (messier_only && (messier_num == 0)) {
            continue;
        }

        // Too faint; include objects with no magnitudes given (recorded as 0) if mag cutoff > mag 50.
        if ((s->dso_mag_min < 50) && (s->dso_mag_min < mag)) {
            continue;
        }

        // Project RA and Dec of object into physical coordinates on the star chart
        double x, y;
        plane_project(&x, &y, s, ra * M_PI / 12, dec * M_PI / 180, 0);

        // Reject this object if it falls outside the plot area
        if ((!gsl_finite(x)) || (!gsl_finite(y))) {
            continue;
        }

        if ((x < s->x_min) || (x > s->x_max) || (y < s->y_min) || (y > s->y_max)) {
            continue;
        }

        // Check if we've exceeded maximum number of objects
        if (dso_counter > s->maximum_dso_count) continue;
        dso_counter++;

        // Create a name for this object
        char object_name[FNAME_LENGTH] = "";
        if (messier_num > 0) {
            snprintf(object_name, FNAME_LENGTH, "M%d", messier_num);
        } else if (ngc_num > 0) {
            snprintf(object_name, FNAME_LENGTH, "NGC%d", ngc_num);
        } else if (ic_num > 0) {
            snprintf(object_name, FNAME_LENGTH, "IC%d", ic_num);
        }

        // Draw a symbol showing the position of this object
        double x_canvas, y_canvas, rendered_symbol_width = 0;
        const double pt = 1. / 72; // 1 pt
        const double point_size = 0.75 * 3 * pt * s->dso_point_size_scaling;
        double image_scale = s->width * s->cm / (s->x_max - s->x_min); // pixels per radian
        double arcminute = image_scale / (180 / M_PI * 60); // pixels per arcminute
        fetch_canvas_coordinates(&x_canvas, &y_canvas, x, y, s);

        if (strncmp(type_string, "OC", 2) == 0) {
            // Draw an open cluster
            const double radius = gsl_max((axis_major + axis_minor) / 4 * arcminute, s->dpi * point_size * 1.2);
            draw_open_cluster(s, x_canvas, y_canvas, radius);
            rendered_symbol_width = radius;
        } else if (strncmp(type_string, "Gb", 2) == 0) {
            // Draw a globular cluster
            const double radius = gsl_max((axis_major + axis_minor) / 4 * arcminute, s->dpi * point_size * 1.2);
            draw_globular_cluster(s, x_canvas, y_canvas, radius);
            rendered_symbol_width = radius;
        } else if (strncmp(type_string, "Gx", 2) == 0) {
            // Draw a galaxy
            double aspect_ratio = gsl_max(axis_minor, 1e-6) / gsl_max(axis_major, 1e-6);
            if ((axis_major == 0) || (axis_minor == 0) || (!gsl_finite(aspect_ratio))) aspect_ratio = 1;
            if (aspect_ratio > 1) aspect_ratio = 1;
            if (aspect_ratio < 0.2) aspect_ratio = 0.2;

            const double radius_major = gsl_max(gsl_max(axis_major, axis_minor) / 2 * arcminute, s->dpi * point_size);
            const double radius_minor = radius_major * aspect_ratio;

            // Work out direction of north on the chart
            double x2, y2;
            plane_project(&x2, &y2, s, ra * M_PI / 12, (dec + 1e-3) * M_PI / 180, 0);

            // Check output is finite
            if ((!gsl_finite(x2)) || (!gsl_finite(y2))) {
                continue;
            }

            const double north_direction[2] = {x2-x, y2-y};
            const double north_theta = atan2(north_direction[0], north_direction[1]) * 180 / M_PI; // degrees

            // Start drawing
            draw_galaxy(s, axis_pa + north_theta, x_canvas, y_canvas, radius_major, radius_minor);
            rendered_symbol_width = (
                    radius_major * sin(axis_pa * M_PI / 180) +
                    radius_minor * cos(axis_pa * M_PI / 180)
            );
            rendered_symbol_width = gsl_max(rendered_symbol_width, s->dpi * point_size);
        } else {
            draw_generic_nebula(s, x_canvas, y_canvas, s->dpi * point_size);
            rendered_symbol_width = s->dpi * point_size;
        }

        // Don't allow text labels to be placed over this deep sky object
        {
            double x_exclusion_region, y_exclusion_region;
            fetch_graph_coordinates(x_canvas, y_canvas, &x_exclusion_region, &y_exclusion_region, s);
            chart_add_label_exclusion(page, s,
                                      x_exclusion_region, x_exclusion_region,
                                      y_exclusion_region, y_exclusion_region);
        }

        // Consider whether to write a text label next to this deep sky object
        if ((mag < s->dso_label_mag_min) && (label_counter < s->maximum_dso_label_count)) {

            // Write a text label for this object
            const double horizontal_offset = 1.1 * s->mm + rendered_symbol_width;
            const int show_name = s->dso_names;
            const int show_mag = s->dso_mags && (mag < 40);
            const int multiple_labels = show_name && show_mag;

            if (show_name) {
                chart_label_buffer(page, s, s->dso_label_col, object_name,
                                   (label_position[2]) {{x, y, horizontal_offset,  -1, 0},
                                                        {x, y, -horizontal_offset, 1,  0}}, 2,
                                   multiple_labels, 0, 1.2 * s->label_font_size_scaling,
                                   0, 0, 0, mag);
                label_counter++;
            }
            if (show_mag) {
                snprintf(temp_err_string, FNAME_LENGTH, "mag %.1f", mag);
                chart_label_buffer(page, s, s->dso_label_col, temp_err_string,
                                   (label_position[2]) {{x, y, horizontal_offset,  -1, 0},
                                                        {x, y, -horizontal_offset, 1,  0}}, 2,
                                   multiple_labels, 0, 1.2 * s->label_font_size_scaling,
                                   0, 0, 0, mag);
                label_counter++;
            }
        }
    }

    // Close data file listing deep sky objects
    fclose(file);

    // print debugging message
    if (DEBUG) {
        snprintf(temp_err_string, FNAME_LENGTH, "Displayed %d DSOs and %d DSO labels", dso_counter, label_counter);
        stch_log(temp_err_string);
    }
}

//! draw_dso_symbol_key - Draw a legend below the star chart indicating the symbols used to represent deep sky objects.
//! \param s - A <chart_config> structure defining the properties of the star chart to be drawn.
//! \param legend_y_pos - The vertical pixel position of the top of the next legend to go under the star chart.

double draw_dso_symbol_key(chart_config *s, double legend_y_pos) {
    const double w_left = 0.4; // The left margin
    const double w_item = 3.7 * s->font_size; // The width of each legend item (cm)

    // Number of items to show in legend
    const double N = 3.7;

    // The top (y0) and bottom (y1) of the legend
    const double y0 = legend_y_pos;
    const double y1 = y0 - 0.4;

    // The horizontal position of the centre of the legend
    const double x1 = s->canvas_offset_x + (s->width - s->legend_right_column_width) / 2;

    // The width of the legend
    const double xw = w_left * 1.5 + w_item * N;
    const double size = 0.4;

    // The left edge of the legend
    double x = x1 - xw / 2;
    cairo_text_extents_t extents;

    cairo_set_font_size(s->cairo_draw, 3.6 * s->mm * s->font_size);
    cairo_set_line_width(s->cairo_draw, 2.5 * s->line_width_base);

    x += w_left;

    // Reset font weight
    cairo_select_font_face(s->cairo_draw, s->font_family, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);

    // Set point size for all legend entries
    double point_size = 0.2 * s->cm;

    // Draw galaxy symbol
    draw_galaxy(s, 30, x * s->cm, y1 * s->cm, point_size, point_size * 0.5);

    // Write a text label next to it
    {
        const char *label = "Galaxy";
        cairo_set_source_rgb(s->cairo_draw, 0, 0, 0);
        cairo_text_extents(s->cairo_draw, label, &extents);
        cairo_move_to(s->cairo_draw,
                      (x + 0.1 + size) * s->cm - extents.x_bearing,
                      y1 * s->cm - extents.height / 2 - extents.y_bearing
        );
        cairo_show_text(s->cairo_draw, label);
    }

    // Advance horizontally to draw the next item in the legend
    x += w_item * 0.7;

    // Draw generic nebula symbol
    draw_generic_nebula(s, x * s->cm, y1 * s->cm, point_size * 0.5);

    // Write a text label next to it
    {
        const char *label = "Bright nebula";
        cairo_set_source_rgb(s->cairo_draw, 0, 0, 0);
        cairo_text_extents(s->cairo_draw, label, &extents);
        cairo_move_to(s->cairo_draw,
                      (x + 0.1 + size) * s->cm - extents.x_bearing,
                      y1 * s->cm - extents.height / 2 - extents.y_bearing
        );
        cairo_show_text(s->cairo_draw, label);
    }

    // Advance horizontally to draw the next item in the legend
    x += w_item;

    // Draw open cluster symbol
    draw_open_cluster(s, x * s->cm, y1 * s->cm, point_size);

    // Write a text label next to it
    {
        const char *label = "Open cluster";
        cairo_set_source_rgb(s->cairo_draw, 0, 0, 0);
        cairo_text_extents(s->cairo_draw, label, &extents);
        cairo_move_to(s->cairo_draw,
                      (x + 0.1 + size) * s->cm - extents.x_bearing,
                      y1 * s->cm - extents.height / 2 - extents.y_bearing
        );
        cairo_show_text(s->cairo_draw, label);
    }

    // Advance horizontally to draw the next item in the legend
    x += w_item;

    // Draw globular cluster symbol
    draw_globular_cluster(s, x * s->cm, y1 * s->cm, point_size);

    // Write a text label next to it
    {
        const char *label = "Globular cluster";
        cairo_set_source_rgb(s->cairo_draw, 0, 0, 0);
        cairo_text_extents(s->cairo_draw, label, &extents);
        cairo_move_to(s->cairo_draw,
                      (x + 0.1 + size) * s->cm - extents.x_bearing,
                      y1 * s->cm - extents.height / 2 - extents.y_bearing
        );
        cairo_show_text(s->cairo_draw, label);
    }

    // Advance horizontally to draw the next item in the legend
    x += w_item;

    // Finally, return the vertical position for the next legend item below this one
    const double new_bottom_to_legend_items = y0 + 0.4;
    return new_bottom_to_legend_items;
}
