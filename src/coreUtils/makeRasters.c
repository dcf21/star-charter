// makeRasters.c
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
#include <stdio.h>
#include <math.h>

//! raster_linear - Populate the double array <out> with a linear raster of values
//! \param [out] out The array of doubles to populate
//! \param [in] start Start populating the raster from out[start]
//! \param [in] end The final value in the raster should be written to out[end-1]
//! \param [in] startval The value of the linear raster at out[start]
//! \param [in] endval The value of the linear raster at out[end-1]

void raster_linear(double *out, int start, int end, double startval, double endval) {
    int i = 0;
    int n = end - start;
    for (i = start; i < end; i++) out[i] = startval + (endval - startval) * (i - start) / n;
}

//! raster_log - Populate the double array <out> with a log raster of values
//! \param [out] out The array of doubles to populate
//! \param [in] start Start populating the raster from out[start]
//! \param [in] end The final value in the raster should be written to out[end-1]
//! \param [in] startval The value of the log raster at out[start]
//! \param [in] endval The value of the log raster at out[end-1]

void raster_log(double *out, int start, int end, double startval, double endval) {
    int i = 0;
    int n = end - start;
    for (i = start; i < end; i++) out[i] = startval * pow(endval / startval, ((double) (i - start)) / n);
}
