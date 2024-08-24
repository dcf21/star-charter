// jpl.h
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

#ifndef JPL_H
#define JPL_H 1

void jpl_computeXYZ(int body_id, double jd, double *x, double *y, double *z);

void jpl_computeEphemeris(int bodyId, double jd, double *x, double *y, double *z, double *ra, double *dec,
                          double *mag, double *phase, double *angSize, double *phySize, double *albedo, double *sunDist,
                          double *earthDist, double *sunAngDist, double *theta_ESO, double *eclipticLongitude,
                          double *eclipticLatitude, double *eclipticDistance);

#endif

