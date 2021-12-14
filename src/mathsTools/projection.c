// projection.c
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

#include <gsl/gsl_math.h>

#include "settings/chart_config.h"

#include "mathsTools/sphericalTrig.h"
#include "mathsTools/projection.h"

//! galacticProject - Project a position on the sky from equatorial coordinates (RA, Dec) into galactic coordinates
//! \param ra - The right ascension of the point to convert (radians)
//! \param dec - The declination of the point to convert (radians)
//! \param l_out - Galactic longitude (radians)
//! \param b_out - Galactic latitude (radians)

void galactic_project(double ra, double dec, double *l_out, double *b_out) {
    double l_cp = 123.932 * M_PI / 180;
    double ra_gp = 192.85948 * M_PI / 180;
    double dec_gp = 27.12825 * M_PI / 180;
    double b = asin(sin(dec) * sin(dec_gp) +
                    cos(dec_gp) * cos(dec) * cos(ra - ra_gp)); // See pp30-31 of Binney & Merrifield
    double l_sin = cos(dec) * sin(ra - ra_gp) / cos(b);
    double l_cos = (cos(dec_gp) * sin(dec) - sin(dec_gp) * cos(dec) * cos(ra - ra_gp)) / cos(b);
    double l = l_cp - atan2(l_sin, l_cos);

    // Make sure that output angles are within range
    while (b < -M_PI) b += 2 * M_PI;
    while (b > M_PI) b -= 2 * M_PI;
    while (l < 0) l += 2 * M_PI;
    while (l > 2 * M_PI) l -= 2 * M_PI;

    // Output angles
    *l_out = l;
    *b_out = b;
}

//! gnomonic_project - Project a pair of celestial coordinates (RA, Dec) into pixel coordinates (x,y)
//! \param [out] x The x position of (RA, Dec)
//! \param [out] y The y position of (RA, Dec)
//! \param [in] s - Settings for the star chart we are drawing, including projection information
//! \param [in] lng - The right ascension of the point to project (radians)
//! \param [in] lat - The declination of the point to project (radians)
//! \param [in] grid_line - Boolean indicating whether this is a grid-line. When using galactic coordinates, the grid
//! lines have coordinates specified in galactic coordinates, while everything else is specified in RA/Dec, and needs
//! to be converted into galactic coordinates.

void plane_project(double *x, double *y, chart_config *s, double lng, double lat, int grid_line) {
    double azimuth, radius = 0, zenith_angle;

    if ((s->coords == SW_COORDS_GAL) && (!grid_line))
    {
        // Project (RA,Dec) into (l,b)
        galactic_project(lng, lat, &lng, &lat);
    }

    if (s->projection == SW_PROJECTION_FLAT) {
        // Don't do gnomonic transformations; just return flat difference in RA,Dec
        *x = s->ra0 - lng;
        *y = s->dec0 - lat;

        // Right ascension axis wraps around. Don't do this with declination!
        while (*x < -M_PI) *x += 2 * M_PI;
        while (*x > M_PI) *x -= 2 * M_PI;
        return;
    } else if (s->projection == SW_PROJECTION_PETERS) {
        // Don't do gnomonic transformations; just return flat difference in RA,Dec
        *x = s->ra0 - lng;
        // Peters projection
        *y = 2 * (sin(s->dec0) - sin(lat));

        // Right ascension axis wraps around. Don't do this with declination!
        while (*x < -M_PI) *x += 2 * M_PI;
        while (*x > M_PI) *x -= 2 * M_PI;
        return;
    }
    make_zenithal(&zenith_angle, &azimuth, lng, lat, s->ra0, s->dec0);
    azimuth -= s->position_angle * M_PI / 180;
    if (zenith_angle > M_PI / 2) {
        // Opposite side of sphere!
        *x = *y = GSL_NAN;
        return;
    }

    if (s->projection == SW_PROJECTION_GNOM) radius = tan(zenith_angle);
    else if (s->projection == SW_PROJECTION_SPH) radius = sin(zenith_angle);
    else if (s->projection == SW_PROJECTION_ALTAZ) radius = zenith_angle / (M_PI / 2);

    *y = radius * cos(azimuth);
    *x = radius * -sin(azimuth);
}

//! gnomonic_project - Project a pair of pixel coordinates (x,y) into celestial coordinates (RA, Dec)
//! \param [in] x The x position of (RA, Dec)
//! \param [in] y The y position of (RA, Dec)
//! \param [in] s - Settings for the star chart we are drawing, including projection information
//! \param [out] ra - The right ascension of the point to project (radians)
//! \param [out] dec - The declination of the point to project (radians)

void inv_plane_project(double *ra, double *dec, chart_config *s, double x, double y) {
    double za = 0.0;

    if (s->projection == SW_PROJECTION_GNOM) za = atan(hypot(x, y));
    else if (s->projection == SW_PROJECTION_SPH) za = asin(hypot(x, y));
    else if (s->projection == SW_PROJECTION_ALTAZ) za = hypot(x, y) * (M_PI / 2);

    if (s->projection == SW_PROJECTION_FLAT) {
        double pa = s->position_angle * M_PI / 180.;
        double xp = x * cos(pa) + y * sin(pa);
        double yp = -x * sin(pa) + y * cos(pa);
        *ra = s->ra0 - xp; // Don't do gnomonic transformations; just return flat difference in RA,Dec
        *dec = s->dec0 - yp;
    } else if (s->projection == SW_PROJECTION_PETERS) {
        double pa = s->position_angle * M_PI / 180.;
        double xp = x * cos(pa) + y * sin(pa);
        double yp = -x * sin(pa) + y * cos(pa);
        *ra = s->ra0 - xp; // Don't do gnomonic transformations; just return flat difference in RA,Dec
        *dec = asin((2 * sin(s->dec0) - yp) / 2);
    } else {
        double az = atan2(-x, y) + s->position_angle * M_PI / 180.;

        double altitude = M_PI / 2 - za;
        double a[3] = {cos(altitude) * cos(az), cos(altitude) * sin(az), sin(altitude)};

        if (altitude < 0) {
            *ra = *dec = GSL_NAN;
            return;
        }

        rotate_xz(a, a, -(M_PI / 2) + s->dec0);
        rotate_xy(a, a, s->ra0);

        *ra = atan2(a[1], a[0]);
        *dec = asin(a[2]);
    }

    if (s->coords == SW_COORDS_GAL) // Project (l,b) into (RA,Dec)
    {
        double l = *ra;
        double b = *dec;
        double l_cp = 123.932 * M_PI / 180;
        double ra_gp = 192.85948 * M_PI / 180;
        double dec_gp = 27.12825 * M_PI / 180;
        double DEC = asin(
                sin(b) * sin(dec_gp) + cos(dec_gp) * cos(b) * cos(l_cp - l)); // See pp30-31 of Binney & Merrifield
        double rsin = cos(b) * sin(l_cp - l) / cos(DEC);
        double rcos = (cos(dec_gp) * sin(b) - sin(dec_gp) * cos(b) * cos(l_cp - l)) / cos(DEC);
        double RA = ra_gp + atan2(rsin, rcos);
        *ra = RA;
        *dec = DEC;
    }
    while ((*ra) < 0) (*ra) += 2 * M_PI;
    while ((*ra) > 2 * M_PI) (*ra) -= 2 * M_PI;
}
