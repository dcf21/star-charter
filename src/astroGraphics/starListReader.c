// stars.c
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
#include <string.h>
#include <ctype.h>
#include <math.h>

#include <gsl/gsl_math.h>

#include "astroGraphics/starListReader.h"
#include "coreUtils/asciiDouble.h"
#include "coreUtils/errorReport.h"
#include "mathsTools/projection.h"
#include "settings/chart_config.h"

// Length of buffer used for manipulating object names
#define BUFLEN 1020

// Binary file format version number
const int binary_format_version = 2;

// Filenames
const char *ascii_star_catalogue = SRCDIR "../data/stars/starCataloguesMerge/output/star_charter_stars.dat.gz";
const char *binary_star_catalogue = SRCDIR "../data/stars/starCataloguesMerge/output/star_charter_stars.bin";

// Define tiling pattern
tiling_level_definition object_tilings[] = {{6.5,  1,  1},
                                            {8.5,  5,  10},
                                            {10.2, 10, 20},
                                            {12,   20, 40},
                                            {13,   40, 80},
                                            {14,   80, 160},
                                            {-1,   -1, -1}
};


//! open_binary_star_catalogue - Open the binary catalogue listing all the stars (for reading)
//! \return - File handle
FILE *open_binary_star_catalogue() {
    // Open the binary catalogue listing all the stars
    FILE *file = fopen(binary_star_catalogue, "r");

    // If binary file was opened, check the version number is correct
    if (file != NULL) {
        int version_number;
        fread(&version_number, sizeof(int), 1, file);
        if (version_number != binary_format_version) {
            fclose(file);
            file = NULL;
        }
    }

    // If binary file did not open successfully, recreate it from the ASCII catalogue
    if (file == NULL) {
        star_list_to_binary();
        file = fopen(binary_star_catalogue, "r");
        if (file == NULL) stch_fatal(__FILE__, __LINE__, "Could not open binary star catalogue for reading");
    }

    // Return to the beginning of the file
    fseek(file, 0, SEEK_SET);
    return file;
}

//! read_binary_star_catalogue_headers - Read the data contained in the header of the binary star catalogue
//! \param file - File handle to read the header from
//! \return - A <tiling_information> structure
tiling_information read_binary_star_catalogue_headers(FILE *file) {
    // Read from the beginning of the file
    fseek(file, 0, SEEK_SET);

    // Read the header information from the binary catalogue
    tiling_information tiles;
    fread(&tiles.binary_version, sizeof(int), 1, file);
    fread(&tiles.file_stars_start_position, sizeof(unsigned long int), 1, file);
    fread(&tiles.total_level_count, sizeof(int), 1, file);
    fread(&tiles.total_tile_count, sizeof(int), 1, file);

    // Allocate storage for data structures
    tiles.tile_level_start_index = malloc(tiles.total_level_count * sizeof(int));
    tiles.tile_info = malloc(tiles.total_tile_count * sizeof(star_tile_info));

    // Read data structure describing where to find individual tiles in this data file
    fread(tiles.tile_level_start_index, tiles.total_level_count * sizeof(int), 1, file);
    fread(tiles.tile_info, tiles.total_tile_count * sizeof(star_tile_info), 1, file);
    return tiles;
}

//! write_binary_star_catalogue_headers - Write the data contained in the header of the binary star catalogue
//! \param tiles - A <tiling_information> structure to write out
//! \param out - File handle to write the headers to
void write_binary_star_catalogue_headers(const tiling_information *tiles, FILE *out) {
    // Write to the beginning of the file
    fseek(out, 0, SEEK_SET);

    // Write header information to binary file
    fwrite(&tiles->binary_version, sizeof(int), 1, out);
    fwrite(&tiles->file_stars_start_position, sizeof(unsigned long int), 1, out);
    fwrite(&tiles->total_level_count, sizeof(int), 1, out);
    fwrite(&tiles->total_tile_count, sizeof(int), 1, out);
    fwrite(tiles->tile_level_start_index, tiles->total_level_count * sizeof(int), 1, out);
    fwrite(tiles->tile_info, tiles->total_tile_count * sizeof(star_tile_info), 1, out);
}

//! Free up the storage used by a <tiling_information> structure
//! \param tiles - A <tiling_information> structure to free
void free_binary_star_catalogue_headers(tiling_information *tiles) {
    if (tiles->tile_level_start_index != NULL) {
        free(tiles->tile_level_start_index);
        tiles->tile_level_start_index = NULL;
    }
    if (tiles->tile_info != NULL) {
        free(tiles->tile_info);
        tiles->tile_info = NULL;
    }
}

//! test_if_tile_in_field_of_view - Test if a particular tile is visible within the field of view of a chart
//! \param s - The chart we are determining field of view for
//! \param level - Tiling level index
//! \param ra_index - The RA tile index within tiling level <level>
//! \param dec_index - The Dec tile index within tiling level <level>
//! \return - Boolean flag indicating whether we need to plot stars from within this tile
int test_if_tile_in_field_of_view(chart_config *s, int level, int ra_index, int dec_index) {
    // Does this tile's sky area fall within field of view?
    const double ra_min = ra_index * (2 * M_PI) / object_tilings[level].ra_bins;
    const double ra_max = ra_min + (2 * M_PI) / object_tilings[level].ra_bins;
    const double dec_min = (dec_index / (double) object_tilings[level].dec_bins - 0.5) * M_PI;
    const double dec_max = dec_min + M_PI / object_tilings[level].dec_bins;

    // Does centre of field of view fall within this tile?
    if ((s->ra0 >= ra_min) && (s->ra0 <= ra_max) && (s->dec0 >= dec_min) && (s->dec0 <= dec_max)) return 1;

    // Do any of the corners of the field of view fall within this tile?
    const int steps = 1;
    for (int i = 0; i <= steps; i++)
        for (int j = 0; j <= steps; j++) {
            double ra, dec;
            const double x = (i - 0.5) * s->wlin;
            const double y = (j - 0.5) * s->aspect * s->wlin;
            inv_plane_project(&ra, &dec, s, x, y);

            // Does this corner fall within this tile?
            if ((ra >= ra_min) && (ra <= ra_max) && (dec >= dec_min) && (dec <= dec_max)) return 1;
        }

    // Do any of the corners of this tile fall within the field of view?
    for (int i = 0; i <= steps; i++)
        for (int j = 0; j <= steps; j++) {
            // Work out where tile-corner appears on chart
            double x, y;
            const double ra = (ra_min * (steps - i) + ra_max * i) / steps;
            const double dec = (dec_min * (steps - i) + dec_max * i) / steps;
            plane_project(&x, &y, s, ra, dec, 0);

            // Include this tile if this corner falls inside the plot area
            if (gsl_finite(x) && gsl_finite(y) &&
                (x >= s->x_min) && (x <= s->x_max) &&
                (y >= s->y_min) && (y <= s->y_max)) {
                return 1;
            }
        }

    // This tile is not within the field of view
    return 0;
}

//! copy_name - Copy a string into a static character buffer where we can edit it.
//! \param in - Input string
//! \return - Output copy of string
static char *copy_name(const char *in) {
    int i;
    static char buf[BUFLEN + 4];
    for (i = 0; ((i < BUFLEN) && ((in[i] < '\0') || (in[i] > ' '))); i++) buf[i] = in[i];
    buf[i] = '\0';
    return buf;
}

//! read_star_definition_from_ascii - Extract information about a star from a line of text
//! \param line - The ASCII entry for the star
//! \return - A star_definition structure
static star_definition read_star_definition_from_ascii(const char *line) {
    const char *scan = line;
    star_definition sd;
    while ((*scan > '\0') && (*scan <= ' ')) scan++;
    if (scan[0] == '\0') {
        // Blank line; return null output
        sd.ra = GSL_NAN;
        return sd;
    }
    memset((void *) &sd, 0, sizeof(star_definition)); // Ensures md5 checksum is the same on each run
    sd.hd_num = (int) get_float(scan, NULL);
    scan = next_word(scan);
    sd.ybsn_num = (int) get_float(scan, NULL);
    scan = next_word(scan);
    sd.hip_num = (int) get_float(scan, NULL);
    scan = next_word(scan);
    sd.ra = get_float(scan, NULL) / 180. * M_PI;
    scan = next_word(scan);
    sd.dec = get_float(scan, NULL) / 180. * M_PI;
    scan = next_word(scan);
    sd.mag = get_float(scan, NULL);
    scan = next_word(scan);
    sd.parallax = get_float(scan, NULL);
    scan = next_word(scan);
    sd.distance = get_float(scan, NULL);
    scan = next_word(scan);
    strcpy(sd.name1, copy_name(scan)); // Bayer letter
    scan = next_word(scan);
    strcpy(sd.name2, copy_name(scan)); // Full Bayer designation
    scan = next_word(scan);
    strcpy(sd.name3, copy_name(scan)); // Name of star
    scan = next_word(scan);
    strcpy(sd.name4, copy_name(scan)); // Catalogue designation of star
    scan = next_word(scan);
    strcpy(sd.name5, copy_name(scan)); // Flamsteed number

    return sd;
}

//! calculate_bin_number - Calculate which tile this star should be put into
//! \param sd - The star_definition for this star
//! \param tile_level_start_index - The starting position for the tiles within each level of the hierarchy
//! \return - Integer offset within <tile_info> array
static int calculate_bin_number(const star_definition sd, const int *tile_level_start_index) {
    for (int level = 0; object_tilings[level].ra_bins > 0; level++)
        if (sd.mag <= object_tilings[level].faintest_mag) {
            const int ra_bin = floor((sd.ra / (2 * M_PI)) * object_tilings[level].ra_bins);
            const int dec_bin = floor(((sd.dec / M_PI) + 0.5) * object_tilings[level].dec_bins);
            const int bin_index = dec_bin * object_tilings[level].ra_bins + ra_bin;
            return tile_level_start_index[level] + bin_index;
        }

    // Star was too faint to fit within any tiling level
    return -1;
}

//! star_list_to_binary - Take the text-based list of stars in <star_charter_stars.dat> and turn it into a binary dump
//! in <star_charter_stars.bin>. This means we can read it much faster next time.
void star_list_to_binary() {
    // Structure to hold information about the tiling hierarchy
    tiling_information tiles;
    tiles.binary_version = binary_format_version;
    tiles.file_stars_start_position = 0;
    tiles.total_level_count = 0;
    tiles.total_tile_count = 0;
    tiles.tile_level_start_index = malloc(64 * sizeof(int));

    // Count total number of tiles
    int highest_level_number = 0;
    for (int level = 0; object_tilings[level].ra_bins > 0; level++) {
        tiles.tile_level_start_index[level] = tiles.total_tile_count;
        tiles.total_tile_count += object_tilings[level].ra_bins * object_tilings[level].dec_bins;
        highest_level_number = level;
    }
    tiles.total_level_count = highest_level_number + 1;

    // Create structure for holding number of stars in each tile
    tiles.tile_info = malloc(tiles.total_tile_count * sizeof(star_tile_info));
    for (int i = 0; i < tiles.total_tile_count; i++) {
        tiles.tile_info[i].file_position = 0;
        tiles.tile_info[i].star_count = 0;
    }

    // Cycle through input data file and count how many stars fall into each bin
    char cmd[FNAME_LENGTH];
    sprintf(cmd, "zcat %s", ascii_star_catalogue);

    // Open pipe using zcat to read the ascii star catalogue
    {
        FILE *in = popen(cmd, "r");
        if (!in) {
            stch_fatal(__FILE__, __LINE__, "Could not open ASCII star catalogue");
        }

        // Loop over the lines of the text-based input star catalogue
        while ((!feof(in)) && (!ferror(in))) {
            char line[FNAME_LENGTH];
            file_readline(in, line);

            // Read star's information from ASCII text
            star_definition sd = read_star_definition_from_ascii(line);
            if (!gsl_finite(sd.ra)) continue;

            // Work out which bin this star should go into
            int bin_number = calculate_bin_number(sd, tiles.tile_level_start_index);

            // Add this star to the tally
            if (bin_number >= 0) {
                tiles.tile_info[bin_number].star_count++;
            }
        }

        // Close input ASCII file
        fclose(in);
    }

    // Now work out the position within the file where each tile will get written
    int total_star_count = 0;
    for (int i = 0; i < tiles.total_tile_count; i++) {
        tiles.tile_info[i].file_position = total_star_count;
        total_star_count += tiles.tile_info[i].star_count;
    }

    // Start writing binary output file
    FILE *out = fopen(binary_star_catalogue, "wb");
    if (out == NULL) stch_fatal(__FILE__, __LINE__, "Could not open binary star catalogue for output");

    // Write header information to binary file
    write_binary_star_catalogue_headers(&tiles, out);

    // Note position within the file where we start writing star descriptors
    tiles.file_stars_start_position = ftell(out);

    // Rewrite header information to binary file, now that start position is set
    write_binary_star_catalogue_headers(&tiles, out);

    // Keep track of how many stars we have written to each tile
    int *stars_written = malloc(tiles.total_tile_count * sizeof(int));
    for (int i = 0; i < tiles.total_tile_count; i++) stars_written[i] = 0;

    // Open pipe using zcat to read the ascii star catalogue
    {
        FILE *in = popen(cmd, "r");
        if (!in) {
            stch_fatal(__FILE__, __LINE__, "Could not open ASCII star catalogue");
        }

        // Loop over the lines of the text-based input star catalogue
        while ((!feof(in)) && (!ferror(in))) {
            char line[FNAME_LENGTH];
            file_readline(in, line);

            // Read star's information from ASCII text
            star_definition sd = read_star_definition_from_ascii(line);
            if (!gsl_finite(sd.ra)) continue;

            // Work out which bin this star should go into
            int bin_number = calculate_bin_number(sd, tiles.tile_level_start_index);

            // Reject this star if no bin was found
            if (bin_number < 0) continue;

            // Work out where, in the file, this star belongs
            unsigned int offset_within_file = tiles.tile_info[bin_number].file_position + stars_written[bin_number];
            unsigned long int file_position =
                    tiles.file_stars_start_position + offset_within_file * sizeof(star_definition);

            // Write to correct position in file
            fseek(out, (long) file_position, SEEK_SET);

            // Write this star into the binary output file
            fwrite(&sd, sizeof(star_definition), 1, out);

            // Keep track of stars written into each tile
            stars_written[bin_number]++;
        }

        // Close input ASCII file
        fclose(in);
    }

    // Close output binary data file
    fclose(out);

    // Free up the temporary arrays we used
    free_binary_star_catalogue_headers(&tiles);
    free(stars_written);
}
