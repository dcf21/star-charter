// julianDate.c
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

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <gsl/gsl_math.h>

#include "mathsTools/deltaT.h"

#include "julianDate.h"
#include "precession.h"

// Routines for looking up the dates when the transition between the Julian calendar and the Gregorian calendar occurred

//! switch_over_calendar_date - Return the calendar date when the Julian calendar was replaced with the Gregorian
//! calendar
//! \param [out] last_julian - The last day of the Julian calendar, expressed as yyyymmdd
//! \param [out] first_gregorian - The first day of the Gregorian calendar, expressed as yyyymmdd

void switch_over_calendar_date(double *last_julian, double *first_gregorian) {
    *last_julian = 17520902.0;
    *first_gregorian = 17520914.0; // British
}

//! The Julian day number when the Julian calendar was replaced with the Gregorian
//! \return Julian day number

double switch_over_jd() {
    return 2361222.0; // British
}

//! get_month_name - Get the name of month number i, where 1 is January and 12 is December
//! \param [in] i - Month number
//! \return Month name

char *get_month_name(const int i) {
    switch (i) {
        case 1:
            return "January";
        case 2:
            return "February";
        case 3:
            return "March";
        case 4:
            return "April";
        case 5:
            return "May";
        case 6:
            return "June";
        case 7:
            return "July";
        case 8:
            return "August";
        case 9:
            return "September";
        case 10:
            return "October";
        case 11:
            return "November";
        case 12:
            return "December";
        default:
            return "???";
    }
}

//! get_week_day_name - Get the name of the ith day of the week, where 0 is Monday and 6 is Sunday
//! \param [in] i - Number of the day of the week
//! \return Name of the day

char *get_week_day_name(const int i) {
    switch (i) {
        case 0:
            return "Monday";
        case 1:
            return "Tuesday";
        case 2:
            return "Wednesday";
        case 3:
            return "Thursday";
        case 4:
            return "Friday";
        case 5:
            return "Saturday";
        case 6:
            return "Sunday";
        default:
            return "???";
    }
}

//! julian_day - Convert a calendar date into a Julian day number
//! \param [in] year The calendar year
//! \param [in] month The calendar month
//! \param [in] day The day of the month
//! \param [in] hour The hour of the day
//! \param [in] min The minutes within the hour
//! \param [in] sec The seconds within the minute
//! \param [out] status Zero on success; One on failure
//! \param [out] err_text Error message (if status is non-zero)
//! \return Julian day number

double julian_day(int year, int month, const int day, const int hour, const int min, const int sec,
                  int *status, char *err_text) {
    double last_julian, first_gregorian;
    int b;

    if ((year < -1e6) || (year > 1e6) || (!gsl_finite(year))) {
        *status = 1;
        sprintf(err_text, "Supplied year is too big.");
        return 0.0;
    }
    if ((day < 1) || (day > 31)) {
        *status = 1;
        sprintf(err_text, "Supplied day number should be in the range 1-31.");
        return 0.0;
    }
    if ((hour < 0) || (hour > 23)) {
        *status = 1;
        sprintf(err_text, "Supplied hour number should be in the range 0-23.");
        return 0.0;
    }
    if ((min < 0) || (min > 59)) {
        *status = 1;
        sprintf(err_text, "Supplied minute number should be in the range 0-59.");
        return 0.0;
    }
    if ((sec < 0) || (sec > 59)) {
        *status = 1;
        sprintf(err_text, "Supplied second number should be in the range 0-59.");
        return 0.0;
    }
    if ((month < 1) || (month > 12)) {
        *status = 1;
        sprintf(err_text, "Supplied month number should be in the range 1-12.");
        return 0.0;
    }

    switch_over_calendar_date(&last_julian, &first_gregorian);
    const double required_date = 10000.0 * year + 100 * month + day;

    if (month <= 2) {
        month += 12;
        year--;
    }

    // Julian calendar
    if (required_date <= last_julian) { b = -2 + ((year + 4716) / 4) - 1179; }

    // Gregorian calendar
    else if (required_date >= first_gregorian) { b = (year / 400) - (year / 100) + (year / 4); } else {
        *status = 1;
        sprintf(err_text,
                "The requested date never happened in the British calendar: "
                "it was lost in the transition from the Julian to the Gregorian calendar.");
        return 0.0;
    }

    const double jd = 365.0 * year - 679004.0 + 2400000.5 + b + floor(30.6001 * (month + 1)) + day;

    const double day_fraction = (fabs(hour) + fabs(min) / 60.0 + fabs(sec) / 3600.0) / 24.0;

    return jd + day_fraction;
}

//! inv_julian_day - Convert a julian day number into a calendar date
//! \param [in] jd Julian day number
//! \param [out] year The calendar year
//! \param [out] month The calendar month
//! \param [out] day The day of the month
//! \param [out] hour The hour of the day
//! \param [out] min The minutes within the hour
//! \param [out] sec The seconds within the minute
//! \param [out] status Zero on success; One on failure
//! \param [out] err_text Error message (if status is non-zero)

void inv_julian_day(const double jd, int *year, int *month, int *day, int *hour, int *min, double *sec, int *status,
                    char *err_text) {
    long b, c;
    int temp;
    if (month == NULL) month = &temp; // Dummy placeholder, since we need month later in the calculation

    if ((jd < -1e8) || (jd > 1e8) || (!gsl_finite(jd))) {
        *status = 1;
        sprintf(err_text, "Supplied Julian Day number is too big.");
        return;
    }

    // Work out hours, minutes and seconds
    const double day_fraction = (jd + 0.5) - floor(jd + 0.5);
    if (hour != NULL) *hour = (int) floor(24 * day_fraction);
    if (min != NULL) *min = (int) floor(fmod(1440 * day_fraction, 60));
    if (sec != NULL) *sec = fmod(86400 * day_fraction, 60);

    // Now work out calendar date
    // a = Number of whole Julian days.
    // b = Number of centuries since the Council of Nicaea.
    // c = Julian Day number as if century leap years happened.
    const long a = (long) (jd + 0.5);

    // Julian calendar
    if (a < (long) switch_over_jd()) {
        b = 0;
        c = a + 1524;
    }

    // Gregorian calendar
    else {
        b = (a - 1867216.25) / 36524.25;
        c = a + b - (b / 4) + 1525;
    }
    const long d = (c - 122.1) / 365.25; // Number of 365.25 periods, starting the year at the end of February
    const long e = 365 * d + d / 4; // Number of days accounted for by these
    const long f = (c - e) / 30.6001; // Number of 30.6001 days periods (a.k.a. months) in remainder
    if (day != NULL) *day = (int) floor(c - e - (int) (30.6001 * f));
    *month = (int) floor(f - 1 - 12 * (f >= 14));
    if (year != NULL) *year = (int) floor(d - 4715 - (*month >= 3));
}

//! unix_from_jd - Convert a Julian date into a unix time.
//! \param [in] jd - Input Julian date
//! \return - Float unix time

double unix_from_jd(const double jd) {
    return 86400.0 * (jd - 2440587.5);
}

//! jd_from_unix - Convert a unix time into a Julian date.
//! \param [in] utc - Input unix time
//! \return - Float Julian date

double jd_from_unix(const double utc) {
    return (utc / 86400.0) + 2440587.5;
}

//! sidereal_time - Return the Greenwich sidereal time, in hours, at UTC unix time <utc>. This is the RA at the zenith
//! in Greenwich.
//! \param [in] utc - Unix time (in UTC; not TT; by definition of sidereal_time)
//! \return - Sidereal time (hours)

double sidereal_time(const double utc) {
    const double u = utc;
    const double mjd_utc = 40587.5 + u / 86400.0; // Julian date - 2400000
    const double t = (mjd_utc - 51545.0) / 36525.0; // Julian century (no centuries since 2000.0)

    // See pages 87-88 of Astronomical Algorithms, by Jean Meeus
    const double st = fmod((
                               280.46061837 +
                               360.98564736629 * (mjd_utc - 51545.0) +
                               0.000387933 * t * t +
                               t * t * t / 38710000.0
                           ), 360) * 12 / 180;

    // Sidereal time, in hours. RA at zenith in Greenwich.
    return st;
}

//! sidereal_time_jd - Turns a Julian date into a sidereal time (in hours, at Greenwich)
//! @param [in] dt_calc - Initialised DeltaTCalculator struct instance.
//! @param [in] jd_tt - Julian date (TT)
//! @return - Sidereal time in radians

double sidereal_time_jd(const DeltaTCalculator *dt_calc, const double jd_tt) {
    const double jd_utc = utc_from_tt(dt_calc, jd_tt, 0, 0);
    const double unix_time = unix_from_jd(jd_utc);
    const double st_hours = sidereal_time(unix_time);
    const double st_radians = M_PI / 12. * st_hours;
    return st_radians;
}

//! inclination_ecliptic - Calculate the inclination of the ecliptic at epoch JD.
//!    Method based on Laskar, J., A&A, 157, 68 (1986)
//!    See Astronomical Algorithms, Jean Meeus, chapter 22, page 147.
//! @param [in] jd - The epoch at which to calculate the inclination of the ecliptic; Julian day number.
//! @return - Inclination of the ecliptic; degrees.
double inclination_ecliptic(const double jd) {
    // Convert JD to U = T / 100
    const double t = (jd - 2451545.0) / 36525.0;
    const double u = t / 100.0;

    // Constants
    const double degree = M_PI / 180.0;
    const double arcsecond = degree / 3600.0;

    // Polynomial coefficients (radians)
    static const double eta_coefficients[] = {
        (23.0 * 3600.0 + 26.0 * 60.0 + 21.448) * arcsecond,
        -4680.93 * arcsecond,
        -1.55 * arcsecond,
        1999.25 * arcsecond,
        -51.38 * arcsecond,
        -249.67 * arcsecond,
        -39.05 * arcsecond,
        7.12 * arcsecond,
        27.87 * arcsecond,
        5.79 * arcsecond,
        2.45 * arcsecond
    };

    // Inclination of the ecliptic (degrees)
    const double eta = polynomial_horner(u, eta_coefficients,
                                         sizeof(eta_coefficients) / sizeof(eta_coefficients[0])
                       ) / degree;

    return eta;
}

//! get_zenith_position - Calculate the right ascension and declination of the zenith
//! \param [in] latitude - The latitude of the observer, degrees
//! \param [in] longitude - The longitude of the observer, degrees
//! \param [in] julian_date - The Julian date of the observation (UTC)
//! \param [out] ra_at_epoch_out - Output right ascension, in radians, at epoch
//! \param [out] dec_at_epoch_out - Output declination, in radians, at epoch
void get_zenith_position(double latitude, double longitude, double julian_date,
                         double *ra_at_epoch_out, double *dec_at_epoch_out) {
    const double utc = unix_from_jd(julian_date);
    const double st = sidereal_time(utc) * M_PI / 12;

    // Convert geographic coordinates to radians
    latitude *= M_PI / 180;
    longitude *= M_PI / 180;

    // Calculate RA and Dec
    double dec = latitude;
    double ra = longitude + st;

    // Ensure RA and Dec are within allowed range
    while (dec < -M_PI) dec += 2 * M_PI;
    while (dec > M_PI) dec -= 2 * M_PI;
    while (ra < 0) ra += 2 * M_PI;
    while (ra > 2 * M_PI) ra -= 2 * M_PI;

    // Return output
    *ra_at_epoch_out = ra;
    *dec_at_epoch_out = dec;
}

//! sun_pos_approx - Calculate an estimate of the J2000.0 RA and Decl of the Sun at a particular Unix time.
//! See Jean Meeus, Astronomical Algorithms, pp 163-4
//! \param [in] julian_date - Julian date of query
//! \param [out] ra_j2000_out - Right ascension, J2000.0, hours
//! \param [out] dec_j2000_out - Declination, J2000.0, degrees
void sun_pos_approx(const double julian_date, double *ra_j2000_out, double *dec_j2000_out) {
    const double deg = M_PI / 180;

    const double jd = julian_date;

    const double t = (jd - 2451545.0) / 36525.;
    const double l0 = 280.46646 + 36000.76983 * t + 0.0003032 * t * t;
    const double m = 357.52911 + 35999.05029 * t + 0.0001537 * t * t;
    // const double e = 0.016708634 - 0.000042037 * t - 0.0000001267 * t * t;

    const double c = ((1.914602 - 0.004817 * t - 0.000014 * t * t) * sin(m * deg) +
                      (0.019993 - 0.000101 * t) * sin(2 * m * deg) +
                      0.000289 * sin(3 * m * deg));

    const double tl = l0 + c; // true longitude
    // const double v = m + c;  // true anomaly

    const double epsilon = inclination_ecliptic(jd);

    // Calculate RA and Dec at the epoch of the observation
    const double ra_epoch = atan2(cos(epsilon * deg) * sin(tl * deg), cos(tl * deg)); // radians
    const double dec_epoch = asin(sin(epsilon * deg) * sin(tl * deg)); // radians

    // Convert to J2000.0 coordinates
    double ra_j2000, dec_j2000; // radians
    ra_dec_to_j2000(ra_epoch, dec_epoch, jd, &ra_j2000, &dec_j2000);

    // Ensure right ascension is in the range 0-24 hours
    double ra_j2000_hours = ra_j2000 * 12 / M_PI;
    double dec_j2000_deg = dec_j2000 * 180 / M_PI;
    while (ra_j2000_hours < 0) {
        ra_j2000_hours += 24;
    }

    // Return output
    *ra_j2000_out = ra_j2000_hours;
    *dec_j2000_out = dec_j2000_deg;
}
