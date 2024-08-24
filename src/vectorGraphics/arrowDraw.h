// arrowDraw.h
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

#ifndef ARROWDRAW_H
#define ARROWDRAW_H 1

#include "settings/chart_config.h"

#define CONST_ARROW_ANGLE       ( 45.0 *M_PI/180 )
#define CONST_ARROW_CONSTRICT   ( 0.2            )
#define CONST_ARROW_HEADSIZE    ( 6.0            )

void draw_arrow(chart_config *s, double lw, int head_start, int head_end,
                double x0, double y0, double x1, double y1);

#endif
