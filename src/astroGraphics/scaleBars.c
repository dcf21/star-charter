// scaleBars.c
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

#include "astroGraphics/scaleBars.h"
#include "coreUtils/asciiDouble.h"
#include "coreUtils/errorReport.h"
#include "mathsTools/projection.h"
#include "mathsTools/sphericalTrig.h"
#include "settings/chart_config.h"
#include "vectorGraphics/arrowDraw.h"
#include "vectorGraphics/lineDraw.h"
#include "vectorGraphics/cairo_page.h"

//! convert_normalised_coordinates_to_tangent_plane - Convert coordinates normalised to 0-1 across the star chart
//! canvas into tangent plane coordinates
//! \param [in] s - A <chart_config> structure defining the properties of the star chart to be drawn.
//! \param [in] x_norm - Input x coordinate, normalised from 0 to 1.
//! \param [in] y_norm - Input y coordinate, normalised from 0 to 1.
//! \param [out] x_tangent_out - Output tangent plane x coordinate (radians)
//! \param [out] y_tangent_out - Output tangent plane y coordinate (radians)

void convert_normalised_coordinates_to_tangent_plane(const chart_config *s,
                                                     double x_norm, double y_norm,
                                                     double *x_tangent_out, double *y_tangent_out,
                                                     double *x_cairo_out, double *y_cairo_out) {
    // Convert central coordinates of scale bar from (0-1) into Cairo coordinates
    const double x_cairo = (x_norm * s->width + s->canvas_offset_x) * s->cm;
    const double y_cairo = (y_norm * s->width * s->aspect + s->canvas_offset_y) * s->cm;

    // Convert central coordinates of scale bar to tangent-plane coordinates
    double x_tangent, y_tangent;
    fetch_graph_coordinates(x_cairo, y_cairo, &x_tangent, &y_tangent, s);

    // Return outputs
    if (x_tangent_out != NULL) *x_tangent_out = x_tangent;
    if (y_tangent_out != NULL) *y_tangent_out = y_tangent;

    if (x_cairo_out != NULL) *x_cairo_out = x_cairo;
    if (y_cairo_out != NULL) *y_cairo_out = y_cairo;
}

//! cairo_units_per_degree - Calculate scaling of Cairo units per degree on the sky
//! \param [in] s - A <chart_config> structure defining the properties of the star chart to be drawn.
//! \return - Cairo units per degree on the sky

double cairo_units_per_degree(const chart_config *s) {
    // Calculate RA and Dec of two points separated by half the height of the plot
    double x_tangent_0, y_tangent_0, x_tangent_1, y_tangent_1;
    double ra0, dec0, ra1, dec1;
    convert_normalised_coordinates_to_tangent_plane(s, 0.5, 0.25, &x_tangent_0, &y_tangent_0, NULL, NULL);
    convert_normalised_coordinates_to_tangent_plane(s, 0.5, 0.75, &x_tangent_1, &y_tangent_1, NULL, NULL);
    inv_plane_project(&ra0, &dec0, s, x_tangent_0, y_tangent_0);
    inv_plane_project(&ra1, &dec1, s, x_tangent_1, y_tangent_1);

    // Angular distance between top and bottom of the chart
    const double angular_height = angDist_RADec(ra0, dec0, ra1, dec1) * 180 / M_PI / 0.5;

    // Physical distance between top and bottom of the chart
    const double physical_height = s->width * s->aspect * s->cm;

    // Calculate scaling of Cairo units per degree on the sky
    const double angle_scaling = physical_height / angular_height;
    return angle_scaling;
}

//! plot_scale_bars - Plot requested scale bars into the star chart.
//! \param [in] s - A <chart_config> structure defining the properties of the star chart to be drawn.
//! \param [in] page - A <cairo_page> structure defining the cairo drawing context.

void plot_scale_bars(chart_config *s, cairo_page *page) {
    // Loop over each requested scale bar
    for (int index = 0; index < s->scale_bars_final_count; index++) {
        // Fetch the string definition of this scale bar, passed by the user
        const char *scale_bar_definition = s->scale_bars[index];

        // Extract <x_pos>, <y_pos>, <position_angle>, <degrees> from the scale bar definition string
        const char *in_scan = scale_bar_definition;
        char buffer[FNAME_LENGTH];

        // Read <x_pos>
        str_comma_separated_list_scan(&in_scan, buffer);
        const double x_pos = get_float(buffer, NULL);  // 0-1
        // Read <y_pos>
        str_comma_separated_list_scan(&in_scan, buffer);
        const double y_pos = get_float(buffer, NULL);  // 0-1
        // Read <position_angle>
        str_comma_separated_list_scan(&in_scan, buffer);
        const double position_angle = get_float(buffer, NULL);  // degrees
        // Read length in degrees
        str_comma_separated_list_scan(&in_scan, buffer);
        const double scale_degrees = get_float(buffer, NULL);  // degrees on sky

        // Convert central coordinates of scale bar to tangent-plane coordinates
        double x_cairo, y_cairo, x_tangent, y_tangent;
        convert_normalised_coordinates_to_tangent_plane(s, x_pos, y_pos,
                                                        &x_tangent, &y_tangent,
                                                        &x_cairo, &y_cairo);

        // Calculate length of arrow
        double arrow_half_length = 0;
        {
            // Calculate scaling of Cairo units per degree on the sky
            double angle_scaling = cairo_units_per_degree(s);

            // Height of arrow of required length
            arrow_half_length = 0.5 * angle_scaling * scale_degrees;
        }

        // Marker size
        const double marker_size = 0.4 * s->cm;

        // Rotation angle of arrow
        const double pa = position_angle * M_PI / 180;

        // Create string label
        char label_text[FNAME_LENGTH];
        snprintf(label_text, FNAME_LENGTH, "%.0f°", scale_degrees);
        label_text[FNAME_LENGTH - 1] = '\0';

        // Draw arrow
        cairo_set_source_rgba(s->cairo_draw,
                              s->scale_bar_colour.red, s->scale_bar_colour.grn, s->scale_bar_colour.blu,
                              s->scale_bar_colour.alpha);
        draw_arrow(s, 2, 1, 1,
                   x_cairo + arrow_half_length * sin(pa), y_cairo - arrow_half_length * cos(pa),
                   x_cairo - arrow_half_length * sin(pa), y_cairo + arrow_half_length * cos(pa));

        // Write label
        chart_label_buffer(page, s, s->scale_bar_colour, label_text,
                           (label_position[2]) {
                                   {x_tangent, y_tangent, 0, marker_size * cos(pa),  marker_size * sin(pa),  0, 0},
                                   {x_tangent, y_tangent, 0, -marker_size * cos(pa), -marker_size * sin(pa), 0, 0}
                           }, 2,
                           0, 0, 1.6 * s->label_font_size_scaling,
                           0, 0, 0, -10);
    }
}
