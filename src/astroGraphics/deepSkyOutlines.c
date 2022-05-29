// deepSkyOutlines.c
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
#include <glob.h>
#include <wordexp.h>

#include <gsl/gsl_math.h>

#include "astroGraphics/deepSkyOutlines.h"
#include "coreUtils/asciiDouble.h"
#include "coreUtils/errorReport.h"
#include "mathsTools/projection.h"
#include "settings/chart_config.h"
#include "vectorGraphics/cairo_page.h"

//! close_dso_outline - Close the path around a deep sky object
//! \param s - A <chart_config> structure defining the properties of the star chart to be drawn.

void close_dso_outline(chart_config *s) {
    cairo_set_source_rgb(s->cairo_draw, s->dso_nebula_col.red, s->dso_nebula_col.grn,
                         s->dso_nebula_col.blu);
    cairo_fill_preserve(s->cairo_draw);
    cairo_set_source_rgb(s->cairo_draw, s->dso_outline_col.red, s->dso_outline_col.grn, s->dso_outline_col.blu);
    cairo_set_line_width(s->cairo_draw, 0.8);
    cairo_stroke(s->cairo_draw);
}

//! plot_deep_sky_outlines - Draw outlines of deep sky objects onto a star chart
//! \param s - A <chart_config> structure defining the properties of the star chart to be drawn.
//! \param page - A <cairo_page> structure defining the cairo drawing context.

void plot_deep_sky_outlines(chart_config *s, cairo_page *page) {
    wordexp_t w;
    glob_t g;

    // Path to where deep sky object outlines are stored
    const char *outlines_path = SRCDIR "/../data/deepSky/ngc/outlines/*.txt";

    // Fetch list of all the deep sky object outlines we have
    if (wordexp(outlines_path, &w, 0) != 0) return; // No matches; return empty list
    for (int i = 0; i < w.we_wordc; i++) {
        if (glob(w.we_wordv[i], 0, NULL, &g) != 0) continue;
        for (int j = 0; j < g.gl_pathc; j++) {
            // Deep sky object outline we are processing
            const char *outline_file = g.gl_pathv[j];

            // Logging message
            if (DEBUG) {
                char message[FNAME_LENGTH];
                snprintf(message, FNAME_LENGTH, "Drawing outline from <%s>", outline_file);
                stch_log(message);
            }

            // Open file
            FILE *file = fopen(outline_file, "r");
            if (file == NULL) stch_fatal(__FILE__, __LINE__, "Could not open deep sky object outline");

            // Initially, read path into an array
            const int max_points = 4096;
            double ra[max_points], dec[max_points];
            double x[max_points], y[max_points];
            double x_canvas[max_points], y_canvas[max_points];
            int continuous[max_points];

            // Loop over the lines of the data file
            int point_counter = 0;
            int reject_object = 0;
            while ((!feof(file)) && (!ferror(file))) {
                char line[FNAME_LENGTH];
                const char *line_ptr = line;

                file_readline(file, line);

                // Ignore comment lines
                if (line[0] != 'l') continue;

                // Extract data from line
                line_ptr = next_word(line_ptr);
                continuous[point_counter] = (line_ptr[0] == '+');
                line_ptr = next_word(line_ptr);
                ra[point_counter] = get_float(line_ptr, NULL);
                line_ptr = next_word(line_ptr);
                dec[point_counter] = get_float(line_ptr, NULL);

                // Project RA and Dec of object into physical coordinates on the star chart
                plane_project(&x[point_counter], &y[point_counter], s,
                              ra[point_counter] * M_PI / 180, dec[point_counter] * M_PI / 180, 0);

                // Reject this object if it falls outside the plot area
                if ((!gsl_finite(x[point_counter])) || (!gsl_finite(y[point_counter]))) {
                    reject_object = 1;
                    break;
                }

                if (
                        (x[point_counter] < s->x_min * 1.2) || (x[point_counter] > s->x_max * 1.2) ||
                        (y[point_counter] < s->y_min * 1.2) || (y[point_counter] > s->y_max * 1.2)
                        ) {
                    reject_object = 1;
                    break;
                }

                // Convert coordinates from tangent plane into pixels on the Cairo canvas
                fetch_canvas_coordinates(&x_canvas[point_counter], &y_canvas[point_counter],
                                         x[point_counter], y[point_counter], s);

                // Check that we haven't jumped off one side of star chart, and on the other side
                if (point_counter > 0) {
                    double line_length = hypot(y_canvas[point_counter - 1] - y_canvas[point_counter],
                                               x_canvas[point_counter - 1] - x_canvas[point_counter]);
                    if (line_length > 100) reject_object = 1;
                }

                // Update point counter
                point_counter++;
            }

            // If this object has been rejected, ignore it
            if (reject_object) continue;

            // Start drawing a path around the outline of this object
            cairo_new_path(s->cairo_draw);

            // Loop over all the points in the path
            int line_point_counter = 0;
            for (int k = 0; k < point_counter; k++) {

                // Either continue an existing line, or start a new path
                if (line_point_counter == 0) {
                    cairo_move_to(s->cairo_draw, x_canvas[k], y_canvas[k]);
                } else {
                    cairo_line_to(s->cairo_draw, x_canvas[k], y_canvas[k]);
                }

                // Close path, if requested
                if (!continuous[k]) {
                    close_dso_outline(s);
                    line_point_counter = 0;
                }

                // Update point counter
                line_point_counter++;
            }

            // Finally, stroke and fill path
            if (line_point_counter > 0) {
                close_dso_outline(s);
            }

            // Close outline file
            fclose(file);
        }
        globfree(&g);
    }
    wordfree(&w);
}
