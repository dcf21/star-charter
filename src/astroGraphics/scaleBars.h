// scaleBars.h
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

#ifndef SCALE_BARS_H
#define SCALE_BARS_H 1

#include <stdlib.h>

#include "settings/chart_config.h"
#include "vectorGraphics/cairo_page.h"

void convert_normalised_coordinates_to_tangent_plane(const chart_config *s,
                                                     double x_norm, double y_norm,
                                                     double *x_tangent_out, double *y_tangent_out,
                                                     double *x_cairo_out, double *y_cairo_out);

double cairo_units_per_degree(const chart_config *s);

void plot_scale_bars(chart_config *s, cairo_page *page);

#endif
