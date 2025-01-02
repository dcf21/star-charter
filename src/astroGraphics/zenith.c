// zenith.c
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

#include "coreUtils/asciiDouble.h"
#include "coreUtils/errorReport.h"
#include "mathsTools/julianDate.h"
#include "mathsTools/projection.h"
#include "settings/chart_config.h"
#include "vectorGraphics/cairo_page.h"

//! plot_zenith - Plot the positions of the zenith.
//! \param s - A <chart_config> structure defining the properties of the star chart to be drawn.
//! \param page - A <cairo_page> structure defining the cairo drawing context.

void plot_zenith(chart_config *s, cairo_page *page) {
    // Find the coordinates of the zenith
    double ra_zenith_at_epoch, dec_zenith_at_epoch;
    double ra_zenith_j2000, dec_zenith_j2000;
    get_zenith_position(s->horizon_latitude, s->horizon_longitude, s->julian_date,
                        &ra_zenith_at_epoch, &dec_zenith_at_epoch);
    ra_dec_to_j2000(ra_zenith_at_epoch, dec_zenith_at_epoch, s->julian_date,
                    &ra_zenith_j2000, &dec_zenith_j2000);

    // Project onto the plotting canvas
    double x, y;
    plane_project(&x, &y, s, ra_zenith_j2000, dec_zenith_j2000, 0);

    // Ignore this marker if it falls outside the plot area
    if ((!gsl_finite(x)) || (!gsl_finite(y)) ||
        (x < s->x_min) || (x > s->x_max) || (y < s->y_min) || (y > s->y_max)) {
        return;
    }

    // Draw zenith marker
    double x_canvas, y_canvas;
    fetch_canvas_coordinates(&x_canvas, &y_canvas, x, y, s);
    cairo_set_source_rgb(s->cairo_draw, s->horizon_zenith_colour.red,
                         s->horizon_zenith_colour.grn, s->horizon_zenith_colour.blu);

    // Draw cross
    const double marker_size = 0.1 * s->horizon_zenith_marker_size * s->dpi;
    const double marker_line_width = 1.3;

    cairo_set_line_width(s->cairo_draw, marker_line_width);
    cairo_new_path(s->cairo_draw);
    cairo_move_to(s->cairo_draw, x_canvas - marker_size, y_canvas);
    cairo_line_to(s->cairo_draw, x_canvas + marker_size, y_canvas);
    cairo_move_to(s->cairo_draw, x_canvas, y_canvas - marker_size);
    cairo_line_to(s->cairo_draw, x_canvas, y_canvas + marker_size);
    cairo_stroke(s->cairo_draw);

    // Write label
    chart_label_buffer(page, s, s->horizon_zenith_colour, "Zenith",
                       (label_position[2]) {
                               {x, y, 0, marker_size / 4,  marker_size / 4, -1, -1},
                               {x, y, 0, -marker_size / 4, marker_size / 4, 1,  -1}
                       }, 2,
                       0, 0, 1.4 * s->label_font_size_scaling,
                       0, 0, 0, -10);
}

//! plot_celestial poles - Plot the positions of the celestial poles.
//! \param s - A <chart_config> structure defining the properties of the star chart to be drawn.
//! \param page - A <cairo_page> structure defining the cairo drawing context.

void plot_celestial_poles(chart_config *s, cairo_page *page) {
    // Plot each pole in turn
    for (double decl = -90; decl < 91; decl += 180) {
        // Project onto the plotting canvas
        double x, y;
        plane_project(&x, &y, s, 0, decl * M_PI / 180.00001, 0);

        // Ignore this marker if it falls outside the plot area
        if ((!gsl_finite(x)) || (!gsl_finite(y)) ||
            (x < s->x_min) || (x > s->x_max) || (y < s->y_min) || (y > s->y_max)) {
            continue;
        }

        // Draw pole marker
        double x_canvas, y_canvas;
        fetch_canvas_coordinates(&x_canvas, &y_canvas, x, y, s);
        cairo_set_source_rgb(s->cairo_draw, s->horizon_zenith_colour.red,
                             s->horizon_zenith_colour.grn, s->horizon_zenith_colour.blu);

        // Draw cross
        const double marker_size = 0.1 * s->horizon_zenith_marker_size * s->dpi;
        const double marker_line_width = 1.3;

        cairo_set_line_width(s->cairo_draw, marker_line_width);
        cairo_new_path(s->cairo_draw);
        cairo_move_to(s->cairo_draw, x_canvas - marker_size, y_canvas);
        cairo_line_to(s->cairo_draw, x_canvas + marker_size, y_canvas);
        cairo_move_to(s->cairo_draw, x_canvas, y_canvas - marker_size);
        cairo_line_to(s->cairo_draw, x_canvas, y_canvas + marker_size);
        cairo_stroke(s->cairo_draw);

        // Write label
        char buffer[FNAME_LENGTH];
        snprintf(buffer, FNAME_LENGTH, "%s Celestial Pole", ((decl < 0) ? "South" : "North"));
        chart_label_buffer(page, s, s->horizon_zenith_colour, buffer,
                           (label_position[2]) {
                                   {x, y, 0, marker_size / 4,  marker_size / 4, -1, -1},
                                   {x, y, 0, -marker_size / 4, marker_size / 4, 1,  -1}
                           }, 2,
                           0, 0, 1.4 * s->label_font_size_scaling,
                           0, 0, 0, -10);
    }
}
