// cairo_page.h
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

#ifndef CAIRO_PAGE_H
#define CAIRO_PAGE_H 1

#include <stdlib.h>
#include <stdio.h>

#include "listTools/ltList.h"

#include "settings/chart_config.h"

typedef struct {
    // The (x, y) position to label (tangent plane angular coordinates)
    double x, y;

    // The clockwise rotation angle of the text relative to upright (degrees)
    double rotation;

    // The offset of this label text relative to the point being labelled (cairo pixel coordinates)
    double offset_x, offset_y;

    // Value indicating horizontal alignment: -1 (left), 0 (centre) or 1 (right)
    int h_align;

    // Value indicating vertical alignment: -1 (bottom), 0 (middle) or 1 (top)
    int v_align;
} label_position;

typedef struct {
    label_position *possible_positions;
    int possible_position_count;
    chart_config *s;
    colour colour;
    const char *label;
    int multiple_labels;
    double font_size;
    int font_bold;
    int font_italic;
    int make_background;
    double extra_margin;
    double priority;
} label_buffer_item;

typedef struct {
    double x_min, x_max, y_min, y_max;
} exclusion_region;

typedef struct {
    list *x_labels, *x2_labels, *y_labels, *y2_labels;

    label_buffer_item *labels_buffer;
    int labels_buffer_counter;
    exclusion_region *exclusion_regions;
    int exclusion_region_counter;
} cairo_page;

char *string_make_permanent(const char *in);

void cairo_init(cairo_page *p, chart_config *s);

void plot_background_image(chart_config *s);

void move_to_next_page(chart_config *s);

void chart_edge_line_path(chart_config *s);

void draw_chart_edge_line(chart_config *s);

void draw_chart_edging(cairo_page *p, chart_config *s);

void fetch_canvas_coordinates(double *x_out, double *y_out, double x_in, double y_in, const chart_config *s);

void fetch_graph_coordinates(double x_in, double y_in, double *x_out, double *y_out, const chart_config *s);

void chart_label_buffer(cairo_page *p, chart_config *s, colour colour, const char *label,
                        const label_position *possible_positions, int possible_position_count, int multiple_labels,
                        int make_background, double font_size, int font_bold, int font_italic, double extra_margin,
                        double priority);

void chart_label_unbuffer(cairo_page *p);

void chart_add_label_exclusion(cairo_page *p, chart_config *s, double x_min, double x_max, double y_min, double y_max);

int chart_label(cairo_page *p, chart_config *s, colour colour, const char *label,
                const label_position *possible_positions, int possible_position_count, int multiple_labels,
                int make_background, double font_size, int font_bold, int font_italic,
                double extra_margin, double priority);

int chart_finish(cairo_page *p, chart_config *s);

#endif

