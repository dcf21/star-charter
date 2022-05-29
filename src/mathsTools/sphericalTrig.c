// sphericalTrig.c
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

#include <stdlib.h>
#include <math.h>
#include <string.h>

#include <gsl/gsl_math.h>

#include "mathsTools/sphericalTrig.h"

//! angDist_RADec - Calculate the angular distance between (RA0, Dec0) and (RA1, Dec1)
//! \param ra0 - Right ascension of the first point (radians)
//! \param dec0 - Declination of the first point (radians)
//! \param ra1 - Right ascension of the second point (radians)
//! \param dec1 - Declination of the second point (radians)
//! \return Angular distance between two points (radians)

double angDist_RADec(double ra0, double dec0, double ra1, double dec1) {
    double p0x = sin(ra0) * cos(dec0);
    double p0y = cos(ra0) * cos(dec0);
    double p0z = sin(dec0);

    double p1x = sin(ra1) * cos(dec1);
    double p1y = cos(ra1) * cos(dec1);
    double p1z = sin(dec1);

    double sep2 = gsl_pow_2(p0x - p1x) + gsl_pow_2(p0y - p1y) + gsl_pow_2(p0z - p1z);
    if (sep2 <= 0) return 0;

    double sep = sqrt(sep2);
    return 2 * asin(sep / 2);
}

//! rotate_xy - Rotate a three-component vector about the z axis
//! \param [out] out Rotated vector
//! \param [in] in Vector to rotate
//! \param theta The angle to rotate around the z axis (radians)

void rotate_xy(double *out, double *in, double theta) {
    double t[3];
    memcpy(t, in, 3 * sizeof(double));
    out[0] = t[0] * cos(theta) + t[1] * -sin(theta);
    out[1] = t[0] * sin(theta) + t[1] * cos(theta);
    out[2] = t[2];
}

//! rotate_xz - Rotate a three-component vector about the y axis
//! \param [out] out Rotated vector
//! \param [in] in Vector to rotate
//! \param theta The angle to rotate around the y axis (radians)

void rotate_xz(double *out, double *in, double theta) {
    double t[3];
    memcpy(t, in, 3 * sizeof(double));
    out[0] = t[0] * cos(theta) + t[2] * -sin(theta);
    out[1] = t[1];
    out[2] = t[0] * sin(theta) + t[2] * cos(theta);
}

//! make_zenithal - Convert a position on the sky into alt/az coordinates
//! \param [out] zenith_angle The zenith angle of the point (radians); equals pi/2 - altitude
//! \param [out] azimuth The azimuth of the point (radians)
//! \param [in] ra The right ascension of the point to convert (radians)
//! \param [in] dec The declination of the point to convert (radians)
//! \param [in] ra0 The right ascension of the zenith (radians)
//! \param [in] dec0 The declination of the zenith (radians)

void make_zenithal(double *zenith_angle, double *azimuth, double ra, double dec, double ra0, double dec0) {
    double altitude;
    double x = cos(ra) * cos(dec);
    double y = sin(ra) * cos(dec);
    double z = sin(dec);
    double a[3] = {x, y, z};
    rotate_xy(a, a, -ra0);
    rotate_xz(a, a, (M_PI / 2) - dec0);
    if (a[2] > 0.999999999) a[2] = 1.0;
    if (a[2] < -0.999999999) a[2] = -1.0;
    altitude = asin(a[2]);
    if (fabs(cos(altitude)) < 1e-7) *azimuth = 0.0; // Ignore azimuth at pole!
    else *azimuth = atan2(a[1] / cos(altitude), a[0] / cos(altitude));
    *zenith_angle = (M_PI / 2) - altitude;
}

//! find_mean_position - Return the average of a list of points on the sky
//! \param [out] ra_out - The right ascension of the average of the input points
//! \param [out] dec_out - The declination of the average of the input points
//! \param [in] ra_list - The right ascension of the first point (radians)
//! \param [in] dec_list - The declination of the first point (radians)
//! \param [in] point_count - The number of points to average

void find_mean_position(double *ra_out, double *dec_out, const double *ra_list, const double *dec_list,
                        int point_count) {
    int i;
    double x_sum = 0, y_sum = 0, z_sum = 0;

    // Convert the points from spherical polar coordinates to Cartesian coordinates, and sum them
    for (i = 0; i < point_count; i++) {
        x_sum += cos(ra_list[i]) * cos(dec_list[i]);
        y_sum += sin(ra_list[i]) * cos(dec_list[i]);
        z_sum += sin(dec_list[i]);
    }

    // Work out the magnitude of the centroid vector
    double mag = sqrt(gsl_pow_2(x_sum) + gsl_pow_2(y_sum) + gsl_pow_2(z_sum));

    // Convert the Cartesian coordinates into RA and Dec
    *dec_out = asin(z_sum / mag);
    *ra_out = atan2(y_sum, x_sum);
}
