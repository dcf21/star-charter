// arrowDraw.c
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

#include <math.h>

#include <gsl/gsl_math.h>

#include "settings/chart_config.h"
#include "vectorGraphics/arrowDraw.h"

//! draw_arrow - Draw an arrow on the Cairo context.
//! \param [in] s - A <chart_config> structure defining the properties of the star chart to be drawn.
//! \param [in] lw - The line width to use when stroking the arrow stalk.
//! \param [in] head_start - Boolean (0 or 1) indicating whether to put an arrow head at (x0, y0)
//! \param [in] head_end - Boolean (0 or 1) indicating whether to put an arrow head at (x1, y1)
//! \param [in] x0 - X coordinate of start of arrow
//! \param [in] y0 - Y coordinate of start of arrow
//! \param [in] x1 - X coordinate of end of arrow
//! \param [in] y1 - Y coordinate of end of arrow

void draw_arrow(chart_config *s, double lw, int head_start, int head_end,
                double x0, double y0, double x1, double y1) {
    double x_start, y_start, x_end, y_end, direction;

    // Set line width
    cairo_set_line_width(s->cairo_draw, lw);

    // Work out direction of arrow
    if (hypot(x1 - x0, y1 - y0) < 1e-200) direction = 0.0;
    else direction = atan2(x1 - x0, y1 - y0);

    // Draw arrowhead on beginning of arrow if desired
    if (head_start) {
        // Pointy back of arrowhead on one side
        const double x3 = x0 - CONST_ARROW_HEADSIZE * lw * sin((direction + M_PI) - CONST_ARROW_ANGLE / 2);
        const double y3 = y0 - CONST_ARROW_HEADSIZE * lw * cos((direction + M_PI) - CONST_ARROW_ANGLE / 2);

        // Pointy back of arrowhead on other side
        const double x5 = x0 - CONST_ARROW_HEADSIZE * lw * sin((direction + M_PI) + CONST_ARROW_ANGLE / 2);
        const double y5 = y0 - CONST_ARROW_HEADSIZE * lw * cos((direction + M_PI) + CONST_ARROW_ANGLE / 2);

        // Point where back of arrowhead crosses stalk
        const double x4 = x0 - CONST_ARROW_HEADSIZE * lw * sin(direction + M_PI) * (1.0 - CONST_ARROW_CONSTRICT) *
                               cos(CONST_ARROW_ANGLE / 2);
        const double y4 = y0 - CONST_ARROW_HEADSIZE * lw * cos(direction + M_PI) * (1.0 - CONST_ARROW_CONSTRICT) *
                               cos(CONST_ARROW_ANGLE / 2);

        // Draw arrow head
        cairo_new_path(s->cairo_draw);
        cairo_move_to(s->cairo_draw, x4, y4);
        cairo_line_to(s->cairo_draw, x3, y3);
        cairo_line_to(s->cairo_draw, x0, y0);
        cairo_line_to(s->cairo_draw, x5, y5);
        cairo_close_path(s->cairo_draw);
        cairo_fill(s->cairo_draw);

        // Start stalk of the arrow from the back of the arrow head
        x_start = x4;
        y_start = y4;
    } else {
        // Start stalk from the supplied coordinates (there is no arrow head)
        x_start = x0;
        y_start = y0;
    }

    // Draw arrowhead on end of arrow if desired
    if (head_end) {
        // Pointy back of arrowhead on one side
        const double x3 = x1 - CONST_ARROW_HEADSIZE * lw * sin(direction - CONST_ARROW_ANGLE / 2);
        const double y3 = y1 - CONST_ARROW_HEADSIZE * lw * cos(direction - CONST_ARROW_ANGLE / 2);

        // Pointy back of arrowhead on other side
        const double x5 = x1 - CONST_ARROW_HEADSIZE * lw * sin(direction + CONST_ARROW_ANGLE / 2);
        const double y5 = y1 - CONST_ARROW_HEADSIZE * lw * cos(direction + CONST_ARROW_ANGLE / 2);

        // Point where back of arrowhead crosses stalk
        const double x4 = x1 - CONST_ARROW_HEADSIZE * lw * sin(direction) * (1.0 - CONST_ARROW_CONSTRICT) *
                               cos(CONST_ARROW_ANGLE / 2);
        const double y4 = y1 - CONST_ARROW_HEADSIZE * lw * cos(direction) * (1.0 - CONST_ARROW_CONSTRICT) *
                               cos(CONST_ARROW_ANGLE / 2);

        // Draw arrow head
        cairo_new_path(s->cairo_draw);
        cairo_move_to(s->cairo_draw, x4, y4);
        cairo_line_to(s->cairo_draw, x3, y3);
        cairo_line_to(s->cairo_draw, x1, y1);
        cairo_line_to(s->cairo_draw, x5, y5);
        cairo_close_path(s->cairo_draw);
        cairo_fill(s->cairo_draw);

        // Start stalk of the arrow from the back of the arrow head
        x_end = x4;
        y_end = y4;
    } else {
        // Start stalk from the supplied coordinates (there is no arrow head)
        x_end = x1;
        y_end = y1;
    }

    // Draw stalk of arrow
    cairo_new_path(s->cairo_draw);
    cairo_move_to(s->cairo_draw, x_start, y_start);
    cairo_line_to(s->cairo_draw, x_end, y_end);
    cairo_stroke(s->cairo_draw);
}
