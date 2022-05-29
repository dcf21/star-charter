// stars.h
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

#ifndef STAR_LIST_READER_H
#define STAR_LIST_READER_H 1

#include <stdlib.h>
#include <stdio.h>

#include "settings/chart_config.h"
#include "vectorGraphics/lineDraw.h"
#include "vectorGraphics/cairo_page.h"

//! tiling_level_definition - A structure to define a level within the tiling scheme
typedef struct {
    double faintest_mag; // The faintest magnitude of stars within this level of the hierarchy
    int ra_bins; // The number of cells into which the RA axis is divided
    int dec_bins; // The number of cells into which the Dec axis is divided
} tiling_level_definition;

//! star_tile_info - A structure to hold information about where a star_tile is to be found in the output file
typedef struct {
    // The number of stars in this tile
    int star_count;

    // The location in the binary file where the star descriptors in this tile start
    // Stored in units of sizeof(star_definition) beyond the start point of <file_stars_start_position> bytes
    int file_position;
} star_tile_info;

//! tiling_information - Information stored in the header of a binary file containing a tiled star catalogue
typedef struct {
    int binary_version; // The version number of the code which produce this binary file
    int total_level_count; // The number of levels of tiling hierarchy in the tiling scheme
    int total_tile_count; // The total number of tiles in all levels of the tiling hierarchy
    unsigned long int file_stars_start_position; // The position within file where we start writing star descriptors
    int *tile_level_start_index; // In the array <tile_info>, at what index do tiles in level x begin?
    star_tile_info *tile_info; // Information about every tile, in every level of the tiling hierarchy
} tiling_information;

//! star_definition - A structure to represent all of the data that describes a star
typedef struct {
    char name1[10]; // Bayer letter
    char name2[24]; // Full Bayer designation
    char name3[32]; // Name of star
    char name4[24]; // Catalogue designation, e.g. V337_Car
    char name5[6]; // Flamsteed number
    int hd_num, hip_num, ybsn_num; // Catalogue numbers for this star; 0 for null
    double ra; // radians, J2000.0
    double dec; // radians, J2000.0
    double mag;
    double parallax, distance;
} star_definition;

extern const char *ascii_star_catalogue; // The filename for the ascii star catalogue
extern const char *binary_star_catalogue; // The filename for the binary compressed and tiled star catalogue

extern tiling_level_definition object_tilings[];

FILE *open_binary_star_catalogue();
tiling_information read_binary_star_catalogue_headers(FILE *file);
void write_binary_star_catalogue_headers(const tiling_information *tiles, FILE *out);
void free_binary_star_catalogue_headers(tiling_information *tiles);

int test_if_tile_in_field_of_view(chart_config *s, int level, int ra_index, int dec_index);

void star_list_to_binary();

#endif
