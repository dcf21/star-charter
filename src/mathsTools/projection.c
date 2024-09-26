// projection.c
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
#include <math.h>

#include <gsl/gsl_math.h>

#include "settings/chart_config.h"

#include "mathsTools/julianDate.h"
#include "mathsTools/sphericalTrig.h"
#include "mathsTools/projection.h"

//! galactic_project - Project a position on the sky from equatorial coordinates (RA, Dec) into galactic coordinates
//! \param [in] ra - The right ascension of the point to convert (radians)
//! \param [in] dec - The declination of the point to convert (radians)
//! \param [out] l_out - Galactic longitude (radians)
//! \param [out] b_out - Galactic latitude (radians)

void galactic_project(double ra, double dec, double *l_out, double *b_out) {
    const double l_cp = 123.932 * M_PI / 180;
    const double ra_gp = 192.85948 * M_PI / 180;
    const double dec_gp = 27.12825 * M_PI / 180;

    // See pp30-31 of Binney & Merrifield
    double b = asin(sin(dec) * sin(dec_gp) +
                    cos(dec_gp) * cos(dec) * cos(ra - ra_gp));
    const double l_sin = cos(dec) * sin(ra - ra_gp) / cos(b);
    const double l_cos = (cos(dec_gp) * sin(dec) - sin(dec_gp) * cos(dec) * cos(ra - ra_gp)) / cos(b);
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

//! inv_galactic_project - Project a position on the sky from galactic coordinates into equatorial coordinates (RA, Dec)
//! \param [out] ra_out - The output right ascension of the point (radians)
//! \param [out] dec_out - The output declination of the point (radians)
//! \param [in] l - Galactic longitude to convert (radians)
//! \param [in] b - Galactic latitude to convert (radians)

void inv_galactic_project(double *ra_out, double *dec_out, double l, double b) {
    const double l_cp = 123.932 * M_PI / 180;
    const double ra_gp = 192.85948 * M_PI / 180;
    const double dec_gp = 27.12825 * M_PI / 180;

    // See pp30-31 of Binney & Merrifield
    const double decl = asin(
            sin(b) * sin(dec_gp) + cos(dec_gp) * cos(b) * cos(l_cp - l));
    const double rsin = cos(b) * sin(l_cp - l) / cos(decl);
    const double rcos = (cos(dec_gp) * sin(b) - sin(dec_gp) * cos(b) * cos(l_cp - l)) / cos(decl);
    const double ra = ra_gp + atan2(rsin, rcos);
    *ra_out = ra;
    *dec_out = decl;
}

//! alt_az - Converts [RA, Dec] into local [altitude, azimuth]
//! \param [in] ra - The right ascension of the object, radians, epoch of observation.
//! \param [in] dec - The declination of the object, degrees, epoch of observation.
//! \param [in] julian_date - The Julian date of the observation
//! \param [in] latitude - The longitude of the observer, degrees
//! \param [in] longitude - The longitude of the observer, degrees
//! \param [out] alt - The output altitude of the object, radians
//! \param [out] az  - The output azimuth of the object, radians

void alt_az(double ra, double dec, double julian_date, double latitude, double longitude, double *alt, double *az) {
    const double utc = unix_from_jd(julian_date);
    const double st = sidereal_time(utc) * M_PI / 12 + longitude * M_PI / 180;
    const double xyz[3] = {
            sin(ra) * cos(dec),
            -sin(dec),  // y-axis = towards south pole
            cos(ra) * cos(dec) // z-axis = vernal equinox; RA=0
    };

    // Rotate by hour angle around y-axis
    const double xyz2[3] = {
            xyz[0] * cos(st) - xyz[2] * sin(st),
            xyz[1],
            xyz[0] * sin(st) + xyz[2] * cos(st)
    };

    // Rotate by latitude around x-axis
    const double t = M_PI / 2 - latitude * M_PI / 180;
    const double xyz3[3] = {
            xyz2[0],
            xyz2[1] * cos(t) - xyz2[2] * sin(t),
            xyz2[1] * sin(t) + xyz2[2] * cos(t)
    };

    *alt = -asin(xyz3[1]);
    *az = atan2(xyz3[0], -xyz3[2]);
}

//! inv_alt_az - Converts local [altitude, azimuth] into [RA, Dec] at epoch
//! \param [in] alt - The input altitude of the object, radians
//! \param [in] az - The input azimuth of the object, radians
//! \param [in] julian_date - The Julian date of the observation
//! \param [in] latitude - The latitude of the observer, degrees
//! \param [in] longitude - The longitude of the observer, degrees
//! \param [out] ra_out - The output right ascension of the object, radians, epoch of observation.
//! \param [out] dec_out - The output declination of the object, degrees, epoch of observation.

void inv_alt_az(double alt, double az, double julian_date, double latitude, double longitude,
                double *ra_out, double *dec_out) {
    const double utc = unix_from_jd(julian_date);
    const double st = sidereal_time(utc) * M_PI / 12 + longitude * M_PI / 180;
    const double xyz3[3] = {
            sin(az) * cos(alt),
            sin(-alt),
            -cos(az) * cos(alt)
    };

    // Rotate by latitude around x-axis
    const double t = M_PI / 2 - latitude * M_PI / 180;
    const double xyz2[3] = {
            xyz3[0],
            xyz3[1] * cos(t) + xyz3[2] * sin(t),
            -xyz3[1] * sin(t) + xyz3[2] * cos(t)
    };

    // Rotate by hour angle around y-axis
    const double xyz[3] = {
            xyz2[0] * cos(st) + xyz2[2] * sin(st),
            xyz2[1],
            -xyz2[0] * sin(st) + xyz2[2] * cos(st)
    };

    double dec = -asin(xyz[1]);
    double ra = atan2(xyz[0], xyz[2]);

    while (ra < 0) ra += 2 * M_PI;

    *ra_out = ra;
    *dec_out = dec;
}

//! convert_ra_dec_to_selected_coordinates - Convert (RA, Dec) to the user's selected coordinate
//! system. This is used for flat projections which are coordinate-system dependent.
//! \param [in] s - Settings for the star chart we are drawing, including projection information
//! \param [in] ra - The right ascension of the point to project (radians)
//! \param [in] dec - The declination of the point to project (radians)
//! \param [out] lat_out - The latitude coordinate of the input point (radians)
//! \param [out] lng_out - The longitude coordinate of the input point (radians)

void convert_ra_dec_to_selected_coordinates(const chart_config *s, int coords, double ra, double dec,
                                            double *lat_out, double *lng_out) {
    if (coords == SW_COORDS_GALACTIC) {
        // Project (RA,Dec) into (l,b)
        galactic_project(ra, dec, lng_out, lat_out);
    } else if (coords == SW_COORDS_ALTAZ) {
        // Project (RA,Dec) into (alt,az)
        double ra_at_epoch, dec_at_epoch;
        ra_dec_from_j2000(ra, dec, s->julian_date, &ra_at_epoch, &dec_at_epoch);
        alt_az(ra_at_epoch, dec_at_epoch, s->julian_date, s->horizon_latitude, s->horizon_longitude, lng_out, lat_out);
    } else {
        *lat_out = dec;
        *lng_out = ra;
    }
}

//! convert_selected_coordinates_to_ra_dec - Convert position in user's selected coordinates to (RA, Dec)
//! system.
//! \param [in] s - Settings for the star chart we are drawing, including projection information
//! \param [in] lng_in - The longitude coordinate of the point to project (radians)
//! \param [in] lat_in - The latitude coordinate of the point to project (radians)
//! \param [out] ra_out - The right ascension of the point to project (radians; J2000)
//! \param [out] dec_out - The declination of the point to project (radians; J2000)

void convert_selected_coordinates_to_ra_dec(const chart_config *s, int coords, double lng_in, double lat_in,
                                            double *ra_out, double *dec_out) {
    if (coords == SW_COORDS_GALACTIC) {
        // Convert galactic coordinates of centre of field of view into (RA, Dec) J2000
        inv_galactic_project(ra_out, dec_out, lng_in, lat_in);
    } else if (coords == SW_COORDS_ALTAZ) {
        // Convert alt/az of centre of field of view into (RA, Dec) at epoch <julian_date>
        double ra_at_epoch, dec_at_epoch;
        inv_alt_az(lat_in, lng_in, s->julian_date, s->horizon_latitude, s->horizon_longitude,
                   &ra_at_epoch, &dec_at_epoch);

        // Convert (RA, Dec) at epoch <julian_date> to J2000
        ra_dec_to_j2000(ra_at_epoch, dec_at_epoch, s->julian_date, ra_out, dec_out);
    } else {
        // Centre of field-of-view is already specified in (RA, Dec)
        *ra_out = lng_in;  // radians
        *dec_out = lat_in;  // radians
    }
}

//! plane_project_flat - Project a pair of celestial coordinates (RA, Dec) into pixel coordinates (x,y)
//! \param [out] x The output x position of the point (radians in tangent plane)
//! \param [out] y The output y position of the point (radians in tangent plane)
//! \param [in] s - Settings for the star chart we are drawing, including projection information
//! \param [in] ra - The right ascension of the point to project (radians)
//! \param [in] dec - The declination of the point to project (radians)

void plane_project_flat(double *x, double *y, const chart_config *s, double ra, double dec) {
    double lat_in, lng_in, lat_0, lng_0;
    convert_ra_dec_to_selected_coordinates(s, s->coords, ra, dec, &lat_in, &lng_in);
    convert_ra_dec_to_selected_coordinates(s, s->coords, s->ra0_final, s->dec0_final, &lat_0, &lng_0);

    // Don't do gnomonic transformations; just return flat difference in RA,Dec
    double x0 = lng_0 - lng_in;
    double y0 = lat_0 - lat_in;

    // Right ascension axis wraps around. Don't do this with declination!
    while (x0 < -M_PI) x0 += 2 * M_PI;
    while (x0 > M_PI) x0 -= 2 * M_PI;

    // Rotate by position angle
    const double pa = s->position_angle * M_PI / 180.;
    const double x1 = x0 * cos(pa) - y0 * sin(pa);
    const double y1 = x0 * sin(pa) + y0 * cos(pa);

    // Return output
    *x = x1;
    *y = y1;
}

//! plane_project_peters - Project a pair of celestial coordinates (RA, Dec) into pixel coordinates (x,y)
//! \param [out] x The output x position of the point (radians in tangent plane)
//! \param [out] y The output y position of the point (radians in tangent plane)
//! \param [in] s - Settings for the star chart we are drawing, including projection information
//! \param [in] ra - The right ascension of the point to project (radians)
//! \param [in] dec - The declination of the point to project (radians)

void plane_project_peters(double *x, double *y, const chart_config *s, double ra, double dec) {
    double lat_in, lng_in, lat_0, lng_0;
    convert_ra_dec_to_selected_coordinates(s, s->coords, ra, dec, &lat_in, &lng_in);
    convert_ra_dec_to_selected_coordinates(s, s->coords, s->ra0_final, s->dec0_final, &lat_0, &lng_0);

    // Don't do gnomonic transformations; just return flat difference in RA,Dec
    double x0 = lng_0 - lng_in;

    // Peters projection
    double y0 = 2 * (sin(lat_0) - sin(lat_in));

    // Right ascension axis wraps around. Don't do this with declination!
    while (x0 < -M_PI) x0 += 2 * M_PI;
    while (x0 > M_PI) x0 -= 2 * M_PI;

    // Rotate by position angle
    const double pa = s->position_angle * M_PI / 180.;
    const double x1 = x0 * cos(pa) - y0 * sin(pa);
    const double y1 = x0 * sin(pa) + y0 * cos(pa);

    // Return output
    *x = x1;
    *y = y1;
}

//! plane_project_multilatitude - Project a pair of celestial coordinates (RA, Dec) onto a multi-latitude plot
//! \param [out] x The output x position of the point (radians in tangent plane)
//! \param [out] y The output y position of the point (radians in tangent plane)
//! \param [in] s - Settings for the star chart we are drawing, including projection information
//! \param [in] ra - The right ascension of the point to project (radians)
//! \param [in] dec - The declination of the point to project (radians)

void plane_project_multilatitude(double *x, double *y, const chart_config *s, double ra, double dec) {
    // Fetch sidereal time at epoch
    const double st_hr = sidereal_time(unix_from_jd(s->julian_date)); // hours
    const double st_radians = st_hr * M_PI / 12;

    // Longitude of point relative to the prime meridian
    const double lng = ra - st_radians;
    const double dec0 = dec;

    // Project point into Cartesian coordinates
    const double a[3] = {
            cos(lng) * cos(dec0), // Directed towards the prime meridian at epoch
            sin(lng) * cos(dec0),
            sin(dec0) // Directed towards north celestial pole
    };

    // Latitude at which the point is on the local horizon
    const double y_plane = -atan2(a[2], a[0]);  // radians

    // Project point into alt/az coordinate at the latitude where it is on the horizon
    double a_local[3];
    rotate_xz(a_local, a, y_plane);

    // Longitude of the point where it is on the horizon
    const double x_plane = atan2(a_local[1], a_local[0]);

    // We use RA/Dec as a shift in the image plane
    *x = x_plane + s->ra0_final;
    *y = -y_plane + s->dec0_final;
}

//! plane_project_spherical - Project a pair of celestial coordinates (RA, Dec) into pixel coordinates (x,y)
//! \param [out] x The output x position of the point (radians in tangent plane)
//! \param [out] y The output y position of the point (radians in tangent plane)
//! \param [in] s - Settings for the star chart we are drawing, including projection information
//! \param [in] ra - The right ascension of the point to project (radians)
//! \param [in] dec - The declination of the point to project (radians)

void plane_project_spherical(double *x, double *y, const chart_config *s, double ra, double dec) {
    double azimuth, radius = 0, zenith_angle;

    make_zenithal(&zenith_angle, &azimuth, ra, dec, s->ra0_final, s->dec0_final);
    azimuth -= s->position_angle * M_PI / 180;
    if ((zenith_angle > M_PI / 2) && (s->projection != SW_PROJECTION_STEREOGRAPHIC)) {
        // Opposite side of sphere!
        *x = *y = GSL_NAN;
        return;
    }

    if (s->projection == SW_PROJECTION_GNOMONIC) {
        radius = tan(zenith_angle);
    } else if (s->projection == SW_PROJECTION_STEREOGRAPHIC) {
        radius = tan(zenith_angle / 2);
    } else if (s->projection == SW_PROJECTION_SPHERICAL) {
        radius = sin(zenith_angle);
    } else if (s->projection == SW_PROJECTION_ALTAZ) {
        radius = zenith_angle / (M_PI / 2);
    }

    *y = radius * cos(azimuth);
    *x = radius * -sin(azimuth);
}

//! plane_project - Project a pair of celestial coordinates (RA, Dec) into pixel coordinates (x,y)
//! \param [out] x The output x position of the point (radians in tangent plane)
//! \param [out] y The output y position of the point (radians in tangent plane)
//! \param [in] s - Settings for the star chart we are drawing, including projection information
//! \param [in] ra - The right ascension of the point to project (radians)
//! \param [in] dec - The declination of the point to project (radians)
//! \param [in] allow_below_horizon - Return positions for objects below the horizon (bool)

void plane_project(double *x, double *y, const chart_config *s, const double ra, const double dec,
                   const int allow_below_horizon) {
    if (s->projection == SW_PROJECTION_FLAT) {
        plane_project_flat(x, y, s, ra, dec);
    } else if (s->projection == SW_PROJECTION_PETERS) {
        plane_project_peters(x, y, s, ra, dec);
    } else if (s->projection == SW_PROJECTION_MULTILATITUDE) {
        plane_project_multilatitude(x, y, s, ra, dec);
    } else {
        plane_project_spherical(x, y, s, ra, dec);
    }

    // If we are showing the horizon, then hide all objects beneath the horizon
    if (s->show_horizon && !allow_below_horizon) {
        // Find the coordinates of the zenith
        double ra_zenith_at_epoch, dec_zenith_at_epoch;
        double ra_zenith_j2000, dec_zenith_j2000;
        get_zenith_position(s->horizon_latitude, s->horizon_longitude, s->julian_date,
                            &ra_zenith_at_epoch, &dec_zenith_at_epoch);
        ra_dec_to_j2000(ra_zenith_at_epoch, dec_zenith_at_epoch, s->julian_date,
                        &ra_zenith_j2000, &dec_zenith_j2000);

        // Zenith angle
        const double zenith_angle = angDist_RADec(ra_zenith_j2000, dec_zenith_j2000, ra, dec);

        // Reject zenith angles greater than 90 degrees
        if (zenith_angle > M_PI / 2) {
            *x = *y = GSL_NAN;
        }
    }
}

//! inv_plane_project_flat - Project a pair of pixel coordinates (x,y) into celestial coordinates (RA, Dec)
//! \param [in] x The input x position of the point (radians in tangent plane)
//! \param [in] y The input y position of the point (radians in tangent plane)
//! \param [in] s - Settings for the star chart we are drawing, including projection information
//! \param [out] ra - The output right ascension of the point to project (radians)
//! \param [out] dec - The output declination of the point to project (radians)

void inv_plane_project_flat(double *ra, double *dec, const chart_config *s, double x, double y) {
    double lat_0, lng_0;
    convert_ra_dec_to_selected_coordinates(s, s->coords, s->ra0_final, s->dec0_final, &lat_0, &lng_0);

    // Don't do gnomonic transformations; just return flat difference in RA,Dec
    const double pa = s->position_angle * M_PI / 180.;
    const double xp = x * cos(pa) + y * sin(pa);
    const double yp = -x * sin(pa) + y * cos(pa);
    const double lng_out = lng_0 - xp;
    const double lat_out = lat_0 - yp;
    convert_selected_coordinates_to_ra_dec(s, s->coords, lng_out, lat_out, ra, dec);
}

//! inv_plane_project_peters - Project a pair of pixel coordinates (x,y) into celestial coordinates (RA, Dec)
//! \param [in] x The input x position of the point (radians in tangent plane)
//! \param [in] y The input y position of the point (radians in tangent plane)
//! \param [in] s - Settings for the star chart we are drawing, including projection information
//! \param [out] ra - The output right ascension of the point to project (radians)
//! \param [out] dec - The output declination of the point to project (radians)

void inv_plane_project_peters(double *ra, double *dec, const chart_config *s, double x, double y) {
    double lat_0, lng_0;
    convert_ra_dec_to_selected_coordinates(s, s->coords, s->ra0_final, s->dec0_final, &lat_0, &lng_0);

    // Don't do gnomonic transformations; just return flat difference in RA,Dec
    const double pa = s->position_angle * M_PI / 180.;
    const double xp = x * cos(pa) + y * sin(pa);
    const double yp = -x * sin(pa) + y * cos(pa);
    const double lng_out = lng_0 - xp;
    const double lat_out = asin((2 * sin(lat_0) - yp) / 2);
    convert_selected_coordinates_to_ra_dec(s, s->coords, lng_out, lat_out, ra, dec);
}

//! inv_plane_project_multilatitude - Project a pair of pixel coordinates (x,y) into celestial coordinates (RA, Dec)
//! \param [in] x The input x position of the point (radians in tangent plane)
//! \param [in] y The input y position of the point (radians in tangent plane)
//! \param [in] s - Settings for the star chart we are drawing, including projection information
//! \param [out] ra - The output right ascension of the point to project (radians)
//! \param [out] dec - The output declination of the point to project (radians)

void inv_plane_project_multilatitude(double *ra, double *dec, const chart_config *s, double x, double y) {
    // We use RA/Dec as a shift in the image plane
    const double x_plane = x - s->ra0_final;
    const double y_plane = -(y - s->dec0_final);

    const double a_local[3] = {
            cos(x_plane),
            sin(x_plane),
            0
    };

    // Project point from alt/az coordinate at the latitude where it is on the horizon
    double a[3];
    rotate_xz(a, a_local, -y_plane);

    const double dec0 = asin(a[2]);
    const double lng = atan2(a[1], a[0]);

    // Fetch sidereal time at epoch
    const double st_hr = sidereal_time(unix_from_jd(s->julian_date)); // hours
    const double st_radians = st_hr * M_PI / 12;

    // Longitude of point relative to the prime meridian
    const double ra0 = lng + st_radians;

    *ra = ra0;
    *dec = dec0;
}

//! inv_plane_project_spherical - Project a pair of pixel coordinates (x,y) into celestial coordinates (RA, Dec)
//! \param [in] x The input x position of the point (radians in tangent plane)
//! \param [in] y The input y position of the point (radians in tangent plane)
//! \param [in] s - Settings for the star chart we are drawing, including projection information
//! \param [out] ra - The output right ascension of the point to project (radians)
//! \param [out] dec - The output declination of the point to project (radians)

void inv_plane_project_spherical(double *ra, double *dec, const chart_config *s, double x, double y) {
    double za = 0.0;

    if (s->projection == SW_PROJECTION_GNOMONIC) {
        za = atan(hypot(x, y));
    } else if (s->projection == SW_PROJECTION_STEREOGRAPHIC) {
        za = atan(hypot(x, y)) * 2;
    } else if (s->projection == SW_PROJECTION_SPHERICAL) {
        za = asin(hypot(x, y));
    } else if (s->projection == SW_PROJECTION_ALTAZ) {
        za = hypot(x, y) * (M_PI / 2);
    }

    const double az = atan2(-x, y) + s->position_angle * M_PI / 180.;

    const double altitude = M_PI / 2 - za;
    double a[3] = {cos(altitude) * cos(az), cos(altitude) * sin(az), sin(altitude)};

    if ((altitude < 0) && (s->projection != SW_PROJECTION_STEREOGRAPHIC)) {
        *ra = *dec = GSL_NAN;
        return;
    }

    rotate_xz(a, a, -(M_PI / 2) + s->dec0_final);
    rotate_xy(a, a, s->ra0_final);

    *ra = atan2(a[1], a[0]);
    *dec = asin(a[2]);
}

//! inv_plane_project - Project a pair of pixel coordinates (x,y) into celestial coordinates (RA, Dec)
//! \param [in] x The input x position of the point (radians in tangent plane)
//! \param [in] y The input y position of the point (radians in tangent plane)
//! \param [in] s - Settings for the star chart we are drawing, including projection information
//! \param [out] ra - The output right ascension of the point to project (radians)
//! \param [out] dec - The output declination of the point to project (radians)

void inv_plane_project(double *ra, double *dec, const chart_config *s, double x, double y) {
    if (s->projection == SW_PROJECTION_FLAT) {
        inv_plane_project_flat(ra, dec, s, x, y);
    } else if (s->projection == SW_PROJECTION_PETERS) {
        inv_plane_project_peters(ra, dec, s, x, y);
    } else if (s->projection == SW_PROJECTION_MULTILATITUDE) {
        inv_plane_project_multilatitude(ra, dec, s, x, y);
    } else {
        inv_plane_project_spherical(ra, dec, s, x, y);
    }

    // Ensure that output RA is within the allowed range of 0 - 2pi
    while ((*ra) < 0) (*ra) += 2 * M_PI;
    while ((*ra) > 2 * M_PI) (*ra) -= 2 * M_PI;
}
