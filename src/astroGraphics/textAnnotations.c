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
#include "mathsTools/projection.h"
#include "settings/chart_config.h"
#include "vectorGraphics/cairo_page.h"

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
        char buffer[FNAME_LENGTH];

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
        convert_normalised_coordinates_to_tangent_plane(s, x_pos, y_pos,
                                                        &x_tangent, &y_tangent,
                                                        &x_cairo, &y_cairo);

        // Write label
        chart_label_buffer(page, s, label_colour, label_text,
                           (label_position[1]) {
                                   {x_tangent, y_tangent, 0, 0, 0, x_align, y_align}
                           }, 1,
                           0, 0, font_size * s->label_font_size_scaling,
                           0, 0, 0, -10);
    }
}
