// solarSystem.h
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

#ifndef SOLARSYSTEM_H
#define SOLARSYSTEM_H 1

#include <stdlib.h>

#include "settings/chart_config.h"

void solar_system_write_ephemeris_definitions(const char (*solar_system_ids)[N_TRACES_MAX][FNAME_LENGTH],
                                              double julian_date, int object_count,
                                              char (*ephemeris_definitions_out)[N_TRACES_MAX][FNAME_LENGTH]);

void draw_solar_system_object(chart_config *s, cairo_page *page, colour object_colour, colour label_colour,
                              double mag, double x, double y, int is_comet, double sun_pa,
                              const char *label, const double *priority_in,
                              const int *possible_positions_in_count, const label_position *possible_positions_in);

void draw_moon(chart_config *s, cairo_page *page, colour label_colour,
               double x, double y, double ra, double dec, double ang_size,
               double julian_date, char *label);

void draw_sun(chart_config *s, cairo_page *page, colour label_colour,
              double x, double y, double ang_size, char *label);

void plot_solar_system(chart_config *s, cairo_page *page);

#endif
