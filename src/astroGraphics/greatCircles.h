// greatCircles.h
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

#ifndef GREATCIRCLES_H
#define GREATCIRCLES_H 1

#include <stdlib.h>
#include <stdio.h>

#include "settings/chart_config.h"
#include "vectorGraphics/lineDraw.h"
#include "vectorGraphics/cairo_page.h"

//! A label to place along the length of a great circle
typedef struct {
    char label[16];
    double xpos;
} gc_label;

void plot_great_circle(double ra0, double dec0, chart_config *s, line_drawer *ld,
                       cairo_page *page, int n_labels, gc_label *labels, colour colour);

void plot_equator(chart_config *s, line_drawer *ld, cairo_page *page);

void plot_galactic_plane(chart_config *s, line_drawer *ld, cairo_page *page);

void plot_ecliptic(chart_config *s, line_drawer *ld, cairo_page *page);

double draw_great_circle_key(chart_config *s, double legend_y_pos);

#endif

