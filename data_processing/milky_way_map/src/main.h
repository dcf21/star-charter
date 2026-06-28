// stars.h
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

#ifndef STARS_H
#define STARS_H 1

#include <stdlib.h>
#include <stdio.h>

// Output 1 is for starcharts_svg
#define SMOOTH        16 /* Median smoothing of Axel Mellinger's panorama */
#define RESOLUTION_1  (360./8000*(SMOOTH)/16)    /* resolution of grid in degrees */

#define H_SIZE_MAP1   8000
#define V_SIZE_MAP1   4000

#define RESOLUTION_2  (360./4096)                /* resolution of grid in degrees */
#define H_SIZE_MAP2   4096
#define V_SIZE_MAP2   1137

typedef struct {
    unsigned char C[V_SIZE_MAP1][H_SIZE_MAP1];  /* 4000 x 8000 = 32 MB */
} dataArray;

#endif

