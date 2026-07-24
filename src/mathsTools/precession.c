// precession.c
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

#include <math.h>

#include <gsl/gsl_math.h>

#include "precession.h"

// Precess celestial coordinates from one epoch to another

// For similar Javascript code, see:
// https://github.com/commenthol/astronomia/blob/master/src/base.js
// https://github.com/commenthol/astronomia/blob/master/src/precess.js

//! polynomial_horner - Evaluate a polynomial with coefficients c, at point x, using Horner's method.
//! The constant term is c[0]; the linear term is c[1]; etc.
//! \param x - Evaluate the polynomial at point x
//! \param c - The coefficients of the polynomial
//! \param length - The number of coefficients in the polynomial
//! \return - f(x)
double polynomial_horner(const double x, const double *c, const int length) {
    int i = length - 1;
    double y = c[i];
    while (i > 0) {
        i--;
        y = y * x + c[i];
    }
    return y;
}

//! ra_dec_switch_epoch - Convert equatorial coordinates at one epoch into equatorial coordinates at a different epoch.
//! \param [in] jd_in - The epoch of the input equatorial coordinates, expressed as a Julian day number.
//! \param [in] jd_out - The epoch of the output equatorial coordinates, expressed as a Julian day number.
//! \param [in] ra_epoch_in - Input right ascension, in radians, reference frame at epoch
//! \param [in] dec_epoch_in - Input declination, in radians, reference frame at epoch
//! \param [out] ra_epoch_out - Output right ascension, in radians, reference frame at epoch
//! \param [out] dec_epoch_out - Output declination, in radians, reference frame at epoch
void ra_dec_switch_epoch(const double jd_in, const double jd_out, const double ra_epoch_in, const double dec_epoch_in,
                         double *ra_epoch_out, double *dec_epoch_out) {
    // See "Astronomical Algorithms", Jean Meeus, Chapter 21, page 134

    // Convert epochs from JDs into Julian years
    const double year_in = (jd_in - 2451545.0) / 365.25 + 2000;
    const double year_out = (jd_out - 2451545.0) / 365.25 + 2000;

    const double T = (year_in - 2000) * 0.01; // Julian centuries
    const double t = (year_out - year_in) * 0.01; // Julian centuries

    // Special treatment of cases close to the pole
    const double smallAngle = 10 * M_PI / 180 / 60; // about .003 radians
    const double cosSmallAngle = cos(smallAngle); // cosine of SmallAngle; about .999996

    // Coefficients from equation (21.5)
    const double d = M_PI / 180; // degree
    const double s = d / 3600; // arcsecond
    const double zeta_T[3] = {2306.2181 * s, 1.39656 * s, -0.000139 * s}; // Inner polynomial terms
    const double z_T[3] = {2306.2181 * s, 1.39656 * s, -0.000139 * s};
    const double theta_T[3] = {2004.3109 * s, -0.85330 * s, -0.000217 * s};

    // Outer polynomial terms
    double zeta_coeff[3];
    double z_coeff[3];
    double theta_coeff[3];

    zeta_coeff[0] = polynomial_horner(T, zeta_T, 3);
    zeta_coeff[1] = 0.30188 * s - 0.000344 * s * T;
    zeta_coeff[2] = 0.017998 * s;

    z_coeff[0] = polynomial_horner(T, z_T, 3);
    z_coeff[1] = 1.09468 * s + 0.000066 * s * T;
    z_coeff[2] = 0.018203 * s;

    theta_coeff[0] = polynomial_horner(T, theta_T, 3);
    theta_coeff[1] = -0.42665 * s - 0.000217 * s * T;
    theta_coeff[2] = -0.041833 * s;

    const double zeta = polynomial_horner(t, zeta_coeff, 3) * t;
    const double z = polynomial_horner(t, z_coeff, 3) * t;
    const double theta = polynomial_horner(t, theta_coeff, 3) * t;
    const double sin_theta = sin(theta);
    const double cos_theta = cos(theta);

    // Equation (21.7); page 137
    const double sin_dec0 = sin(dec_epoch_in);
    const double cos_dec0 = cos(dec_epoch_in);
    const double sin_sum = sin(ra_epoch_in + zeta);
    const double cos_sum = cos(ra_epoch_in + zeta);

    const double a = cos_dec0 * sin_sum;
    const double b = cos_theta * cos_dec0 * cos_sum - sin_theta * sin_dec0;
    const double c = sin_theta * cos_dec0 * cos_sum + cos_theta * sin_dec0;

    *ra_epoch_out = atan2(a, b) + z;

    if (c < cosSmallAngle) {
        *dec_epoch_out = asin(c);
    } else {
        *dec_epoch_out = acos(hypot(a, b)); // near pole
    }

    // Check that output is in range
    while (*ra_epoch_out < 0) *ra_epoch_out += 2 * M_PI;
    while (*ra_epoch_out >= 2 * M_PI) *ra_epoch_out -= 2 * M_PI;
}

//! ra_dec_from_j2000 - Convert celestial coordinates from J2000 into a new epoch.
//! \param [in] ra_j2000_in - Input right ascension, in radians, J2000
//! \param [in] dec_j2000_in - Input declination, in radians, J2000
//! \param [in] jd_new - Julian date of the epoch we are to transform celestial coordinates into
//! \param [out] ra_epoch_out - Output right ascension, in radians, reference frame at epoch
//! \param [out] dec_epoch_out - Output declination, in radians, reference frame at epoch
void ra_dec_from_j2000(const double ra_j2000_in, const double dec_j2000_in, const double jd_new,
                       double *ra_epoch_out, double *dec_epoch_out) {
    ra_dec_switch_epoch(2451545.0, jd_new,
                        ra_j2000_in, dec_j2000_in,
                        ra_epoch_out, dec_epoch_out);
}

//! ra_dec_to_j2000 - Convert celestial coordinates to J2000 from another epoch.
//! \param [in] ra_epoch_in - Input right ascension, in radians, reference frame at epoch
//! \param [in] dec_epoch_in - Input declination, in radians, reference frame at epoch
//! \param [in] jd_old - Julian date of the epoch we are to transform celestial coordinates from
//! \param [out] ra_j2000_out - Output right ascension, in radians, J2000
//! \param [out] dec_j2000_out - Output declination, in radians, J2000
void ra_dec_to_j2000(double ra_epoch_in, double dec_epoch_in, double jd_old,
                     double *ra_j2000_out, double *dec_j2000_out) {
    ra_dec_switch_epoch(jd_old, 2451545.0,
                        ra_epoch_in, dec_epoch_in,
                        ra_j2000_out, dec_j2000_out);
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

//! ecl_switch_epoch - Convert ecliptic coordinates at one epoch into ecliptic coordinates at a different epoch.
//! \param [in] jd_in - The epoch of the input ecliptic coordinates, expressed as a Julian day number.
//! \param [in] jd_out - The epoch of the output ecliptic coordinates, expressed as a Julian day number.
//! \param [in] ecl_lng_in - Input ecliptic longitude (radians)
//! \param [in] ecl_lat_in - Input ecliptic latitude (radians)
//! \param [out] ecl_lng_out - Output ecliptic longitude (radians)
//! \param [out] ecl_lat_out - Output ecliptic latitude (radians)
void ecl_switch_epoch(const double jd_in, const double jd_out, const double ecl_lng_in, const double ecl_lat_in,
                      double *ecl_lng_out, double *ecl_lat_out) {
    // See "Astronomical Algorithms", Jean Meeus, Chapter 21, page 136

    // Convert epochs from JDs into Julian years
    const double year_in = (jd_in - 2451545.0) / 365.25 + 2000;
    const double year_out = (jd_out - 2451545.0) / 365.25 + 2000;

    const double T = (year_in - 2000) * 0.01; // Julian centuries
    const double t = (year_out - year_in) * 0.01; // Julian centuries

    // Special treatment of cases close to the pole
    const double smallAngle = 10 * M_PI / 180 / 60; // about .003 radians
    const double cosSmallAngle = cos(smallAngle); // cosine of SmallAngle; about .999996

    // Coefficients from equation (21.5)
    const double d = M_PI / 180; // degree
    const double s = d / 3600; // arcsecond
    const double eta_T[3] = {47.0029 * s, -0.06603 * s, 0.000598 * s}; // Inner polynomial terms
    const double pi_T[3] = {174.876384 * d, 3289.4789 * s, 0.60622 * s};
    const double p_T[3] = {5029.0966 * s, 2.22226 * s, -0.000042 * s};

    // Outer polynomial terms
    double eta_coeff[3];
    double pi_coeff[3];
    double p_coeff[3];

    eta_coeff[0] = polynomial_horner(T, eta_T, 3);
    eta_coeff[1] = -0.03302 * s + 0.000598 * s * T;
    eta_coeff[2] = 0.000060 * s;

    pi_coeff[0] = polynomial_horner(T, pi_T, 3);
    pi_coeff[1] = -869.8089 * s - 0.50491 * s * T;
    pi_coeff[2] = 0.03536 * s;

    p_coeff[0] = polynomial_horner(T, p_T, 3);
    p_coeff[1] = 1.11113 * s - 0.000042 * s * T;
    p_coeff[2] = -0.000006 * s;

    const double eta = polynomial_horner(t, eta_coeff, 3) * t;
    const double pi = polynomial_horner(t, pi_coeff, 3);
    const double p = polynomial_horner(t, p_coeff, 3) * t;
    const double s_eta = sin(eta);
    const double c_eta = cos(eta);

    // Equation (21.7); page 137
    const double s_beta = sin(ecl_lat_in);
    const double c_beta = cos(ecl_lat_in);
    const double sd = sin(pi - ecl_lng_in);
    const double cd = cos(pi - ecl_lng_in);

    const double A = c_eta * c_beta * sd - s_eta * s_beta;
    const double B = c_beta * cd;
    const double C = c_eta * s_beta + s_eta * c_beta * sd;

    *ecl_lng_out = p + pi - atan2(A, B);

    if (C < cosSmallAngle) {
        *ecl_lat_out = asin(C);
    } else {
        *ecl_lat_out = acos(hypot(A, B)); // near pole
    }
}
