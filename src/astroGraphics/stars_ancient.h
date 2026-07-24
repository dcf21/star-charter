// stars_ancient.h
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

#ifndef STARS_ANCIENT_H
#define STARS_ANCIENT_H 1

#include "vectorGraphics/cairo_page.h"

void draw_star_type1(cairo_t *cr, double x0, double y0, double ps, double azimuth);

void draw_star_type2(cairo_t *cr, double x0, double y0, double ps, double azimuth);

void draw_star_type3(cairo_t *cr, double x0, double y0, double ps, double azimuth);

void draw_star_type4(cairo_t *cr, double x0, double y0, double ps, double azimuth);

void draw_star_type5(cairo_t *cr, double x0, double y0, double ps, double azimuth);

void draw_star_type6(cairo_t *cr, double x0, double y0, double ps, double azimuth);

void draw_star_type7(cairo_t *cr, double x0, double y0, double ps, double azimuth);

void draw_star_type8(cairo_t *cr, double x0, double y0, double ps, double azimuth);

void draw_star_type9(cairo_t *cr, double x0, double y0, double ps, double azimuth);

void draw_ancient_star(cairo_t *cr, double x0, double y0, double mag, double ps);

#endif
