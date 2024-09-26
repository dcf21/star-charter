// textAnnotations.c
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <gsl/gsl_math.h>

#include "astroGraphics/scaleBars.h"
#include "astroGraphics/textAnnotations.h"
#include "coreUtils/asciiDouble.h"
#include "coreUtils/errorReport.h"
#include "mathsTools/julianDate.h"
#include "mathsTools/projection.h"
#include "settings/chart_config.h"
#include "vectorGraphics/arrowDraw.h"
#include "vectorGraphics/cairo_page.h"

//! annotation_transform_coordinates - Transform the supplied coordinates for annotations into Cairo pixels
//! \param s - A <chart_config> structure defining the properties of the star chart to be drawn.
//! \param coordinates - The chosen coordinate system for this point
//! \param x_pos - The x coordinate of this point in the input coordinate system (0-1, hours or degrees)
//! \param y_pos - The y coordinate of this point in the input coordinate system (0-1, or degrees)
//! \param x_cairo - The x coordinate in Cairo pixels
//! \param y_cairo - The y coordinate in Cairo pixels
//! \param x_tangent - The x coordinate in the tangent plane; radians
//! \param y_tangent - The y coordinate in the tangent plane; radians

void annotation_transform_coordinates(const chart_config *s, const int coordinates,
                                      const double x_pos, const double y_pos,
                                      double *x_cairo, double *y_cairo, double *x_tangent, double *y_tangent) {
    if (coordinates == SW_ANNOTATION_COORDS_PAGE) {
        // Page coordinates
        convert_normalised_coordinates_to_tangent_plane(s, x_pos, y_pos,
                                                        x_tangent, y_tangent,
                                                        x_cairo, y_cairo);
    } else if (coordinates == SW_ANNOTATION_COORDS_ALTAZ) {
        double ra_at_epoch, dec_at_epoch, ra_j2000, dec_j2000;

        // Alt/az coordinates
        const double az_radians = y_pos * M_PI / 180;
        const double alt_radians = x_pos * M_PI / 180;
        inv_alt_az(alt_radians, az_radians,
                   s->julian_date, s->horizon_latitude, s->horizon_longitude,
                   &ra_at_epoch, &dec_at_epoch);

        // Convert (RA, Dec) at epoch <julian_date> to J2000
        ra_dec_to_j2000(ra_at_epoch, dec_at_epoch, s->julian_date, &ra_j2000, &dec_j2000);

        // RA/Dec coordinates
        plane_project(x_tangent, y_tangent, s, ra_j2000, dec_j2000, 0);
        fetch_canvas_coordinates(x_cairo, y_cairo, (*x_tangent), (*y_tangent), s);
    } else {
        // RA/Dec coordinates
        const double ra_radians = x_pos * M_PI / 12;
        const double dec_radians = y_pos * M_PI / 180;
        plane_project(x_tangent, y_tangent, s, ra_radians, dec_radians, 0);
        fetch_canvas_coordinates(x_cairo, y_cairo, (*x_tangent), (*y_tangent), s);
    }
}

//! plot_arrow_annotations - Plots arrow/line annotations onto the star chart.
//! \param s - A <chart_config> structure defining the properties of the star chart to be drawn.
//! \param page - A <cairo_page> structure defining the cairo drawing context.

void plot_arrow_annotations(chart_config *s) {
    // Loop over all the annotations to write
    for (int i = 0; i < s->arrow_labels_final_count; i++) {
        // Pointer to label definition
        const char *def = s->arrow_labels[i];

        // Extract comma-separated entries from the definition string
        const char *in_scan = def;
        char buffer[FNAME_LENGTH], coordinates_str[FNAME_LENGTH];
        int coordinates_0, coordinates_1;

        // First point - Read coordinate system
        str_comma_separated_list_scan(&in_scan, coordinates_str);
        if (str_cmp_no_case(coordinates_str, "page") == 0) {
            coordinates_0 = SW_ANNOTATION_COORDS_PAGE;
        } else if (str_cmp_no_case(coordinates_str, "ra_dec") == 0) {
            coordinates_0 = SW_ANNOTATION_COORDS_RADEC;
        } else if (str_cmp_no_case(coordinates_str, "alt_az") == 0) {
            coordinates_0 = SW_ANNOTATION_COORDS_ALTAZ;
        } else {
            char err_buffer[FNAME_LENGTH];
            snprintf(err_buffer, FNAME_LENGTH, "Unknown coordinate system <%s>.", coordinates_str);
            stch_fatal(__FILE__, __LINE__, err_buffer);
            exit(1);
        }

        // First point - Read x_pos
        str_comma_separated_list_scan(&in_scan, buffer);
        const double x_pos_0 = get_float(buffer, NULL);

        // First point - Read y_pos
        str_comma_separated_list_scan(&in_scan, buffer);
        const double y_pos_0 = get_float(buffer, NULL);

        // Second point - Read coordinate system
        str_comma_separated_list_scan(&in_scan, coordinates_str);
        if (str_cmp_no_case(coordinates_str, "page") == 0) {
            coordinates_1 = SW_ANNOTATION_COORDS_PAGE;
        } else if (str_cmp_no_case(coordinates_str, "ra_dec") == 0) {
            coordinates_1 = SW_ANNOTATION_COORDS_RADEC;
        } else if (str_cmp_no_case(coordinates_str, "alt_az") == 0) {
            coordinates_1 = SW_ANNOTATION_COORDS_ALTAZ;
        } else {
            char err_buffer[FNAME_LENGTH];
            snprintf(err_buffer, FNAME_LENGTH, "Unknown coordinate system <%s>.", coordinates_str);
            stch_fatal(__FILE__, __LINE__, err_buffer);
            exit(1);
        }

        // Second point - Read x_pos
        str_comma_separated_list_scan(&in_scan, buffer);
        const double x_pos_1 = get_float(buffer, NULL);

        // Second point - Read y_pos
        str_comma_separated_list_scan(&in_scan, buffer);
        const double y_pos_1 = get_float(buffer, NULL);

        // First point - Head flag
        str_comma_separated_list_scan(&in_scan, buffer);
        const int head_0 = (int) get_float(buffer, NULL);

        // Second point - Head flag
        str_comma_separated_list_scan(&in_scan, buffer);
        const int head_1 = (int) get_float(buffer, NULL);

        // Read colour
        colour label_colour;
        str_comma_separated_list_scan(&in_scan, buffer);
        label_colour.red = get_float(buffer, NULL);
        str_comma_separated_list_scan(&in_scan, buffer);
        label_colour.grn = get_float(buffer, NULL);
        str_comma_separated_list_scan(&in_scan, buffer);
        label_colour.blu = get_float(buffer, NULL);

        // Line width
        str_comma_separated_list_scan(&in_scan, buffer);
        const double line_width = get_float(buffer, NULL);

        // First point - Convert supplied coordinates to tangent-plane coordinates
        double x_cairo_0, y_cairo_0, x_tangent_0, y_tangent_0;
        annotation_transform_coordinates(s, coordinates_0, x_pos_0, y_pos_0,
                                         &x_cairo_0, &y_cairo_0, &x_tangent_0, &y_tangent_0);

        // Second point - Convert supplied coordinates to tangent-plane coordinates
        double x_cairo_1, y_cairo_1, x_tangent_1, y_tangent_1;
        annotation_transform_coordinates(s, coordinates_1, x_pos_1, y_pos_1,
                                         &x_cairo_1, &y_cairo_1, &x_tangent_1, &y_tangent_1);

        // Draw arrow
        cairo_set_line_width(s->cairo_draw, line_width * s->line_width_base);
        cairo_set_source_rgb(s->cairo_draw,
                             label_colour.red, label_colour.grn, label_colour.blu);
        draw_arrow(s, line_width, head_0, head_1, x_cairo_0, y_cairo_0, x_cairo_1, y_cairo_1);
    }
}

//! plot_text_annotations - Plots text annotations onto the star chart.
//! \param s - A <chart_config> structure defining the properties of the star chart to be drawn.
//! \param page - A <cairo_page> structure defining the cairo drawing context.

void plot_text_annotations(chart_config *s, cairo_page *page) {
    // Loop over all the annotations to write
    for (int i = 0; i < s->text_labels_final_count; i++) {
        // Pointer to label definition
        const char *def = s->text_labels[i];

        // Extract comma-separated entries from the definition string
        const char *in_scan = def;
        char buffer[FNAME_LENGTH], coordinates_str[FNAME_LENGTH];
        int coordinates;

        // Read coordinate system
        str_comma_separated_list_scan(&in_scan, coordinates_str);
        if (str_cmp_no_case(coordinates_str, "page") == 0) {
            coordinates = SW_ANNOTATION_COORDS_PAGE;
        } else if (str_cmp_no_case(coordinates_str, "ra_dec") == 0) {
            coordinates = SW_ANNOTATION_COORDS_RADEC;
        } else if (str_cmp_no_case(coordinates_str, "alt_az") == 0) {
            coordinates = SW_ANNOTATION_COORDS_ALTAZ;
        } else {
            char err_buffer[FNAME_LENGTH];
            snprintf(err_buffer, FNAME_LENGTH, "Unknown coordinate system <%s>.", coordinates_str);
            stch_fatal(__FILE__, __LINE__, err_buffer);
            exit(1);
        }

        // Read x_pos
        str_comma_separated_list_scan(&in_scan, buffer);
        const double x_pos = get_float(buffer, NULL);

        // Read y_pos
        str_comma_separated_list_scan(&in_scan, buffer);
        const double y_pos = get_float(buffer, NULL);

        // Read x_align
        str_comma_separated_list_scan(&in_scan, buffer);
        const int x_align = (int) get_float(buffer, NULL);

        // Read y_align
        str_comma_separated_list_scan(&in_scan, buffer);
        const int y_align = (int) get_float(buffer, NULL);

        // Read font size
        str_comma_separated_list_scan(&in_scan, buffer);
        const double font_size = get_float(buffer, NULL);

        // Read font bold
        str_comma_separated_list_scan(&in_scan, buffer);
        const int font_bold = (int) get_float(buffer, NULL);

        // Read font italic
        str_comma_separated_list_scan(&in_scan, buffer);
        const int font_italic = (int) get_float(buffer, NULL);

        // Read colour
        colour label_colour;
        str_comma_separated_list_scan(&in_scan, buffer);
        label_colour.red = get_float(buffer, NULL);
        str_comma_separated_list_scan(&in_scan, buffer);
        label_colour.grn = get_float(buffer, NULL);
        str_comma_separated_list_scan(&in_scan, buffer);
        label_colour.blu = get_float(buffer, NULL);

        // Read label text
        const char *label_text = in_scan;

        // Convert supplied coordinates to tangent-plane coordinates
        double x_cairo, y_cairo, x_tangent, y_tangent;
        annotation_transform_coordinates(s, coordinates, x_pos, y_pos, &x_cairo, &y_cairo, &x_tangent, &y_tangent);

        // Write label
        chart_label_buffer(page, s, label_colour, label_text,
                           (label_position[1]) {
                                   {x_tangent, y_tangent, 0, 0, 0, x_align, y_align}
                           }, 1,
                           0, 0, font_size * s->label_font_size_scaling,
                           font_bold, font_italic, 0, -10);
    }
}
