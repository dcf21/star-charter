// lineDraw.h
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

#ifndef LINEDRAW_H
#define LINEDRAW_H 1

#include <stdlib.h>
#include <stdio.h>

#include "listTools/ltList.h"

#include "settings/chart_config.h"

typedef struct ld_handle {
    char *label;
    int label_on_x, label_on_y;
    list *xlabels, *x2labels, *ylabels, *y2labels;
    int penup, haddata;
    double xmin, xmax, ymin, ymax, wlin;
    chart_config *s;
    double xold, yold;
} line_drawer;

void truncate_at_axis(double *xout, double *yout, double x0, double y0, double x1, double y1, double xmin, double xmax,
                      double ymin, double ymax, char *label, list *xlabels, list *x2labels, list *ylabels,
                      list *y2labels);

void ld_init(line_drawer *self, chart_config *s, list *xlabels, list *x2labels, list *ylabels, list *y2labels);

void ld_label(line_drawer *self, char *l, int label_on_x, int label_on_y);

void ld_pen_up(line_drawer *self, double x, double y, const char *name, int new_line);

void ld_close(line_drawer *self);

void ld_point(line_drawer *self, double x, double y, const char *name);

void ld_point_plot(line_drawer *self, double x, double y, const char *name);

#endif

