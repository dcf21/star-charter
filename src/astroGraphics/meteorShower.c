// meteorShower.c
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
#include <string.h>
#include <math.h>

#include <gsl/gsl_math.h>

#include "coreUtils/asciiDouble.h"
#include "coreUtils/errorReport.h"
#include "mathsTools/projection.h"
#include "mathsTools/sphericalTrig.h"
#include "settings/chart_config.h"
#include "vectorGraphics/arrowDraw.h"
#include "vectorGraphics/cairo_page.h"

//! plot_meteor_showers - Plot the positions of the radiants of meteor showers.
//! \param s - A <chart_config> structure defining the properties of the star chart to be drawn.
//! \param page - A <cairo_page> structure defining the cairo drawing context.

void plot_meteor_showers(chart_config *s, cairo_page *page) {
    // Loop over all the radiants we are to show
    for (int index = 0; index < s->meteor_radiants_final_count; index++) {
        // Fetch the string definition of this meteor shower radiant, passed by the user
        // For example: Geminids,112.0000,33.0000
        const char *trace_definition = s->meteor_radiants[index];

        // Extract label, ra/deg, dec/deg from shower definition string
        const char *in_scan = trace_definition;
        char label[FNAME_LENGTH], buffer[FNAME_LENGTH];
        double ra_radiant_j2000, dec_radiant_j2000;

        // Read object name into <object_id>
        str_comma_separated_list_scan(&in_scan, label);

        // Read RA/deg
        str_comma_separated_list_scan(&in_scan, buffer);
        ra_radiant_j2000 = get_float(buffer, NULL);

        // Read dec/deg
        str_comma_separated_list_scan(&in_scan, buffer);
        dec_radiant_j2000 = get_float(buffer, NULL);

        // Project onto the plotting canvas
        double x, y;  // radians in tangent plane
        plane_project(&x, &y, s, ra_radiant_j2000 * M_PI / 180, dec_radiant_j2000 * M_PI / 180);

        // Ignore this star if it falls outside the plot area
        if ((!gsl_finite(x)) || (!gsl_finite(y)) ||
            (x < s->x_min) || (x > s->x_max) || (y < s->y_min) || (y > s->y_max)) {
            return;
        }

        // Draw radiant marker
        double x_canvas, y_canvas; // Cairo native coordinates
        fetch_canvas_coordinates(&x_canvas, &y_canvas, x, y, s);
        cairo_set_source_rgb(s->cairo_draw, s->meteor_radiant_colour.red,
                             s->meteor_radiant_colour.grn, s->meteor_radiant_colour.blu);

        // Calculate marker size
        const double marker_size = 0.3 * s->meteor_radiant_marker_size * s->cm;
        const double marker_line_width = 1;

        // Draw outward-pointing arrows
        for (int j = 0; j < 6; j++) {
            const double pa = j * M_PI / 3;
            const double x0 = x_canvas + marker_size * 0.5 * cos(pa);
            const double y0 = y_canvas + marker_size * 0.5 * sin(pa);
            const double x1 = x_canvas + marker_size * 1.5 * cos(pa);
            const double y1 = y_canvas + marker_size * 1.5 * sin(pa);

            draw_arrow(s, 0.7, 0, 1, x0, y0, x1, y1);
        }

        // Draw circle
        cairo_set_line_width(s->cairo_draw, marker_line_width);
        cairo_new_path(s->cairo_draw);
        cairo_arc(s->cairo_draw, x_canvas, y_canvas, marker_size, 0, 2 * M_PI);
        cairo_stroke(s->cairo_draw);

        // Write label
        chart_label_buffer(page, s, s->meteor_radiant_colour, label,
                           (label_position[2]) {
                                   {x, y, 0, marker_size,  marker_size, -1, 0},
                                   {x, y, 0, -marker_size, marker_size, 1,  0}
                           }, 2,
                           0, 0, 1.6 * s->label_font_size_scaling,
                           0, 0, 0, -10);
    }

}
