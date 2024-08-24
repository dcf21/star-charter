// julianDate.h
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

#ifndef JULIANDATE_H
#define JULIANDATE_H 1

void switch_over_calendar_date(double *lastJulian, double *firstGregorian);

double switch_over_jd();

char *get_month_name(int i);

char *get_week_day_name(int i);

double julian_day(int year, int month, int day, int hour, int min, int sec, int *status, char *err_text);

void inv_julian_day(double jd, int *year, int *month, int *day, int *hour, int *min, double *sec, int *status,
                    char *err_text);

double sidereal_time(double utc);

double unix_from_jd(double jd);

double jd_from_unix(double utc);

void ra_dec_from_j2000(double ra_j2000_in, double dec_j2000_in, double jd_new,
                       double *ra_epoch_out, double *dec_epoch_out);

void ra_dec_to_j2000(double ra_epoch_in, double dec_epoch_in, double jd_old,
                     double *ra_j2000_out, double *dec_j2000_out);

void ra_dec_switch_epoch(double ra_epoch_in, double dec_epoch_in, double jd_epoch_in,
                         double jd_epoch_out, double *ra_epoch_out, double *dec_epoch_out);

void ra_dec_j2000_from_b1950(double ra_b1950_in, double dec_b1950_in, double *ra_j2000_out, double *dec_j2000_out);

void ra_dec_b1950_from_j2000(double ra_j2000_in, double dec_j2000_in, double *ra_b1950_out, double *dec_b1950_out);

void get_zenith_position(double latitude, double longitude, double julian_date,
                         double *ra_at_epoch_out, double *dec_at_epoch_out);

void sun_pos(double julian_date, double *ra_j2000_out, double *dec_j2000_out);

#endif
