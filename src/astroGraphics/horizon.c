// horizon.c
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

#include "astroGraphics/ephemeris.h"
#include "astroGraphics/greatCircles.h"
#include "coreUtils/asciiDouble.h"
#include "coreUtils/errorReport.h"
#include "mathsTools/julianDate.h"
#include "mathsTools/projection.h"
#include "mathsTools/sphericalTrig.h"
#include "settings/chart_config.h"
#include "vectorGraphics/lineDraw.h"
#include "vectorGraphics/cairo_page.h"

// Cardinal point label texts
static const char cardinal_label[16][6] = {
        "N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE",
        "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"
};

//! project_alt_az_to_xy - Project an (alt, az) pair into a position within the tangent plane.
//! \param s  - A <chart_config> structure defining the properties of the star chart to be drawn.
//! \param alt - The altitude to project into tangent plane coordinates (radians)
//! \param az - The azimuth to project into tangent plane coordinates (radians)
//! \param x_pixels_out - The output x coordinate within the tangent plane (pixels)
//! \param y_pixels_out - The output y coordinate within the tangent plane (pixels)
//! \param x_tangent_out - The output x coordinate within the tangent plane (radians)
//! \param y_tangent_out - The output y coordinate within the tangent plane (radians)

void project_alt_az_to_xy(chart_config *s, double alt, double az,
                          double *x_pixels_out, double *y_pixels_out, double *x_tangent_out, double *y_tangent_out) {
    // Convert (Alt, Az) coordinates to (RA, Dec)
    double ra, dec;
    convert_selected_coordinates_to_ra_dec(s, SW_COORDS_ALTAZ, az, alt, &ra, &dec);

    // Project this point onto the drawing canvas
    double x_tangent, y_tangent;
    plane_project(&x_tangent, &y_tangent, s, ra, dec, 0);

    // Convert tangent-plane position into pixel coordinates
    double x_pixels, y_pixels;
    fetch_canvas_coordinates(&x_pixels, &y_pixels, x_tangent, y_tangent, s);

    // Return outputs
    if (x_pixels_out != NULL) *x_pixels_out = x_pixels;
    if (y_pixels_out != NULL) *y_pixels_out = y_pixels;
    if (x_tangent_out != NULL) *x_tangent_out = x_tangent;
    if (y_tangent_out != NULL) *y_tangent_out = y_tangent;
}

//! plot_horizon - Plot the horizon onto the sky.
//! \param s - A <chart_config> structure defining the properties of the star chart to be drawn.
//! \param page - A <cairo_page> structure defining the cairo drawing context.

void plot_horizon(chart_config *s, line_drawer *ld, cairo_page *page) {
    // Temporarily disable horizon clipping
    int show_horizon = s->show_horizon;
    s->show_horizon = 0;

    // Find the coordinates of the zenith
    double ra_zenith_at_epoch, dec_zenith_at_epoch;
    double ra_zenith_j2000, dec_zenith_j2000;
    get_zenith_position(s->horizon_latitude, s->horizon_longitude, s->julian_date,
                        &ra_zenith_at_epoch, &dec_zenith_at_epoch);
    ra_dec_to_j2000(ra_zenith_at_epoch, dec_zenith_at_epoch, s->julian_date,
                    &ra_zenith_j2000, &dec_zenith_j2000);

    // Set line colour
    colour horizon_colour = (colour) {0, 0, 0};
    ld_pen_up(ld, GSL_NAN, GSL_NAN, NULL, 1);
    cairo_set_source_rgb(s->cairo_draw, horizon_colour.red, horizon_colour.grn, horizon_colour.blu);
    cairo_set_line_width(s->cairo_draw, 4);

    // Draw line around the horizon
    plot_great_circle(ra_zenith_j2000 * 180 / M_PI, dec_zenith_j2000 * 180 / M_PI,
                      s, ld, page, 0, NULL, horizon_colour);

    // Draw cardinal point markers
    if (s->cardinals) {
        cairo_set_source_rgb(s->cairo_draw, s->horizon_cardinal_points_marker_colour.red,
                             s->horizon_cardinal_points_marker_colour.grn,
                             s->horizon_cardinal_points_marker_colour.blu);
        for (int cardinal_index = 0; cardinal_index < 16; cardinal_index++) {
            if ((s->horizon_cardinal_points_marker_count < 16) && ((cardinal_index % 2) != 0)) continue;
            if ((s->horizon_cardinal_points_marker_count < 8) && ((cardinal_index % 4) != 0)) continue;

            const double azimuth_central = 2 * M_PI * cardinal_index / 16;

            // Project centre of marker and calculate position angle of upward vector
            double x0, y0, x1, y1, x0_tangent, y0_tangent;
            project_alt_az_to_xy(s, 0, azimuth_central, &x0, &y0, &x0_tangent, &y0_tangent);
            project_alt_az_to_xy(s, 0.01, azimuth_central, &x1, &y1, NULL, NULL);
            const double pa = atan2(x1 - x0, y1 - y0);

            // If markers fall off the bottom of the chart, we may be requested to elevate them to make them visible
            const double x_centre = x0;
            double y_centre = y0;
            const double x_centre_tangent = x0_tangent;
            double y_centre_tangent = y0_tangent;
            if (s->horizon_cardinal_points_marker_elevate) {
                const double y_bottom = (s->width * s->aspect + s->canvas_offset_y) * s->cm; // Cairo coordinates
                if (y_centre > y_bottom) {
                    y_centre = y_bottom;
                    y_centre_tangent = s->y_max;
                }
            }

            // Calculate vertices of triangular marker
            const double marker_half_size = 0.25 * s->cm * s->horizon_cardinal_points_marker_size;
            const double mx0 = x_centre + marker_half_size * sin(pa - M_PI / 2);
            const double my0 = y_centre + marker_half_size * cos(pa - M_PI / 2);
            const double mx1 = x_centre + marker_half_size * sin(pa + M_PI / 2);
            const double my1 = y_centre + marker_half_size * cos(pa + M_PI / 2);
            const double mx2 = x_centre + marker_half_size * sin(pa) * 1.5;
            const double my2 = y_centre + marker_half_size * cos(pa) * 1.5;

            const double label_rotation = 180 - pa * 180 / M_PI;
            const double label_offset = marker_half_size * 3;

            // Draw triangular marker
            cairo_new_path(s->cairo_draw);
            cairo_move_to(s->cairo_draw, mx0, my0);
            cairo_line_to(s->cairo_draw, mx1, my1);
            cairo_line_to(s->cairo_draw, mx2, my2);
            cairo_close_path(s->cairo_draw);
            cairo_fill(s->cairo_draw);

            // Write label above marker
            chart_label_buffer(page, s, s->horizon_cardinal_points_labels_colour, cardinal_label[cardinal_index],
                               (label_position[1]) {
                                       {x_centre_tangent, y_centre_tangent, label_rotation,
                                        label_offset * sin(pa), label_offset * cos(pa),
                                        0, 0}
                               }, 1,
                               0, 0, 2.5 * s->label_font_size_scaling,
                               0, 0, 0, -10);
        }
    }

    // Re-enable horizon clipping
    s->show_horizon = show_horizon;
}
