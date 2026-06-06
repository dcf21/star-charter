// label_arranger.h
// 
// -------------------------------------------------
// Copyright 2015-2026 Dominic Ford
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

#ifndef LABEL_ARRANGER_H
#define LABEL_ARRANGER_H 1

#include "settings/chart_config.h"
#include "vectorGraphics/cairo_page.h"

char *string_make_permanent(const char *in);

void chart_label_buffer(cairo_page *p, chart_config *s, colour colour, const char *label,
                        const label_position *possible_positions, int possible_position_count, int multiple_labels,
                        int make_background, double font_size, int font_bold, int font_italic, double extra_margin,
                        double priority);

void chart_label_unbuffer(cairo_page *p);

void chart_add_label_exclusion(cairo_page *p, chart_config *s, double x_min, double x_max, double y_min, double y_max);

int chart_check_label_exclusion(const cairo_page *p, double x_min, double x_max, double y_min, double y_max);

int chart_label(cairo_page *p, chart_config *s, colour colour, const char *label,
                const label_position *possible_positions, int possible_position_count, int multiple_labels,
                int make_background, double font_size, int font_bold, int font_italic,
                double extra_margin, double priority);

#endif
