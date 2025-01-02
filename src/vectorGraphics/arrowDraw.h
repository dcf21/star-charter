// arrowDraw.h
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

#ifndef ARROWDRAW_H
#define ARROWDRAW_H 1

#include "settings/chart_config.h"

#define CONST_ARROW_ANGLE           ( 45.0 *M_PI/180 )
#define CONST_ARROW_ANGLE_THICK     ( 55.0 *M_PI/180 )
#define CONST_ARROW_CONSTRICT       ( 0.2            )
#define CONST_ARROW_HEADSIZE        ( 6.0            )
#define CONST_ARROW_HEADSIZE_THICK  ( 8.0            )

void arrow_path_retract(int *path_len, const double *x_list, const double *y_list, const double *theta_list,
                         double distance);

void draw_arrow(chart_config *s, double lw, int head_start, int head_end,
                double x0, double y0, double x1, double y1);

void draw_thick_arrow_segment(
        chart_config *s, double lw, int head_start, int head_end,
        const double *x_pixels, const double *y_pixels, const double *theta,
        int index_start, int index_end);

void draw_thick_arrow(chart_config *s, double lw, int head_start, int head_end,
                      const double *x_list, const double *y_list, const double *theta, int point_count);

#endif
