// raDecLines.c
// 
// -------------------------------------------------
// Copyright 2015-2019 Dominic Ford
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

#include "mathsTools/projection.h"
#include "settings/chart_config.h"
#include "vectorGraphics/lineDraw.h"
#include "vectorGraphics/cairo_page.h"

// Number of samples to make along lines of constant RA and Dec
#define N_POINTS 720

// Small number added to angles before labels are converted to text, to avoid rounding causing 23o59' to appear
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

    // Set line colour
    ld_pen_up(ld, GSL_NAN, GSL_NAN, NULL, 1);
    cairo_set_source_rgb(s->cairo_draw, s->grid_col.red, s->grid_col.grn, s->grid_col.blu);
    cairo_set_line_width(s->cairo_draw, 1.3);

    // Set dashed line style
    double dash_style[1] = {0.5*s->mm};
    cairo_set_dash(s->cairo_draw, dash_style, 1, 0);

    // Lines of constant RA (or galactic longitude)
    for (i = 0; i < s->ra_line_count; i++) {
        double ra = (2 * M_PI) * ((double) i) / s->ra_line_count;
        ra += ((ra >= 0) ? 1 : -1) * EPSILON_ANGLE;

        // Work out what text label should appear on the axes of the star chart where this line meets them
        if (s->coords == SW_COORDS_GAL) {
            // Case 1: Galactic coordinates
            if (floor(360. / s->ra_line_count) == (360. / s->ra_line_count))
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
            // Case 2: RA / Dec
            if (floor(24. / s->ra_line_count) == (24. / s->ra_line_count))
                snprintf(label, FNAME_LENGTH,
                        "%s%dʰ",
                        s->axis_ticks_value_only ? "" : "α=",
                        (int) floor(deg(ra) / 360. * 24.));
            else if (floor(24. * 60. / s->ra_line_count) == (24. * 60. / s->ra_line_count))
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
        ld_label(ld, label, 1, 0);

        // Trace the path of this line across chart
        for (j = 0; j < N_POINTS; j++) {
            double dec = -M_PI / 2 + M_PI * ((double) j) / (N_POINTS - 1);
            double x, y;
            plane_project(&x, &y, s, ra, dec, 1);
            ld_point(ld, x, y, NULL);
        }
        ld_pen_up(ld, GSL_NAN, GSL_NAN, NULL, 1);
    }

    // Lines of constant Dec (or galactic latitude)
    for (i = 0; i < (s->dec_line_count - 1); i++) {
        double dec = -M_PI / 2 + M_PI * ((double) (i + 1)) / s->dec_line_count;
        char *sgn;
        double dmg;
        dec += ((dec >= 0) ? 1 : -1) * EPSILON_ANGLE;
        sgn = (dec < 0) ? "-" : "";
        dmg = fabs(dec);

        // Work out what text label should appear on the axes of the star chart where this line meets them
        if (s->coords == SW_COORDS_GAL) {
            // Case 1: Galactic coordinates
            if (floor(180. / s->dec_line_count) == (180. / s->dec_line_count))
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
            // Case 2: RA / Dec
            if (floor(180. / s->dec_line_count) == (180. / s->dec_line_count))
                snprintf(label,  FNAME_LENGTH,
                        "%s%s%d°",
                        s->axis_ticks_value_only ? "" : "δ=",
                        sgn,
                        (int) floor(deg(dmg)));
            else
                snprintf(label,  FNAME_LENGTH,
                        "%s%s%d°%02d'",
                        s->axis_ticks_value_only ? "" : "δ=",
                        sgn,
                        (int) floor(deg(dmg)),
                        (int) floor(fmod(deg(dmg) * 60, 60)));
        }
        ld_label(ld, label, 0, 1);

        // Trace the path of this line across chart
        for (j = 0; j < N_POINTS; j++) {
            double ra = (2 * M_PI) * ((double) j) / (N_POINTS - 1);
            double x, y;
            plane_project(&x, &y, s, ra, dec, 1);
            ld_point(ld, x, y, NULL);
        }
        ld_pen_up(ld, GSL_NAN, GSL_NAN, NULL, 1);
    }

    // Unset dashed line style
    cairo_set_dash(s->cairo_draw, NULL, 0, 0);
}
