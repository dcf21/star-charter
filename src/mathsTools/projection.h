// projection.h
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

#ifndef PROJECTION_H
#define PROJECTION_H 1

#include "settings/chart_config.h"

void galactic_project(double ra, double dec, double *l_out, double *b_out);

void inv_galactic_project(double *ra_out, double *dec_out, double l, double b);

void alt_az(double ra, double dec, double julian_date, double latitude, double longitude, double *alt, double *az);

void inv_alt_az(double alt, double az, double julian_date, double latitude, double longitude,
                double *ra_out, double *dec_out);

void convert_ra_dec_to_selected_coordinates(const chart_config *s, int coords, double ra, double dec,
                                            double *lat_out, double *lng_out);

void convert_selected_coordinates_to_ra_dec(const chart_config *s, int coords, double lng_in, double lat_in,
                                            double *ra_out, double *dec_out);

void plane_project_flat(double *x, double *y, const chart_config *s, double ra, double dec);

void plane_project_peters(double *x, double *y, const chart_config *s, double ra, double dec);

void plane_project_multilatitude(double *x, double *y, const chart_config *s, double ra, double dec);

void plane_project_spherical(double *x, double *y, const chart_config *s, double ra, double dec);

void plane_project(double *x, double *y, const chart_config *s, double ra, double dec, int allow_below_horizon);

void inv_plane_project_flat(double *ra, double *dec, const chart_config *s, double x, double y);

void inv_plane_project_peters(double *ra, double *dec, const chart_config *s, double x, double y);

void inv_plane_project_multilatitude(double *ra, double *dec, const chart_config *s, double x, double y);

void inv_plane_project_spherical(double *ra, double *dec, const chart_config *s, double x, double y);

void inv_plane_project(double *ra, double *dec, const chart_config *s, double x, double y);

#endif
