// raDecLines.c
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

#include <gsl/gsl_math.h>

#include "coreUtils/errorReport.h"
#include "mathsTools/projection.h"
#include "settings/chart_config.h"
#include "vectorGraphics/lineDraw.h"
#include "vectorGraphics/cairo_page.h"

// Number of samples to make along lines of constant RA and Dec
#define N_POINTS 720

// Small number added to all angles before labels are converted to text, to avoid rounding causing 23o59' to appear
#define EPSILON_ANGLE 1e-10

//! deg - Convert angles in radians into degrees
//! \param x - Angle in radians
//! \return - Angle in degrees

static double deg(double x) { return x / M_PI * 180; }

//! plot_ra_dec_lines - Trace lines of constant RA and Dec (J2000) onto a star chart
//! \param s - A <chart_config> structure defining the properties of the star chart to be drawn.
//! \param ld - A <line_drawer> structure used to draw lines on a cairo surface.

void plot_ra_dec_lines(chart_config *s, line_drawer *ld) {
    int i, j;
    char label[FNAME_LENGTH];

    // Work out how many lines we are going to draw
    double degrees_per_cm = (s->angular_width * 180 / M_PI) / s->width;
    int ra_line_count = 24;
    int dec_line_count = 18;

    if (degrees_per_cm < 0.1) {
        ra_line_count *= 60;
        dec_line_count *= 50;
    } else if (degrees_per_cm < 0.2) {
        ra_line_count *= 30;
        dec_line_count *= 20;
    } else if (degrees_per_cm < 0.38) {
        ra_line_count *= 12;
        dec_line_count *= 10;
    } else if (degrees_per_cm < 0.65) {
        ra_line_count *= 6;
        dec_line_count *= 5;
    } else if (degrees_per_cm < 1) {
        ra_line_count *= 4;
        dec_line_count *= 4;
    } else if (degrees_per_cm < 2) {
        ra_line_count *= 2;
        dec_line_count *= 2;
    }

    // Debugging info about how many lines we have chosen to draw
    if (DEBUG) {
        char message[FNAME_LENGTH];
        snprintf(message, FNAME_LENGTH, "Scale of star chart: %.6f deg/cm", degrees_per_cm);
        stch_log(message);
        snprintf(message, FNAME_LENGTH, "Number of RA lines: %4d", ra_line_count);
        stch_log(message);
        snprintf(message, FNAME_LENGTH, "Number of Dec lines: %4d", dec_line_count);
        stch_log(message);
    }

    // Set line colour
    ld_pen_up(ld, GSL_NAN, GSL_NAN, NULL, 1);
    cairo_set_source_rgb(s->cairo_draw, s->grid_col.red, s->grid_col.grn, s->grid_col.blu);
    cairo_set_line_width(s->cairo_draw, s->coordinate_grid_line_width);

    // Set dashed line style
    double dash_style[1] = {0.5 * s->mm};
    cairo_set_dash(s->cairo_draw, dash_style, 1, 0);

    // Draw lines of constant RA (or galactic longitude)
    for (i = 0; i < ra_line_count; i++) {
        // Work out the RA value this line should trace
        double ra = (2 * M_PI) * ((double) i) / ra_line_count;
        ra += ((ra >= 0) ? 1 : -1) * EPSILON_ANGLE;

        // Work out what text label should appear on the axes of the star chart where this line meets them
        if (s->coords == SW_COORDS_GAL) {
            // Case 1: We are tracing galactic coordinates
            // Depending how many lines we are drawing, the ticks may need to show arcmin as well as degrees
            if (floor(360. / ra_line_count) == (360. / ra_line_count))
                snprintf(label, FNAME_LENGTH,
                         "%s%d°",
                         s->axis_ticks_value_only ? "" : "l=",
                         (int) floor(deg(ra)));
            else
                snprintf(label, FNAME_LENGTH,
                         "%s%d°%02d'",
                         s->axis_ticks_value_only ? "" : "l=",
                         (int) floor(deg(ra)),
                         (int) floor(fmod(deg(ra) * 60, 60)));
        } else {
            // Case 2: We are tracing RA / Dec
            // Depending how many lines we are drawing, the ticks may need to show minutes as well as hours
            if (floor(24. / ra_line_count) == (24. / ra_line_count))
                snprintf(label, FNAME_LENGTH,
                         "%s%dʰ",
                         s->axis_ticks_value_only ? "" : "α=",
                         (int) floor(deg(ra) / 360. * 24.));
            else if (floor(24. * 60. / ra_line_count) == (24. * 60. / ra_line_count))
                snprintf(label, FNAME_LENGTH,
                         "%s%dʰ%02dᵐ",
                         s->axis_ticks_value_only ? "" : "α=",
                         (int) floor(deg(ra) / 360. * 24.),
                         (int) floor(fmod(deg(ra) / 360. * 24. * 60., 60.)));
            else
                snprintf(label, FNAME_LENGTH,
                         "%s%dʰ%02dᵐ%02dˢ",
                         s->axis_ticks_value_only ? "" : "α=",
                         (int) floor(deg(ra) / 360. * 24.),
                         (int) floor(fmod(deg(ra) / 360. * 24. * 60., 60.)),
                         (int) floor(fmod(deg(ra) / 360. * 24. * 3600., 60.)));

        }

        // Tell the line drawing class what label should be placed on the edge of the star chart when we cross it
        // Lines of constant RA only create tick labels on the horizontal axes, not the vertical axes
        ld_label(ld, label, 1, 0);

        // Trace the path of this line across chart
        for (j = 0; j < N_POINTS; j++) {
            // Scan through declinations from -90 to +90
            double dec = -M_PI / 2 + M_PI * ((double) j) / (N_POINTS - 1);

            // Project this point onto the drawing canvas
            double x, y;
            plane_project(&x, &y, s, ra, dec, 1);

            // Add this point to the line we are drawing
            ld_point(ld, x, y, NULL);
        }

        // Lift up the pen at the end of the line
        ld_pen_up(ld, GSL_NAN, GSL_NAN, NULL, 1);
    }

    // Now draw lines of constant Dec (or galactic latitude)
    for (i = 0; i < (dec_line_count - 1); i++) {
        // Work out the declination value this line should trace
        double dec = -M_PI / 2 + M_PI * ((double) (i + 1)) / dec_line_count;
        char *sgn; // String containing the sign of the declination
        double dmg; // Declination magnitude (unsigned)
        dec += ((dec >= 0) ? 1 : -1) * EPSILON_ANGLE;
        sgn = (dec < 0) ? "–" : ""; // Use a nice long n-dash to represent minus signs
        dmg = fabs(dec); // This is the magnitude of the declination

        // Work out what text label should appear on the axes of the star chart where this line meets them
        if (s->coords == SW_COORDS_GAL) {
            // Case 1: We are tracing galactic coordinates
            // Depending how many lines we are drawing, the ticks may need to show arcmin as well as degrees
            if (floor(180. / dec_line_count) == (180. / dec_line_count))
                snprintf(label, FNAME_LENGTH,
                         "%s%s%d°",
                         s->axis_ticks_value_only ? "" : "b=",
                         sgn,
                         (int) floor(deg(dmg)));
            else
                snprintf(label, FNAME_LENGTH,
                         "%s%s%d°%02d}'",
                         s->axis_ticks_value_only ? "" : "b=",
                         sgn,
                         (int) floor(deg(dmg)),
                         (int) floor(fmod(deg(dmg) * 60, 60)));
        } else if ((s->projection == SW_PROJECTION_PETERS) &&
                   ((fabs(dec) > 61 * M_PI / 180) && (fabs(dec) < 79 * M_PI / 180))) {
            // On Peters projection, omit labels between 60 deg and 80 deg, as space is a bit tight
            strcpy(label, "");
        } else {
            // Case 2: We are tracing RA / Dec
            // Depending how many lines we are drawing, the ticks may need to show minutes as well as hours
            if (floor(180. / dec_line_count) == (180. / dec_line_count))
                snprintf(label, FNAME_LENGTH,
                         "%s%s%d°",
                         s->axis_ticks_value_only ? "" : "δ=",
                         sgn,
                         (int) floor(deg(dmg)));
            else
                snprintf(label, FNAME_LENGTH,
                         "%s%s%d°%02d'",
                         s->axis_ticks_value_only ? "" : "δ=",
                         sgn,
                         (int) floor(deg(dmg)),
                         (int) floor(fmod(deg(dmg) * 60, 60)));
        }

        // Tell the line drawing class what label should be placed on the edge of the star chart when we cross it
        // Lines of constant declination only create tick labels on the vertical axes, not the horizontal axes
        ld_label(ld, label, 0, 1);

        // Trace the path of this line across chart
        for (j = 0; j < N_POINTS; j++) {
            // Scan through right ascensions from 0 to 24 hours
            double ra = (2 * M_PI) * ((double) j) / (N_POINTS - 1);

            // Project this point onto the drawing canvas
            double x, y;
            plane_project(&x, &y, s, ra, dec, 1);

            // Add this point to the line we are drawing
            ld_point(ld, x, y, NULL);
        }

        // Lift up the pen at the end of the line
        ld_pen_up(ld, GSL_NAN, GSL_NAN, NULL, 1);
    }

    // Unset dashed line style
    cairo_set_dash(s->cairo_draw, NULL, 0, 0);
}
