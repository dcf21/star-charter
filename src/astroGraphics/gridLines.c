// gridLines.c
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
#include "mathsTools/projection.h"
#include "settings/chart_config.h"
#include "vectorGraphics/lineDraw.h"

// Number of samples to make along lines of constant RA and Dec
#define POINTS_PER_DEGREE 4
#define N_POINTS (360 * POINTS_PER_DEGREE)

// Small number added to all angles before labels are converted to text, to avoid rounding causing 23o59' to appear
#define EPSILON_ANGLE 1e-9

//! deg - Convert angles in radians into degrees
//! \param x - Angle in radians
//! \return - Angle in degrees

static double deg(double x) {
    return x / M_PI * 180;
}

//! plot_grid_lines - Trace lines of constant {RA/Dec , Alt/Az or l/b} onto a star chart
//! \param s - A <chart_config> structure defining the properties of the star chart to be drawn.
//! \param ld - A <line_drawer> structure used to draw lines on a cairo surface.

void plot_grid_lines(chart_config *s, line_drawer *ld) {
    int i, j;
    char label[FNAME_LENGTH];

    // Work out how many lines we are going to draw
    const double degrees_per_cm = (s->angular_width * 180 / M_PI) / s->width;
    int lng_line_count = 24;
    int lat_line_count = 18;

    if (degrees_per_cm < 0.1) {
        lng_line_count *= 60;
        lat_line_count *= 50;
    } else if (degrees_per_cm < 0.2) {
        lng_line_count *= 30;
        lat_line_count *= 20;
    } else if (degrees_per_cm < 0.38) {
        lng_line_count *= 12;
        lat_line_count *= 10;
    } else if (degrees_per_cm < 0.65) {
        lng_line_count *= 6;
        lat_line_count *= 5;
    } else if (degrees_per_cm < 1) {
        lng_line_count *= 4;
        lat_line_count *= 4;
    } else if (degrees_per_cm < 2) {
        lng_line_count *= 2;
        lat_line_count *= 2;
    }

    // Apply multiplicative factor to grid density
    lat_line_count = (int) (lat_line_count * s->grid_line_density);
    lng_line_count = (int) (lng_line_count * s->grid_line_density);

    // Debugging info about how many lines we have chosen to draw
    if (DEBUG) {
        char message[FNAME_LENGTH];
        snprintf(message, FNAME_LENGTH, "Scale of star chart: %.6f deg/cm", degrees_per_cm);
        stch_log(message);
        snprintf(message, FNAME_LENGTH, "Number of RA lines: %4d", lng_line_count);
        stch_log(message);
        snprintf(message, FNAME_LENGTH, "Number of Dec lines: %4d", lat_line_count);
        stch_log(message);
    }

    // Set line colour
    ld_pen_up(ld, GSL_NAN, GSL_NAN, NULL, 1);
    cairo_set_source_rgba(s->cairo_draw,
                          s->grid_col.red, s->grid_col.grn, s->grid_col.blu,
                          s->grid_col.alpha);
    cairo_set_line_width(s->cairo_draw, s->coordinate_grid_line_width * s->line_width_base);

    // Set dashed line style
    double dash_style[1] = {0.5 * s->mm};
    cairo_set_dash(s->cairo_draw, dash_style, 1, 0);

    // Draw lines of constant RA (or longitude)
    for (i = 0; i < lng_line_count; i++) {
        // Work out the RA value this line should trace
        const double lng = (2 * M_PI) * ((double) i) / lng_line_count;
        const int lng_sgn = (lng >= 0) ? 1 : -1;
        const double lng_for_text = lng + lng_sgn * EPSILON_ANGLE;
        const double lng_for_calculation = lng - lng_sgn * EPSILON_ANGLE;

        // Work out what text label should appear on the axes of the star chart where this line meets them
        if ((s->grid_coords == SW_COORDS_GALACTIC) || (s->grid_coords == SW_COORDS_ALTAZ)) {
            // Case 1: We are tracing galactic coordinates
            // Depending on how many lines we are drawing, the ticks may need to show arcmin as well as degrees
            if (floor(360. / lng_line_count) == (360. / lng_line_count))
                snprintf(label, FNAME_LENGTH,
                         "%s%d°",
                         s->axis_ticks_value_only ? "" : "l=",
                         (int) floor(deg(lng_for_text)));
            else
                snprintf(label, FNAME_LENGTH,
                         "%s%d°%02d'",
                         s->axis_ticks_value_only ? "" : "l=",
                         (int) floor(deg(lng_for_text)),
                         (int) floor(fmod(deg(lng_for_text) * 60, 60)));
        } else {
            // Case 2: We are tracing RA / Dec
            // Depending on how many lines we are drawing, the ticks may need to show minutes as well as hours
            if (floor(24. / lng_line_count) == (24. / lng_line_count))
                snprintf(label, FNAME_LENGTH,
                         "%s%dh",
                         s->axis_ticks_value_only ? "" : "α=",
                         (int) floor(deg(lng_for_text) / 360. * 24.));
            else if (floor(24. * 60. / lng_line_count) == (24. * 60. / lng_line_count))
                snprintf(label, FNAME_LENGTH,
                         "%s%dh%02dm",
                         s->axis_ticks_value_only ? "" : "α=",
                         (int) floor(deg(lng_for_text) / 360. * 24.),
                         (int) floor(fmod(deg(lng_for_text) / 360. * 24. * 60., 60.)));
            else
                snprintf(label, FNAME_LENGTH,
                         "%s%dh%02dm%02ds",
                         s->axis_ticks_value_only ? "" : "α=",
                         (int) floor(deg(lng_for_text) / 360. * 24.),
                         (int) floor(fmod(deg(lng_for_text) / 360. * 24. * 60., 60.)),
                         (int) floor(fmod(deg(lng_for_text) / 360. * 24. * 3600., 60.)));

        }

        // Tell the line drawing class what label should be placed on the edge of the star chart when we cross it
        // Lines of constant RA only create tick labels on the horizontal axes, not the vertical axes
        ld_label(ld, label, 1, 0);

        // Trace the path of this line across chart
        for (j = 0; j < N_POINTS; j++) {
            // Scan through declinations from -90 to +90 (with a small margin to avoid instability)
            const double lat = -M_PI / 2 + M_PI * ((double) j) / (N_POINTS - 1);
            const int lat_sgn = (lat >= 0) ? 1 : -1;
            const double lat_for_calculation = lat - lat_sgn * EPSILON_ANGLE;

            // Convert coordinate to (RA, Dec)
            double ra, dec;
            convert_selected_coordinates_to_ra_dec(s, s->grid_coords,
                                                   lng_for_calculation, lat_for_calculation,
                                                   &ra, &dec);

            // Project this point onto the drawing canvas
            double x, y;
            plane_project(&x, &y, s, ra, dec, 0);

            // Add this point to the line we are drawing
            ld_point(ld, x, y, NULL);
        }

        // Lift the pen at the end of the line
        ld_pen_up(ld, GSL_NAN, GSL_NAN, NULL, 1);
    }

    // Now draw lines of constant declination (or latitude)
    for (i = 0; i < (lat_line_count - 1); i++) {
        // Work out the declination value this line should trace
        const double lat = -M_PI / 2 + M_PI * ((double) (i + 1)) / lat_line_count;
        const int lat_sgn = (lat >= 0) ? 1 : -1;
        const double lat_for_text = lat + lat_sgn * EPSILON_ANGLE;
        const double lat_for_calculation = lat - lat_sgn * EPSILON_ANGLE;

        const char *lat_sgn_char = (lat_for_text < 0) ? "–" : ""; // Use a nice long n-dash to represent minus signs
        const double lat_mag = fabs(lat_for_text); // This is the magnitude of the declination

        // Work out what text label should appear on the axes of the star chart where this line meets them
        if ((s->grid_coords == SW_COORDS_GALACTIC) || (s->grid_coords == SW_COORDS_ALTAZ)) {
            // Case 1: We are tracing galactic coordinates
            // Depending on how many lines we are drawing, the ticks may need to show arcmin as well as degrees
            if (floor(180. / lat_line_count) == (180. / lat_line_count))
                snprintf(label, FNAME_LENGTH,
                         "%s%s%d°",
                         s->axis_ticks_value_only ? "" : "b=",
                         lat_sgn_char,
                         (int) floor(deg(lat_mag)));
            else
                snprintf(label, FNAME_LENGTH,
                         "%s%s%d°%02d}'",
                         s->axis_ticks_value_only ? "" : "b=",
                         lat_sgn_char,
                         (int) floor(deg(lat_mag)),
                         (int) floor(fmod(deg(lat_mag) * 60, 60)));
        } else if ((s->projection == SW_PROJECTION_PETERS) &&
                   ((fabs(lat) > 61 * M_PI / 180) && (fabs(lat) < 79 * M_PI / 180))) {
            // On Peters projection, omit labels between 60 deg and 80 deg, as space is a bit tight
            strcpy(label, "");
        } else {
            // Case 2: We are tracing RA / Dec
            // Depending on how many lines we are drawing, the ticks may need to show minutes as well as hours
            if (floor(180. / lat_line_count) == (180. / lat_line_count))
                snprintf(label, FNAME_LENGTH,
                         "%s%s%d°",
                         s->axis_ticks_value_only ? "" : "δ=",
                         lat_sgn_char,
                         (int) floor(deg(lat_mag)));
            else
                snprintf(label, FNAME_LENGTH,
                         "%s%s%d°%02d'",
                         s->axis_ticks_value_only ? "" : "δ=",
                         lat_sgn_char,
                         (int) floor(deg(lat_mag)),
                         (int) floor(fmod(deg(lat_mag) * 60, 60)));
        }

        // Tell the line drawing class what label should be placed on the edge of the star chart when we cross it
        // Lines of constant declination only create tick labels on the vertical axes, not the horizontal axes
        ld_label(ld, label, 0, 1);

        // Trace the path of this line across chart
        for (j = 0; j < N_POINTS; j++) {
            // Scan through right ascensions from 0 to 24 hours
            const double lng = (2 * M_PI) * ((double) j) / (N_POINTS - 1);
            const int lng_sgn = (lng >= 0) ? 1 : -1;
            const double lng_for_calculation = lng - lng_sgn * EPSILON_ANGLE;

            // Convert coordinate to (RA, Dec)
            double ra, dec;
            convert_selected_coordinates_to_ra_dec(s, s->grid_coords,
                                                   lng_for_calculation, lat_for_calculation,
                                                   &ra, &dec);

            // Project this point onto the drawing canvas
            double x, y;
            plane_project(&x, &y, s, ra, dec, 0);

            // Add this point to the line we are drawing
            ld_point(ld, x, y, NULL);
        }

        // Lift the pen at the end of the line
        ld_pen_up(ld, GSL_NAN, GSL_NAN, NULL, 1);
    }

    // Unset dashed line style
    cairo_set_dash(s->cairo_draw, NULL, 0, 0);
}
