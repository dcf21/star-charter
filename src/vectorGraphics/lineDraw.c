// lineDraw.c
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
#include <math.h>
#include <string.h>

#include <gsl/gsl_math.h>

#include "listTools/ltList.h"
#include "listTools/ltMemory.h"

#include "settings/chart_config.h"

#include "vectorGraphics/lineDraw.h"
#include "vectorGraphics/cairo_page.h"

//! list_add_tick - Add a label to one of the axes of a star chart, indicating a particular RA or Dec
//! \param l - The list of labels to go along a particular axis of a chart
//! \param x - The position along the axis where this label should be placed
//! \param label - The new label to place on this axis

static void list_add_tick(list *l, double x, const char *label) {
    void *buff;
    if (l == NULL) return;
    if (label == NULL) return;
    buff = lt_malloc(FNAME_LENGTH);
    memcpy(buff, &x, sizeof(double));
    strcpy(buff + sizeof(double), label);
    listAppendPtr(l, buff, FNAME_LENGTH, 0, DATATYPE_VOID);
}

//! truncate_at_axis - Take a line segment which passes outside the boundary of the star chart, and truncate it at the
//! edge of the chart. Also, if this line has a text label associated with it, then place a label on the axis where
//! this line meets it, typically denoting the RA or the Dec being traced by this line.
//! \param [out] xout - The x coordinate (plot coordinates) of the intersection between this line and the edge of the
//! chart.
//! \param [out] yout - The y coordinate (plot coordinates) of the intersection between this line and the edge of the
//! chart.
//! \param [in] x0 - The x coordinate (plot coordinates) of the start of the line.
//! \param [in] y0 - The y coordinate (plot coordinates) of the start of the line.
//! \param [in] x1 - The x coordinate (plot coordinates) of the end of the line.
//! \param [in] y1 - The y coordinate (plot coordinates) of the end of the line.
//! \param [in] xmin - The left-most limit of the horizontal axis of the star chart (plot coordinates)
//! \param [in] xmax - The right-most limit of the horizontal axis of the star chart (plot coordinates)
//! \param [in] ymin - The bottom-most limit of the vertical axis of the star chart (plot coordinates)
//! \param [in] ymax - The top-most limit of the vertical axis of the star chart (plot coordinates)
//! \param [in] label - The string label associated with this line
//! \param xlabels - The list of labels to place on the bottom edge of the chart
//! \param x2labels - The list of labels to place on the top edge of the chart
//! \param ylabels - The list of labels to place on the left edge of the chart
//! \param y2labels - The list of labels to place on the right edge of the chart

void truncate_at_axis(double *xout, double *yout, double x0, double y0, double x1, double y1, double xmin, double xmax,
                      double ymin, double ymax, char *label, list *xlabels, list *x2labels, list *ylabels,
                      list *y2labels) {
    double x, y;

    if (x1 != x0) {
        // Look for intersection with left edge of chart
        y = (xmin - x0) * (y1 - y0) / (x1 - x0) + y0;
        if (
                (y <= gsl_max(y0, y1)) && (y >= gsl_min(y0, y1)) &&
                (xmin <= gsl_max(x1, x0)) && (xmin >= gsl_min(x1, x0))) {
            list_add_tick(ylabels, y, label);
            *xout = xmin;
            *yout = y;
            return;
        }
        // Look for intersection with right edge of chart
        y = (xmax - x0) * (y1 - y0) / (x1 - x0) + y0;
        if (
                (y <= gsl_max(y0, y1)) && (y >= gsl_min(y0, y1)) &&
                (xmax <= gsl_max(x1, x0)) && (xmax >= gsl_min(x1, x0))) {
            list_add_tick(y2labels, y, label);
            *xout = xmax;
            *yout = y;
            return;
        }
    }

    if (y1 != y0) {
        // Look for intersection with top edge of chart
        x = (ymin - y0) * (x1 - x0) / (y1 - y0) + x0;
        if (
                (x <= gsl_max(x0, x1)) && (x >= gsl_min(x0, x1)) &&
                (ymin <= gsl_max(y1, y0)) && (ymin >= gsl_min(y1, y0))) {
            list_add_tick(xlabels, x, label);
            *xout = x;
            *yout = ymin;
            return;
        }
        // Look for intersection with bottom edge of chart
        x = (ymax - y0) * (x1 - x0) / (y1 - y0) + x0;
        if (
                (x <= gsl_max(x0, x1)) && (x >= gsl_min(x0, x1)) &&
                (ymax <= gsl_max(y1, y0)) && (ymax >= gsl_min(y1, y0))) {
            list_add_tick(x2labels, x, label);
            *xout = x;
            *yout = ymax;
            return;
        }
    }

    // If no intersection found, then simply return the position of the start of the line
    *xout = x0;
    *yout = y0;
}

//! ld_init - Initialise this module for drawing lines on star charts
//! \param self - The <line_drawer> structure used to hold data about this line drawing module instance.
//! \param s - A <chart_config> structure defining the properties of the star chart to be drawn.
//! \param xlabels - The list of labels to place on the bottom edge of the chart
//! \param x2labels - The list of labels to place on the top edge of the chart
//! \param ylabels - The list of labels to place on the left edge of the chart
//! \param y2labels - The list of labels to place on the right edge of the chart

void ld_init(line_drawer *self, chart_config *s, list *xlabels, list *x2labels, list *ylabels, list *y2labels) {
    self->label = NULL;
    self->label_on_x = 1;
    self->label_on_y = 1;
    self->penup = 1;
    self->haddata = 0;
    self->xmin = s->x_min;
    self->xmax = s->x_max;
    self->ymin = s->y_min;
    self->ymax = s->y_max;
    self->xlabels = xlabels;
    self->x2labels = x2labels;
    self->ylabels = ylabels;
    self->y2labels = y2labels;
    self->s = s;
    self->wlin = s->wlin;
    self->xold = GSL_NAN;
    self->yold = GSL_NAN;
    ld_pen_up(self, GSL_NAN, GSL_NAN, NULL, 1);
    cairo_set_line_width(s->cairo_draw, 1.0 * s->line_width_base);
}

//! ld_label - Set a text label to be associated with the next line we draw on the star chart
//! \param self - The <line_drawer> structure used to hold data about this line drawing module instance.
//! \param l - The text label to be associated with the next line we draw
//! \param label_on_x - Boolean indicating whether this label is allowed to be written on horizontal axes
//! \param label_on_y - Boolean indicating whether this label is allowed to be written on vertical axes

void ld_label(line_drawer *self, char *l, int label_on_x, int label_on_y) {
    self->label = l;
    self->label_on_x = label_on_x;
    self->label_on_y = label_on_y;
}

//! ld_pen_up - Lift the pen and start drawing a new line
//! \param self - The <line_drawer> structure used to hold data about this line drawing module instance.
//! \param x - The x coordinate of the last point of the existing line (or GSL_NAN if no point)
//! \param y - The y coordinate of the last point of the existing line (or GSL_NAN if no point)
//! \param name - The name of the existing line we've just finished
//! \param new_line - Boolean flag indicating whether this is a line with a new name, or another segment of same line

void ld_pen_up(line_drawer *self, double x, double y, const char *name, int new_line) {
    if ((gsl_finite(x)) && (!self->penup)) ld_point(self, x, y, name);
    if (!self->penup) {
        cairo_stroke(self->s->cairo_draw);
    }
    self->penup = 1;
    if (new_line) { self->xold = self->yold = GSL_NAN; }
}

//! ld_close - Shut down this line drawing module instance. Finish any lines we've not finished drawing to canvas yet.
//! \param self - The <line_drawer> structure used to hold data about this line drawing module instance.

void ld_close(line_drawer *self) {
    ld_pen_up(self, GSL_NAN, GSL_NAN, "", 1);
}

//! ld_point - Add a point to a line we're drawing
//! \param self - The <line_drawer> structure used to hold data about this line drawing module instance.
//! \param x - The x coordinate (plot coordinates) of the point to move to
//! \param y - The y coordinate (plot coordinates) of the point to move to
//! \param name - The text label associated with the line we're drawing

void ld_point(line_drawer *self, double x, double y, const char *name) {
    int i, Nitems = 0;
    double xp[4], yp[4];
    int both_onscr = ((x < self->xmax) && (x > self->xmin) && (y < self->ymax) && (y > self->ymin) &&
                      (self->xold < self->xmax) && (self->xold > self->xmin) && (self->yold < self->ymax) &&
                      (self->yold > self->ymin));
    if (((self->s->projection == SW_PROJECTION_FLAT) || (self->s->projection == SW_PROJECTION_PETERS)) &&
        (gsl_finite(self->xold)) && both_onscr && ((x - self->xold) > self->wlin / 2)) {
        xp[0] = x - self->wlin;
        yp[0] = y;
        xp[1] = yp[1] = GSL_NAN;
        xp[2] = self->xold + self->wlin;
        yp[2] = y;
        xp[3] = x;
        yp[3] = y;
        Nitems = 4;
    } else if (((self->s->projection == SW_PROJECTION_FLAT) || (self->s->projection == SW_PROJECTION_PETERS)) &&
               (gsl_finite(self->xold)) && both_onscr && ((x - self->xold) < -self->wlin / 2)) {
        xp[0] = x + self->wlin;
        yp[0] = y;
        xp[1] = yp[1] = GSL_NAN;
        xp[2] = self->xold - self->wlin;
        yp[2] = y;
        xp[3] = x;
        yp[3] = y;
        Nitems = 4;
    } else {
        xp[0] = x;
        yp[0] = y;
        Nitems = 1;
    }
    for (i = 0; i < Nitems; i++) {
        if (!gsl_finite(xp[i])) ld_pen_up(self, GSL_NAN, GSL_NAN, NULL, 1);
        else ld_point_plot(self, xp[i], yp[i], name);
    }
}

//! ld_point_plot - Internal function to add a point to a line
//! \param self - The <line_drawer> structure used to hold data about this line drawing module instance.
//! \param x - The x coordinate (plot coordinates) of the point to move to
//! \param y - The y coordinate (plot coordinates) of the point to move to
//! \param name - The text label associated with the line we're drawing

void ld_point_plot(line_drawer *self, double x, double y, const char *name) {
    double x_canvas, y_canvas;
    if ((x < self->xmax) && (x > self->xmin) && (y < self->ymax) && (y > self->ymin)) {
        if ((gsl_finite(self->xold)) && self->penup) {
            double xo, yo;
            truncate_at_axis(&xo, &yo, self->xold, self->yold, x, y, self->xmin, self->xmax, self->ymax, self->ymin,
                             self->label,
                             self->label_on_x ? self->xlabels : NULL,
                             self->label_on_x ? self->x2labels : NULL,
                             self->label_on_y ? self->ylabels : NULL,
                             self->label_on_y ? self->y2labels : NULL);

            fetch_canvas_coordinates(&x_canvas, &y_canvas, xo, yo, self->s);
            if (!self->haddata) {
                cairo_new_path(self->s->cairo_draw);
                cairo_move_to(self->s->cairo_draw, x_canvas, y_canvas);
            } else cairo_line_to(self->s->cairo_draw, x_canvas, y_canvas);
            self->haddata = 1;
        }
        fetch_canvas_coordinates(&x_canvas, &y_canvas, x, y, self->s);
        if (!self->haddata) {
            cairo_new_path(self->s->cairo_draw);
            cairo_move_to(self->s->cairo_draw, x_canvas, y_canvas);
        } else cairo_line_to(self->s->cairo_draw, x_canvas, y_canvas);
        self->haddata = 1;
        self->penup = 0;
    } else {
        if ((gsl_finite(self->xold)) && (!self->penup)) {
            double xo, yo;
            truncate_at_axis(&xo, &yo, self->xold, self->yold, x, y, self->xmin, self->xmax, self->ymax, self->ymin,
                             self->label,
                             self->label_on_x ? self->xlabels : NULL,
                             self->label_on_x ? self->x2labels : NULL,
                             self->label_on_y ? self->ylabels : NULL,
                             self->label_on_y ? self->y2labels : NULL);

            fetch_canvas_coordinates(&x_canvas, &y_canvas, xo, yo, self->s);
            if (!self->haddata) {
                cairo_new_path(self->s->cairo_draw);
                cairo_move_to(self->s->cairo_draw, x_canvas, y_canvas);
            } else cairo_line_to(self->s->cairo_draw, x_canvas, y_canvas);
            self->haddata = 1;
        }
        ld_pen_up(self, GSL_NAN, GSL_NAN, NULL, 0);
    }
    self->xold = x;
    self->yold = y;
}
