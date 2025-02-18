// stars.h
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

#ifndef STARS_H
#define STARS_H 1

#include <stdlib.h>
#include <stdio.h>

#include "astroGraphics/starListReader.h"
#include "settings/chart_config.h"
#include "vectorGraphics/lineDraw.h"
#include "vectorGraphics/cairo_page.h"

void tweak_magnitude_limits(chart_config *s);

void plot_stars_calculate_magnitude_range(chart_config *s, FILE *stars_data_file, const tiling_information *tiles,
                                          int *star_counter, double *star_mag_faintest, double *star_mag_brightest);

void plot_stars_path(chart_config *s, FILE *stars_data_file, const tiling_information *tiles,
                     double radius_multiplier, int clockwise);

int plot_stars_draw(chart_config *s, cairo_page *page, FILE *stars_data_file, const tiling_information *tiles);

double get_star_size(const chart_config *s, double mag);

void plot_stars(chart_config *s, cairo_page *page);

double draw_magnitude_key(chart_config *s, double legend_y_pos);

#endif

