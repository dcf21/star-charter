// magnitudeEstimate.c
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

#define MAGNITUDEESTIMATE_C 1

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <gsl/gsl_math.h>
#include <gsl/gsl_const_mksa.h>

#include "coreUtils/errorReport.h"
#include "listTools/ltMemory.h"
#include "mathsTools/sphericalTrig.h"

#include "alias.h"
#include "magnitudeEstimate.h"
#include "orbitalElements.h"

// The maximum object ID for which we hold tables of data for estimating brightness
#define MAX_BODYID 128

// Array of the albedos of objects (geometric)
double *albedo_array = NULL;

// Array of the radii of objects, in metres
double *phy_size_array = NULL;

//! magnitudeEstimate_init - Initialise tables of data we use to estimate the magnitudes of objects

void magnitudeEstimate_init() {
    int i;

    // Allocate an array to store the sizes and albedos of the planets
    albedo_array = (double *) lt_malloc(MAX_BODYID * sizeof(double));
    phy_size_array = (double *) lt_malloc(MAX_BODYID * sizeof(double));
    if ((albedo_array == NULL) || (phy_size_array == NULL)) {
        ephem_fatal(__FILE__, __LINE__, "Malloc fail.");
        exit(1);
    }
    for (i = 0; i < MAX_BODYID; i++) albedo_array[i] = 0.0;
    for (i = 0; i < MAX_BODYID; i++) phy_size_array[i] = 0.0;

    // The sizes and albedos of the planets; data from various sources

    // Mercury
    phy_size_array[0] = 2.4397e6;
    albedo_array[0] = 0.138;

    // Venus
    phy_size_array[1] = 6.0518e6;
    albedo_array[1] = 0.67;

    // Earth
    phy_size_array[2] = phy_size_array[19] = 6.3710e6;
    albedo_array[2] = albedo_array[19] = 0.367;

    // Mars
    phy_size_array[3] = 3.3962e6;
    albedo_array[3] = 0.15;

    // Jupiter
    phy_size_array[4] = 6.9911e7;
    albedo_array[4] = 0.52;

    // Saturn
    phy_size_array[5] = 6.0268e7;
    albedo_array[5] = 0.47;

    // Uranus
    phy_size_array[6] = 2.5559e7;
    albedo_array[6] = 0.51;

    // Neptune
    phy_size_array[7] = 2.4764e7;
    albedo_array[7] = 0.41;

    // Pluto
    phy_size_array[8] = 1.1570e6;
    albedo_array[8] = (0.49 + 0.66) / 2.;

    // Moon
    phy_size_array[9] = 1.7371e6;
    albedo_array[9] = 0.113;

    // Sun
    phy_size_array[10] = 1.392e9 / 2;
    albedo_array[10] = 1;
}

//! magnitudeEstimate
//! \param [in] body_id - Id number of body to have ephemeris computed. 0=Mercury. 2=Earth/Moon barycentre. 9=Pluto.
//! 10=Sun. 19=Geocentre. 1e7+n=Asteroid n. 2e7+n=Comet n.
//! \param [in] xo - x,y,z position of body, in AU relative to solar system barycentre.
//! \param [in] yo - negative x points to vernal equinox. z points to celestial north pole (i.e. J2000.0).
//! \param [in] zo
//! \param [in] xe - x,y,z position of Earth
//! \param [in] ye
//! \param [in] ze
//! \param [in] xs - x,y,z position of Sun
//! \param [in] ys
//! \param [in] zs
//! \param [out] ra - Right ascension of the object
//! \param [out] dec - Declination of the object
//! \param [out] mag - Estimated V-band magnitude of the object
//! \param [out] phase - Phase of the object (0-1)
//! \param [out] angSize - Angular size of the object
//! \param [out] phySize - Physical size of the object
//! \param [out] albedoOut - Albedo of the object
//! \param [out] sunDist - Distance of the object from the Sun
//! \param [out] earthDist - Distance of the object from the Earth
//! \param [out] sunAngDist - Angular distance of the object from the Sun, as seen from the Earth
//! \param [out] theta_eso - Angular distance of the object from the Earth, as seen from the Sun
//! \param [out] eclipticLongitude - The ecliptic longitude of the object
//! \param [out] eclipticLatitude - The ecliptic latitude of the object
//! \param [out] eclipticDistance - The separation of the object from the Sun, in ecliptic longitude

void magnitudeEstimate(int body_id, double xo, double yo, double zo, double xe, double ye, double ze, double xs,
                       double ys, double zs, double *ra, double *dec, double *mag, double *phase, double *angSize,
                       double *phySize, double *albedoOut, double *sunDist, double *earthDist, double *sunAngDist,
                       double *theta_eso, double *eclipticLongitude, double *eclipticLatitude,
                       double *eclipticDistance) {
    // Distance of object from Sun
    const double Dso = sqrt(gsl_pow_2(xs - xo) + gsl_pow_2(ys - yo) + gsl_pow_2(zs - zo));

    // Distance of object from Earth
    const double Deo = sqrt(gsl_pow_2(xe - xo) + gsl_pow_2(ye - yo) + gsl_pow_2(ze - zo));

    // Angle <Earth - Object - Sun>
    const double theta = angDist_ABC(xe, ye, ze, xo, yo, zo, xs, ys, zs);
    const double theta_deg = theta * 180 / M_PI;

    double albedo, Ro;

    // The phase of the object, in range 0 to 1
    *phase = (1 + cos(theta)) / 2;

    // For comets, we always assume full phase when calculating magnitudes
    if (body_id >= 2e7) *phase = 1;

    // Body is a planet, the Moon or Sun
    if (body_id < MAX_BODYID) {
#pragma omp critical (MagnitudeEstimate_init)
        {
            // Make sure that the array of albedos and radii of solar system objects had been initialised
            if (albedo_array == NULL) magnitudeEstimate_init();
        }

        // Look up the albedo of this object, and its radius in AU
        albedo = albedo_array[body_id];
        Ro = phy_size_array[body_id] / GSL_CONST_MKSA_ASTRONOMICAL_UNIT; // Radius of object in AU

        if (body_id == 0) {
            // Empirical formula for the magnitude of Mercury
            // http://adsabs.harvard.edu/abs/2005AJ....129.2902H
            // http://aa.usno.navy.mil/publications/reports/Hilton2005a.pdf
            *mag = 5 * log10(Dso * Deo) - 0.60 + 4.98 * (theta_deg / 100) - 4.88 * gsl_pow_2(theta_deg / 100) +
                   3.02 * gsl_pow_3(theta_deg / 100);
        } else if (body_id == 1) {
            // Empirical formula for the magnitude of Venus
            // http://adsabs.harvard.edu/abs/2005AJ....129.2902H
            // http://aa.usno.navy.mil/publications/reports/Hilton2005a.pdf
            *mag = 5 * log10(Dso * Deo) - 4.4 + 0.09 * (theta_deg / 100) + 2.39 * gsl_pow_2(theta_deg / 100) -
                   0.65 * gsl_pow_3(theta_deg / 100);
        } else if (body_id == 3) {
            // Empirical formula for the magnitude of Mars
            // See page 285 of Jean Meeus, Astronomical Algorithms
            *mag = -1.52 + 5 * log10(Dso * Deo) + 0.016 * theta_deg;
        } else if (body_id == 5) {
            // Empirical formula for the magnitude of Saturn
            // This is a special case because of its ring
            *mag = -8.88 + 5 * log10(Dso * Deo);
            const double satpole[3] = {0.0856252844139301, 0.0733006357177292, 0.9936273584560815};
            //const double sunDot = (xo-xs)*satpole[0] + (yo-ys)*satpole[1] + (zo-zs)*satpole[2];
            const double dist = sqrt(gsl_pow_2(xo - xe) + gsl_pow_2(yo - ye) + gsl_pow_2(zo - ze));
            const double earthDot = (xo - xe) * satpole[0] + (yo - ye) * satpole[1] + (zo - ze) * satpole[2];
            const double B = fabs(M_PI / 2 - acos(earthDot / dist));
            const double delta_u = theta_deg;
            *mag += 0.0044 * fabs(delta_u) - 2.60 * sin(B) + 1.25 * gsl_pow_2(sin(B)); // Meeus pp 285
        } else {
            // For all other objects, we use the absolute magnitude of Sun (4.83) plus distance modulus
            *mag = 4.83 + 5 * log10(Dso * Deo / Ro / sqrt(albedo) / sqrt(*phase) / 2062650.);
        }
    }

        // Routine for estimating the magnitudes of asteroids and comets
    else if (body_id >= 1e7) {
        orbitalElements *item;
        int fail = 0;
        albedo = GSL_NAN;
        Ro = GSL_NAN;

        if (body_id < 2e7) {
            // Asteroid

            // Open asteroid database
            orbitalElements_asteroids_init();

            // Fetch asteroid's record
            item = orbitalElements_asteroids_fetch(body_id - 10000000);
            if (item == NULL) fail = 1;
        } else {
            // Comet

            // Open comet database
            orbitalElements_comets_init();

            // Fetch comet's record
            item = orbitalElements_comets_fetch(body_id - 20000000);
            if (item == NULL) fail = 1;
        }

        if (!fail) {
            // Absolute magnitude of asteroid is expressed at 1 AU
            *mag = item->absoluteMag + 5 * log10(Deo) +
                   2.5 * item->slopeParam_n * log10(Dso);

            if (item->slopeParam_G > -100) {
                const double G = item->slopeParam_G; // See page 231 of Meeus
                const double phi_1 = exp(-3.33 * pow(tan(theta / 2), 0.63));
                const double phi_2 = exp(-1.87 * pow(tan(theta / 2), 1.22));
                *mag -= 2.5 * log10((1 - G) * phi_1 + G * phi_2);
            } else {
                *mag -= 2.5 * log10(*phase); // Apply geometric phase correction
            }
        } else {
            // Comet or asteroid has an illegal body_id
            albedo = Ro = *mag = GSL_NAN;
        }
    } else {
        // For other bodies, we don't know how to calculate a magnitude
        albedo = Ro = *mag = GSL_NAN;
    }


    // Popular the angular size, physical size, etc, of this object
    *angSize = 2 * atan(Ro / Deo) / M_PI * 180 * 3600; // angular size, in arcseconds
    *phySize = 2 * (Ro * GSL_CONST_MKSA_ASTRONOMICAL_UNIT); // diameter, metres
    *albedoOut = albedo;
    *sunDist = Dso;
    *earthDist = Deo;
    *sunAngDist = angDist_ABC(xo, yo, zo, xe, ye, ze, xs, ys, zs); // Angle <Object - Earth - Sun>
    *theta_eso = angDist_ABC(xe, ye, ze, xs, ys, zs, xo, yo, zo); // Angle <Earth - Sun - Object>

    // The Sun is a special case...
    if (body_id == 10) {

        *phase = 1;
        *mag = 4.83 + 5 * log10(Deo / 2062650.);
    }

    // The Earth is also a special case...
    if ((body_id == 2) || (body_id == 19)) {
        *phase = 1;
        *angSize = 180 * 3600;
        *mag = GSL_NAN;
    }

    // Compute RA and Dec from J2000.0 coordinates
    {
        const double x2 = xo - xe; // Position of object relative to the geocentre, in J2000.0 coordinates
        const double y2 = yo - ye;
        const double z2 = zo - ze;
        *ra = atan2(y2, x2);
        *dec = asin(z2 / sqrt(gsl_pow_2(x2) + gsl_pow_2(y2) + gsl_pow_2(z2)));
        // Clamp RA within range 0 to 2pi radians
        if (*ra < 0) *ra += 2 * M_PI;
    }

    // Compute ecliptic distance from J2000.0 coordinates
    {
        const double xo2 = xo - xe; // Position of object relative to the geocentre
        const double yo2 = yo - ye;
        const double zo2 = zo - ze;
        const double xs2 = xs - xe; // Position of Sun relative to the geocentre
        const double ys2 = ys - ye;
        const double zs2 = zs - ze;

        const double epsilon = (23. + 26. / 60. + 21.448 / 3600.) / 180. * M_PI; // Meeus (22.2)
        // negative x-axis points to the vernal equinox;
        // (y,z) get tipped up by 23.5 degrees from (RA,Dec) to equatorial coordinates
        const double xo3 = xo2;
        const double yo3 = cos(epsilon) * yo2 + sin(epsilon) * zo2;
        const double zo3 = -sin(epsilon) * yo2 + cos(epsilon) * zo2;

        // negative x-axis points to the vernal equinox;
        // (y,z) get tipped up by 23.5 degrees from (RA,Dec) to equatorial coordinates
        const double xs3 = xs2;
        const double ys3 = cos(epsilon) * ys2 + sin(epsilon) * zs2;
        //const double zs3     = -sin(epsilon)*ys2 + cos(epsilon)*zs2;

        *eclipticLongitude = atan2(yo3, xo3);
        *eclipticLatitude = asin(zo3 / sqrt(gsl_pow_2(xo3) + gsl_pow_2(yo3) + gsl_pow_2(zo3)));
        *eclipticDistance = atan2(yo3, xo3) - atan2(ys3, xs3);

        // Make sure that distance between Sun and object along the ecliptic is in the range +/- pi
        while (*eclipticDistance < -M_PI) *eclipticDistance += 2 * M_PI;
        while (*eclipticDistance > M_PI) *eclipticDistance -= 2 * M_PI;

        if (*eclipticDistance < 0) (*theta_eso) *= -1;
    }
}

