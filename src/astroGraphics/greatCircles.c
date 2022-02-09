// ephemeris.c
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

#include "astroGraphics/ephemeris.h"
#include "coreUtils/asciiDouble.h"
#include "coreUtils/errorReport.h"
#include "mathsTools/projection.h"
#include "mathsTools/sphericalTrig.h"
#include "settings/chart_config.h"
#include "vectorGraphics/lineDraw.h"
#include "vectorGraphics/cairo_page.h"

//! The number of samples along the length of a great circle to be plotted across the sky
#define N_SAMPLES 720

//! A label to place along the length of a great circle
typedef struct {
    char label[16];
    double xpos;
} gc_label;

//! plot_great_circle - Plot a great circle across the sky
//! \param ra0 - The right ascension of the pole that the great circle lies perpendicular to (degrees)
//! \param dec0 - The declination of the pole that the great circle lies perpendicular to (degrees)
//! \param s - A <chart_config> structure defining the properties of the star chart to be drawn.
//! \param ld - A <line_drawer> structure used to draw lines on a cairo surface.
//! \param page - A <cairo_page> structure defining the cairo drawing context.
//! \param n_labels - The number of text labels to place along the length of the great circle.
//! \param labels - An array of <gc_label> structures defining the labels to draw.
//! \param colour - The colour to use to paint the great circle.

static void plot_great_circle(double ra0, double dec0, chart_config *s, line_drawer *ld,
                              cairo_page *page, int n_labels, gc_label *labels, colour colour) {
    int i;
    ra0 = ra0 * M_PI / 180;
    dec0 = dec0 * M_PI / 180;
    ld_pen_up(ld, GSL_NAN, GSL_NAN, NULL, 1);
    for (i = 0; i <= N_SAMPLES; i++) {
        const double l = 2 * M_PI * ((double) i) / N_SAMPLES;
        double a[3] = {cos(l), sin(l), 0.};
        double ra, dec, x, y;

        rotate_xz(a, a, dec0 - (M_PI / 2));
        rotate_xy(a, a, ra0);

        dec = asin(a[2]);
        ra = atan2(a[1], a[0]);
        plane_project(&x, &y, s, ra, dec, 0);
        ld_point(ld, x, y, NULL);
    }
    ld_pen_up(ld, GSL_NAN, GSL_NAN, NULL, 1);

    if (n_labels)
        for (i = 0; i < n_labels; i++) {
            const double l = 2 * M_PI * (labels[i].xpos) / 365.2524;
            double a[3] = {cos(l), sin(l), 0.};
            double ra, dec, x, y;

            rotate_xz(a, a, dec0 - (M_PI / 2));
            rotate_xy(a, a, ra0);

            dec = asin(a[2]);
            ra = atan2(a[1], a[0]);
            plane_project(&x, &y, s, ra, dec, 0);
            ld_point(ld, x, y + 0.035, NULL);
            ld_point(ld, x, y - 0.035, NULL);
            ld_pen_up(ld, GSL_NAN, GSL_NAN, NULL, 1);

            chart_label_buffer(page, s, colour, labels[i].label,
                               &(label_position) {x, y + 0.045, 0, 0, -1}, 1,
                               0, 1, 2.0, 1, 0, 0, -0.5);
        }
}

//! plot_equator - Draw a line along the celestial equator
//! \param s - A <chart_config> structure defining the properties of the star chart to be drawn.
//! \param ld - A <line_drawer> structure used to draw lines on a cairo surface.
//! \param page - A <cairo_page> structure defining the cairo drawing context.

void plot_equator(chart_config *s, line_drawer *ld, cairo_page *page) {
    // Set line colour
    ld_pen_up(ld, GSL_NAN, GSL_NAN, NULL, 1);
    cairo_set_source_rgb(s->cairo_draw, s->equator_col.red, s->equator_col.grn, s->equator_col.blu);
    cairo_set_line_width(s->cairo_draw, s->great_circle_line_width);

    plot_great_circle(0, 90, s, ld, page, 0, NULL, s->equator_col);
}

//! plot_galactic_plane - Draw a line along the plane of the Milky Way
//! \param s - A <chart_config> structure defining the properties of the star chart to be drawn.
//! \param ld - A <line_drawer> structure used to draw lines on a cairo surface.
//! \param page - A <cairo_page> structure defining the cairo drawing context.

void plot_galactic_plane(chart_config *s, line_drawer *ld, cairo_page *page) {
    // Set line colour
    ld_pen_up(ld, GSL_NAN, GSL_NAN, NULL, 1);
    cairo_set_source_rgb(s->cairo_draw, s->galactic_plane_col.red, s->galactic_plane_col.grn,
                         s->galactic_plane_col.blu);
    cairo_set_line_width(s->cairo_draw, s->great_circle_line_width);

    plot_great_circle((12. + 51. / 60 + 26.282 / 3600.) / 24. * 360., (27. + 7.0 / 60. + 42.01 / 3600.), s,
                      ld, page, 0, NULL, s->galaxy_col);
}

//! plot_ecliptic - Draw a line along the ecliptic (i.e. the zodiac; the path the Sun follows across the sky)
//! \param s - A <chart_config> structure defining the properties of the star chart to be drawn.
//! \param ld - A <line_drawer> structure used to draw lines on a cairo surface.
//! \param page - A <cairo_page> structure defining the cairo drawing context.

void plot_ecliptic(chart_config *s, line_drawer *ld, cairo_page *page) {
    gc_label labels[12] = {{"Jan", 11},
                           {"Feb", 42},
                           {"Mar", 70},
                           {"Apr", 101},
                           {"May", 131},
                           {"Jun", 162},
                           {"Jul", 192},
                           {"Aug", 223},
                           {"Sep", 254},
                           {"Oct", 284},
                           {"Nov", 315},
                           {"Dec", 345}};

    // Set line colour
    ld_pen_up(ld, GSL_NAN, GSL_NAN, NULL, 1);
    cairo_set_source_rgb(s->cairo_draw, s->ecliptic_col.red, s->ecliptic_col.grn, s->ecliptic_col.blu);
    cairo_set_line_width(s->cairo_draw, s->great_circle_line_width);

    plot_great_circle(18. / 24. * 360., 90. - 23.4, s, ld, page, s->label_ecliptic ? 12 : 0, labels,
                      s->ecliptic_col);
}

//! draw_great_circle_key - Draw a legend below the star chart indicating the colours of the lines representing great
//! circles.
//! \param s - A <chart_config> structure defining the properties of the star chart to be drawn.
//! \param legend_y_pos - The vertical pixel position of the top of the next legend to go under the star chart.

double draw_great_circle_key(chart_config *s, double legend_y_pos) {
    const int N = (s->plot_equator != 0) + (s->plot_ecliptic != 0) + (s->plot_galactic_plane != 0);
    const double w_left = 0.4; // The left margin
    const double w_item = 4.2 * s->font_size; // The width of each legend item (cm)

    // The top (y0) and bottom (y1) of the legend
    const double y0 = legend_y_pos;
    const double y1 = y0 - 0.4;

    // The horizontal position of the centre of the legend
    const double x1 = s->canvas_offset_x + (s->width - s->legend_right_column_width) / 2;

    // The width of the legend
    const double xw = w_left * 1.5 + w_item * N;
    const double size = 0.6;

    // The left edge of the legend
    double x = x1 - xw / 2;
    cairo_text_extents_t extents;

    cairo_set_font_size(s->cairo_draw, 3.6 * s->mm * s->font_size);
    cairo_set_line_width(s->cairo_draw, 2.5 * s->line_width_base);

    x += w_left;

    // Reset font weight
    cairo_select_font_face(s->cairo_draw, s->font_family, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);

    // Draw item on legend showing the colour of the equator
    if (s->plot_equator != 0) {
        const char *label = "The Equator";

        // Draw a line in the right colour
        cairo_set_source_rgb(s->cairo_draw, s->equator_col.red, s->equator_col.grn, s->equator_col.blu);
        cairo_new_path(s->cairo_draw);
        cairo_move_to(s->cairo_draw, x * s->cm, y1 * s->cm);
        cairo_line_to(s->cairo_draw, (x + size) * s->cm, y1 * s->cm);
        cairo_stroke(s->cairo_draw);

        // Write a text label next to it
        cairo_set_source_rgb(s->cairo_draw, 0, 0, 0);
        cairo_text_extents(s->cairo_draw, label, &extents);
        cairo_move_to(s->cairo_draw,
                      (x + 0.1 + size * 1.25) * s->cm - extents.x_bearing,
                      y1 * s->cm - extents.height / 2 - extents.y_bearing
        );
        cairo_show_text(s->cairo_draw, label);

        // Advance horizontally to draw the next item in the legend
        x += w_item;
    }

    // Draw item on legend showing the colour of the ecliptic
    if (s->plot_ecliptic != 0) {
        const char *label = "Ecliptic Plane";

        // Draw a line in the right colour
        cairo_set_source_rgb(s->cairo_draw, s->ecliptic_col.red, s->ecliptic_col.grn, s->ecliptic_col.blu);
        cairo_new_path(s->cairo_draw);
        cairo_move_to(s->cairo_draw, x * s->cm, y1 * s->cm);
        cairo_line_to(s->cairo_draw, (x + size) * s->cm, y1 * s->cm);
        cairo_stroke(s->cairo_draw);

        // Write a text label next to it
        cairo_set_source_rgb(s->cairo_draw, 0, 0, 0);
        cairo_text_extents(s->cairo_draw, label, &extents);
        cairo_move_to(s->cairo_draw,
                      (x + 0.1 + size * 1.25) * s->cm - extents.x_bearing,
                      y1 * s->cm - extents.height / 2 - extents.y_bearing
        );
        cairo_show_text(s->cairo_draw, label);

        // Advance horizontally to draw the next item in the legend
        x += w_item;
    }

    // Draw item on legend showing the colour of the galactic plane
    if (s->plot_galactic_plane != 0) {
        const char *label = "Galactic Plane";

        // Draw a line in the right colour
        cairo_set_source_rgb(s->cairo_draw, s->galactic_plane_col.red, s->galactic_plane_col.grn,
                             s->galactic_plane_col.blu);
        cairo_new_path(s->cairo_draw);
        cairo_move_to(s->cairo_draw, x * s->cm, y1 * s->cm);
        cairo_line_to(s->cairo_draw, (x + size) * s->cm, y1 * s->cm);
        cairo_stroke(s->cairo_draw);

        // Write a text label next to it
        cairo_set_source_rgb(s->cairo_draw, 0, 0, 0);
        cairo_text_extents(s->cairo_draw, label, &extents);
        cairo_move_to(s->cairo_draw,
                      (x + 0.1 + size * 1.25) * s->cm - extents.x_bearing,
                      y1 * s->cm - extents.height / 2 - extents.y_bearing
        );
        cairo_show_text(s->cairo_draw, label);

        // Advance horizontally to draw the next item in the legend
        //x += w_item;
    }

    const double new_bottom_to_legend_items = y0 + 1.0;
    return new_bottom_to_legend_items;
}
