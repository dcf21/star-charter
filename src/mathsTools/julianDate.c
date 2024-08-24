// julianDate.c
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

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <gsl/gsl_math.h>

#include "mathsTools/julianDate.h"

// Routines for looking up the dates when the transition between the Julian calendar and the Gregorian calendar occurred

//! switch_over_calendar_date - Return the calendar date when the Julian calendar was replaced with the Gregorian
//! calendar
//! \param [out] lastJulian - The last day of the Julian calendar, expressed as yyyymmdd
//! \param [out] firstGregorian - The first day of the Gregorian calendar, expressed as yyyymmdd

void switch_over_calendar_date(double *lastJulian, double *firstGregorian) {
    *lastJulian = 17520902.0;
    *firstGregorian = 17520914.0; // British
}

//! The Julian day number when the Julian calendar was replaced with the Gregorian
//! \return Julian day number

double switch_over_jd() {
    return 2361222.0; // British
}

//! get_month_name - Get the name of month number i, where 1 is January and 12 is December
//! \param [in] i - Month number
//! \return Month name

char *get_month_name(int i) {
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

char *get_week_day_name(int i) {
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

double julian_day(int year, int month, int day, int hour, int min, int sec, int *status, char *err_text) {
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

    if (required_date <= last_julian) {
        // Julian calendar
        b = -2 + ((year + 4716) / 4) - 1179;
    } else if (required_date >= first_gregorian) {
        // Gregorian calendar
        b = (year / 400) - (year / 100) + (year / 4);
    } else {
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

void inv_julian_day(double jd, int *year, int *month, int *day, int *hour, int *min, double *sec, int *status,
                    char *err_text) {
    long b, c;
    int temp;

    // Dummy placeholder, since we need month later in the calculation
    if (month == NULL) month = &temp;

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
    const long a = jd + 0.5;

    if (a < switch_over_jd()) {
        // Julian calendar
        b = 0;
        c = a + 1524;
    } else {
        // Gregorian calendar
        b = (a - 1867216.25) / 36524.25;
        c = a + b - (b / 4) + 1525;
    }

    // Number of 365.25 periods, starting the year at the end of February
    const long d = (c - 122.1) / 365.25;

    // Number of days accounted for by these
    const long e = 365 * d + d / 4;

    // Number of 30.6001 days periods (a.k.a. months) in remainder
    const long f = (c - e) / 30.6001;

    if (day != NULL) *day = (int) floor(c - e - (int) (30.6001 * f));
    *month = (int) floor(f - 1 - 12 * (f >= 14));
    if (year != NULL) *year = (int) floor(d - 4715 - (*month >= 3));
}

//! sidereal_time - Return the Greenwich sidereal time, in hours, at unix time <utc>. This is the RA at the zenith
//! in Greenwich.
//! \param [in] utc - Unix time
//! \return - Sidereal time (hours)

double sidereal_time(double utc) {
    const double u = utc;
    const double j = 40587.5 + u / 86400.0;  // Julian date - 2400000
    const double t = (j - 51545.0) / 36525.0; // Julian century (no centuries since 2000.0)

    // See pages 87-88 of Astronomical Algorithms, by Jean Meeus
    const double st = fmod((
                                   280.46061837 +
                                   360.98564736629 * (j - 51545.0) +
                                   0.000387933 * t * t +
                                   t * t * t / 38710000.0
                           ), 360) * 12 / 180;

    // Sidereal time, in hours. RA at zenith in Greenwich.
    return st;
}

//! unix_from_jd - Convert a Julian date into a unix time.
//! \param [in] jd - Input Julian date
//! \return - Float unix time

double unix_from_jd(double jd) {
    return 86400.0 * (jd - 2440587.5);
}

//! jd_from_unix - Convert a unix time into a Julian date.
//! \param [in] utc - Input unix time
//! \return - Float Julian date

double jd_from_unix(double utc) {
    return (utc / 86400.0) + 2440587.5;
}

//! ra_dec_from_j2000 - Convert celestial coordinates from J2000 into a new epoch. See Green's Spherical Astronomy, pp 222-225
//! \param [in] ra_j2000_in - Input right ascension, in radians, J2000
//! \param [in] dec_j2000_in - Input declination, in radians, J2000
//! \param [in] jd_new - Julian date of the epoch we are to transform celestial coordinates into
//! \param [out] ra_epoch_out - Output right ascension, in radians, reference frame at epoch
//! \param [out] dec_epoch_out - Output declination, in radians, reference frame at epoch

void ra_dec_from_j2000(double ra_j2000_in, double dec_j2000_in, double jd_new,
                       double *ra_epoch_out, double *dec_epoch_out) {
    const double j = jd_new - 2400000; // Julian date - 2400000
    const double t = (j - 51545.0) / 36525.0; // Julian century (no centuries since 2000.0)

    const double deg = M_PI / 180;
    const double m = (1.281232 * t + 0.000388 * t * t) * deg;
    const double n = (0.556753 * t + 0.000119 * t * t) * deg;

    const double ra_m = ra_j2000_in + 0.5 * (m + n * sin(ra_j2000_in) * tan(dec_j2000_in));
    const double dec_m = dec_j2000_in + 0.5 * n * cos(ra_m);

    *ra_epoch_out = ra_j2000_in + m + n * sin(ra_m) * tan(dec_m);
    *dec_epoch_out = dec_j2000_in + n * cos(ra_m);
}

//! ra_dec_to_j2000 - Convert celestial coordinates to J2000 from another epoch. See Green's Spherical Astronomy, pp 222-225
//! \param [in] ra_epoch_in - Input right ascension, in radians, reference frame at epoch
//! \param [in] dec_epoch_in - Input declination, in radians, reference frame at epoch
//! \param [in] jd_old - Julian date of the epoch we are to transform celestial coordinates from
//! \param [out] ra_j2000_out - Output right ascension, in radians, J2000
//! \param [out] dec_j2000_out - Output declination, in radians, J2000

void ra_dec_to_j2000(double ra_epoch_in, double dec_epoch_in, double jd_old,
                     double *ra_j2000_out, double *dec_j2000_out) {
    const double j = jd_old - 2400000; // Julian date - 2400000
    const double t = (j - 51545.0) / 36525.0; // Julian century (no centuries since 2000.0)

    const double deg = M_PI / 180;
    const double m = (1.281232 * t + 0.000388 * t * t) * deg;
    const double n = (0.556753 * t + 0.000119 * t * t) * deg;

    const double ra_m = ra_epoch_in - 0.5 * (m + n * sin(ra_epoch_in) * tan(dec_epoch_in));
    const double dec_m = dec_epoch_in - 0.5 * n * cos(ra_m);

    *ra_j2000_out = ra_epoch_in - m - n * sin(ra_m) * tan(dec_m);
    *dec_j2000_out = dec_epoch_in - n * cos(ra_m);
}

//! ra_dec_switch_epoch - Convert celestial coordinates from one epoch into a new epoch. See Green's Spherical Astronomy, pp 222-225
//! \param [in] ra_epoch_in - Input right ascension, in radians, reference frame at epoch
//! \param [in] dec_epoch_in - Input declination, in radians, reference frame at epoch
//! \param [in] jd_epoch_in - Julian date of the epoch we are to transform celestial coordinates from
//! \param [in] jd_epoch_out - Julian date of the epoch we are to transform celestial coordinates into
//! \param [out] ra_epoch_out - Output right ascension, in radians, reference frame at epoch
//! \param [out] dec_epoch_out - Output declination, in radians, reference frame at epoch
void ra_dec_switch_epoch(double ra_epoch_in, double dec_epoch_in, double jd_epoch_in,
                         double jd_epoch_out, double *ra_epoch_out, double *dec_epoch_out) {
    double ra_j2000, dec_j2000;
    ra_dec_to_j2000(ra_epoch_in, dec_epoch_in, jd_epoch_in, &ra_j2000, &dec_j2000);
    ra_dec_from_j2000(ra_j2000, dec_j2000, jd_epoch_out, ra_epoch_out, dec_epoch_out);
}

//! ra_dec_j2000_from_b1950 - Convert celestial coordinates from B1950 into J2000.
//! \param [in] ra_b1950_in - Input right ascension, in radians, B1950
//! \param [in] dec_b1950_in - Input declination, in radians, B1950
//! \param [out] ra_j2000_out - Output right ascension, in radians, J2000
//! \param [out] dec_j2000_out - Output declination, in radians, J2000

void ra_dec_j2000_from_b1950(double ra_b1950_in, double dec_b1950_in, double *ra_j2000_out, double *dec_j2000_out) {
    ra_dec_to_j2000(ra_b1950_in, dec_b1950_in, 2433282.4, ra_j2000_out, dec_j2000_out);
}

//! ra_dec_b1950_from_j2000 - Convert celestial coordinates from J2000 into B1950.
//! \param [in] ra_j2000_in - Input right ascension, in radians, J2000
//! \param [in] dec_j2000_in - Input declination, in radians, J2000
//! \param [out] ra_b1950_out - Output right ascension, in radians, B1950
//! \param [out] dec_b1950_out - Output declination, in radians, B1950
void ra_dec_b1950_from_j2000(double ra_j2000_in, double dec_j2000_in, double *ra_b1950_out, double *dec_b1950_out) {
    ra_dec_from_j2000(ra_j2000_in, dec_j2000_in, 2433282.4, ra_b1950_out, dec_b1950_out);
}

//! get_zenith_position - Calculate the right ascension and declination of the zenith
//! \param [in] latitude - The latitude of the observer, degrees
//! \param [in] longitude - The longitude of the observer, degrees
//! \param [in] julian_date - The Julian date of the observation
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

//! sun_pos - Calculate an estimate of the J2000.0 RA and Decl of the Sun at a particular Unix time.
//! See Jean Meeus, Astronomical Algorithms, pp 163-4
//! \param [in] julian_date - Julian date of query
//! \param [out] ra_j2000_out - Right ascension, J2000.0, hours
//! \param [out] dec_j2000_out - Declination, J2000.0, degrees
void sun_pos(double julian_date, double *ra_j2000_out, double *dec_j2000_out) {
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

    const double epsilon =
            23 + 26. / 60 + 21.448 / 3600 + 46.8150 / 3600 * t + 0.00059 / 3600 * t * t + 0.001813 / 3600 * t * t * t;

    double ra = 12 / M_PI * atan2(cos(epsilon * deg) * sin(tl * deg), cos(tl * deg));  // hours
    const double dec = 180 / M_PI * asin(sin(epsilon * deg) * sin(tl * deg)); // degrees

    // Ensure right ascension is in the range 0-24 hours
    while (ra < 0) {
        ra += 24;
    }

    // Return output
    *ra_j2000_out = ra;
    *dec_j2000_out = dec;
}
