// solarSystem.h
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

#ifndef SOLARSYSTEM_H
#define SOLARSYSTEM_H 1

#include <stdlib.h>

#include "settings/chart_config.h"

void solar_system_write_ephemeris_definitions(const char (*solar_system_ids)[N_TRACES_MAX][FNAME_LENGTH],
                                              double julian_date, int object_count,
                                              char (*ephemeris_definitions_out)[N_TRACES_MAX][FNAME_LENGTH]);

void draw_solar_system_object(chart_config *s, cairo_page *page, colour object_colour, colour label_colour,
                              double mag, double x, double y, const char *label);

void draw_moon(chart_config *s, cairo_page *page, colour label_colour,
               double x, double y, double ra, double dec, double julian_date,
               const char *label);

void plot_solar_system(chart_config *s, cairo_page *page);

#endif
