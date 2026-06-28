// deepSkyOutlines.c
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
#include <glob.h>
#include <wordexp.h>

#include <gsl/gsl_math.h>

#include "astroGraphics/deepSkyOutlines.h"
#include "coreUtils/asciiDouble.h"
#include "coreUtils/errorReport.h"
#include "mathsTools/projection.h"
#include "settings/chart_config.h"
#include "vectorGraphics/cairo_page.h"

#define DSO_MAX_POINTS 4096
#define DSO_MAX_OUTLINES 4096

//! DsoPoint - A single point in a deep sky object outline
typedef struct {
    double ra;   // degrees
    double dec;  // degrees
    int continuous; // 1 if this point continues from the previous, 0 if it starts a new sub-path
} DsoPoint;

//! DsoOutline - All points belonging to one deep sky object outline
typedef struct {
    DsoPoint points[DSO_MAX_POINTS];
    int point_count;
} DsoOutline;

//! DsoOutlineCollection - The full set of loaded outlines
typedef struct {
    DsoOutline outlines[DSO_MAX_OUTLINES];
    int outline_count;
    int loaded;
} DsoOutlineCollection;

// Global collection, loaded once at startup
static DsoOutlineCollection dso_collection = { .loaded = 0, .outline_count = 0 };


//! close_dso_outline - Close the path around a deep sky object
//! \param s - A <chart_config> structure defining the properties of the star chart to be drawn.

void close_dso_outline(chart_config *s) {
    cairo_set_source_rgba(s->cairo_draw,
                          s->dso_nebula_col.red, s->dso_nebula_col.grn, s->dso_nebula_col.blu,
                          s->dso_nebula_col.alpha);
    cairo_fill_preserve(s->cairo_draw);
    cairo_set_source_rgba(s->cairo_draw,
                          s->dso_outline_col.red, s->dso_outline_col.grn, s->dso_outline_col.blu,
                          s->dso_outline_col.alpha);
    cairo_set_line_width(s->cairo_draw, 0.6 * s->line_width_base);
    cairo_stroke(s->cairo_draw);
}


//! load_dso_outlines - Load all deep sky object outline files into memory.
//! Should be called once before any charts are rendered.

void load_dso_outlines() {
    if (dso_collection.loaded) return;
    dso_collection.loaded = 1;
    dso_collection.outline_count = 0;

    wordexp_t w;
    glob_t g;

    const char *outlines_path = SRCDIR "/../data/deepSky/ngc/outlines/*.txt";

    if (wordexp(outlines_path, &w, 0) != 0) return;

    for (int i = 0; i < w.we_wordc; i++) {
        if (glob(w.we_wordv[i], 0, NULL, &g) != 0) continue;

        for (int j = 0; j < g.gl_pathc; j++) {
            if (dso_collection.outline_count >= DSO_MAX_OUTLINES) {
                stch_log("Warning: DSO_MAX_OUTLINES reached; some outlines will not be loaded.");
                break;
            }

            const char *outline_file = g.gl_pathv[j];

            if (DEBUG) {
                char message[FNAME_LENGTH];
                snprintf(message, FNAME_LENGTH, "Loading DSO outline from <%s>", outline_file);
                stch_log(message);
            }

            FILE *file = fopen(outline_file, "r");
            if (file == NULL) {
                char message[FNAME_LENGTH];
                snprintf(message, FNAME_LENGTH, "Could not open deep sky object outline <%s>; skipping.", outline_file);
                stch_log(message);
                continue;
            }

            DsoOutline *outline = &dso_collection.outlines[dso_collection.outline_count];
            outline->point_count = 0;

            while ((!feof(file)) && (!ferror(file))) {
                if (outline->point_count >= DSO_MAX_POINTS) break;

                char line[FNAME_LENGTH];
                const char *line_ptr = line;
                file_readline(file, line, sizeof line);

                if (line[0] != 'l') continue;

                DsoPoint *pt = &outline->points[outline->point_count];

                line_ptr = next_word(line_ptr);
                pt->continuous = (line_ptr[0] == '+');
                line_ptr = next_word(line_ptr);
                pt->ra = get_float(line_ptr, NULL);
                line_ptr = next_word(line_ptr);
                pt->dec = get_float(line_ptr, NULL);

                outline->point_count++;
            }

            fclose(file);

            if (outline->point_count > 0) {
                dso_collection.outline_count++;
            }
        }

        globfree(&g);
    }

    wordfree(&w);

    if (DEBUG) {
        char message[FNAME_LENGTH];
        snprintf(message, FNAME_LENGTH, "Loaded %d DSO outlines into memory.", dso_collection.outline_count);
        stch_log(message);
    }
}


//! plot_deep_sky_outlines - Draw outlines of deep sky objects onto a star chart.
//! Assumes load_dso_outlines() has already been called.
//! \param s - A <chart_config> structure defining the properties of the star chart to be drawn.
//! \param page - A <cairo_page> structure defining the cairo drawing context.

void plot_deep_sky_outlines(chart_config *s, cairo_page *page) {

    // We only plot DSO outlines in the 'coloured' plot style
    if (s->dso_style == SW_DSO_STYLE_FUZZY) return;

    // Ensure outlines are loaded (safe to call multiple times)
    load_dso_outlines();

    // Iterate over all loaded outlines
    for (int c = 0; c < dso_collection.outline_count; c++) {
        DsoOutline *outline = &dso_collection.outlines[c];

        // Project all points for this chart and check visibility
        double x_canvas[DSO_MAX_POINTS], y_canvas[DSO_MAX_POINTS];
        int reject_object = 0;

        for (int k = 0; k < outline->point_count; k++) {
            double x, y;
            plane_project(&x, &y, s,
                          outline->points[k].ra  * M_PI / 180,
                          outline->points[k].dec * M_PI / 180, 0);

            if ((!gsl_finite(x)) || (!gsl_finite(y))) {
                reject_object = 1;
                break;
            }

            if ((x < s->x_min * 1.2) || (x > s->x_max * 1.2) ||
                (y < s->y_min * 1.2) || (y > s->y_max * 1.2)) {
                reject_object = 1;
                break;
            }

            fetch_canvas_coordinates(&x_canvas[k], &y_canvas[k], x, y, s);

            if (k > 0) {
                double line_length = hypot(y_canvas[k - 1] - y_canvas[k],
                                           x_canvas[k - 1] - x_canvas[k]);
                if (line_length > 100) {
                    reject_object = 1;
                    break;
                }
            }
        }

        if (reject_object) continue;

        // Draw the outline
        cairo_new_path(s->cairo_draw);
        int line_point_counter = 0;

        for (int k = 0; k < outline->point_count; k++) {
            if (line_point_counter == 0) {
                cairo_move_to(s->cairo_draw, x_canvas[k], y_canvas[k]);
            } else {
                cairo_line_to(s->cairo_draw, x_canvas[k], y_canvas[k]);
            }

            if (!outline->points[k].continuous) {
                close_dso_outline(s);
                line_point_counter = 0;
            }

            line_point_counter++;
        }

        if (line_point_counter > 0) {
            close_dso_outline(s);
        }
    }
}
