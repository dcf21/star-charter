// deltaT.h
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

#ifndef DELTA_T_H
#define DELTA_T_H

#define SECONDS_PER_DAY 86400.0

typedef struct {
    int s_iers;

    double jd_iers_start;
    double jd_iers_end;
    double c_iers;

    size_t n_iers;

    double *delta_t_iers;
    double *sigma;
} DeltaTCalculator;

int delta_t_init(DeltaTCalculator *calc);

void delta_t_free(DeltaTCalculator *calc);

double delta_t_parametric(double year, double d_alpha);

double delta_t_error_parametric(double year);

double delta_t_interpolate_iers(const DeltaTCalculator *calc, double jd);

double delta_t_interpolate_iers_error(const DeltaTCalculator *calc, double jd);

double delta_t(const DeltaTCalculator *calc, double jd);

double delta_t_error(const DeltaTCalculator *calc, double jd);

double tt_from_utc(const DeltaTCalculator *calc, double jd_utc,
                   int has_assumed_delta_t, double assumed_delta_t);

double utc_from_tt(const DeltaTCalculator *calc, double jd_tt,
                   int has_assumed_delta_t, double assumed_delta_t);

#endif
