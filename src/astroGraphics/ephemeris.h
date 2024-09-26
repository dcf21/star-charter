// ephemeris.h
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

#ifndef EPHEMERIS_H
#define EPHEMERIS_H 1

#include <stdlib.h>
#include <stdio.h>

#include "settings/chart_config.h"
#include "vectorGraphics/lineDraw.h"
#include "vectorGraphics/cairo_page.h"

int ephemerides_fetch(ephemeris **ephemeris_data_out, int ephemeris_count,
                      const char (*ephemeris_definitions)[N_TRACES_MAX][FNAME_LENGTH],
                      double jd_step, int output_coordinates, double jd_central,
                      int do_topocentric_correction,
                      double topocentric_latitude, double topocentric_longitude);

void ephemerides_free(chart_config *s);

void ephemerides_autoscale_show_config(chart_config *s);

void ephemerides_autoscale_plot(chart_config *s, int total_ephemeris_points);

void ephemerides_add_manual_text_labels(chart_config *s);

void ephemerides_add_automatic_text_labels(chart_config *s);

void plot_ephemeris(chart_config *s, line_drawer *ld, cairo_page *page, int trace_num);

double draw_ephemeris_table(chart_config *s, double legend_y_pos, int draw_output, double *width_out);

#endif

