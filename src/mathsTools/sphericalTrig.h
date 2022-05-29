// SphericalTrig.h
// 
// -------------------------------------------------
// Copyright 2015-2022 Dominic Ford
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

#ifndef SPHERICALTRIG_H
#define SPHERICALTRIG_H 1

double angDist_RADec(double ra0, double dec0, double ra1, double dec1);

void rotate_xy(double *out, double *in, double theta);

void rotate_xz(double *out, double *in, double theta);

void make_zenithal(double *zenith_angle, double *azimuth, double ra, double dec, double ra0, double dec0);

void find_mean_position(double *ra_out, double *dec_out, const double *ra_list, const double *dec_list,
                        int point_count);

#endif

