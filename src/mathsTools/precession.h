// precession.h
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

#ifndef PRECESSION_H
#define PRECESSION_H 1

double polynomial_horner(double x, const double *c, int length);

void ra_dec_switch_epoch(double jd_in, double jd_out, double ra_epoch_in, double dec_epoch_in,
                         double *ra_epoch_out, double *dec_epoch_out);

void ra_dec_from_j2000(double ra_j2000_in, double dec_j2000_in, double jd_new,
                       double *ra_epoch_out, double *dec_epoch_out);

void ra_dec_to_j2000(double ra_epoch_in, double dec_epoch_in, double jd_old,
                     double *ra_j2000_out, double *dec_j2000_out);

void ra_dec_j2000_from_b1950(double ra_b1950_in, double dec_b1950_in, double *ra_j2000_out, double *dec_j2000_out);

void ra_dec_b1950_from_j2000(double ra_j2000_in, double dec_j2000_in, double *ra_b1950_out, double *dec_b1950_out);

void ecl_switch_epoch(double jd_in, double jd_out, double ecl_lng_in, double ecl_lat_in,
                      double *ecl_lng_out, double *ecl_lat_out);

#endif
