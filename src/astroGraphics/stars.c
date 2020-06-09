// stars.c
// 
// -------------------------------------------------
// Copyright 2015-2020 Dominic Ford
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

#include "astroGraphics/stars.h"
#include "coreUtils/asciiDouble.h"
#include "coreUtils/errorReport.h"
#include "mathsTools/projection.h"
#include "settings/chart_config.h"
#include "vectorGraphics/lineDraw.h"
#include "vectorGraphics/cairo_page.h"

#define BUFLEN 1020

//! star_definition - A structure to represent all of the data that describes a star

typedef struct {
    char name1[32]; // Bayer letter
    char name2[32]; // Full Bayer designation
    char name3[32]; // Name of star
    char name4[32]; // Flamsteed number
    int hd_num, hip_num, ybsn_num;
    double ra, dec, mag, para, dist;
} star_definition;

//! strcmp_ascii - Compare two strings, on the basis of ASCII characters only, ignoring UTF8 characters
//! \param in1 - First string
//! \param in2 - Second string
//! \return - strcmp-like comparison of in1 and in2

static int strcmp_ascii(const char *in1, const char *in2) {
    char buffer1[256], buffer2[256];
    int j1 = 0, j2 = 0;
    for (int i = 0; in1[i] != '\0' && i < 255; i++) if (isalpha(in1[i])) buffer1[j1++] = in1[i];
    for (int i = 0; in2[i] != '\0' && i < 255; i++) if (isalpha(in2[i])) buffer2[j2++] = in2[i];
    buffer1[j1] = buffer2[j2] = '\0';
    return strcmp(buffer1, buffer2);
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

//! star_list_to_binary - Take the text-based list of stars in <star_charter_stars.dat> and turn it into a binary dump
//! in <star_charter_stars.bin>. This means we can read it much faster next time.

void star_list_to_binary() {
    FILE *in, *out;
    long int count = 0;
    char line[FNAME_LENGTH], *scan;
    star_definition sd;

    in = fopen(SRCDIR "../data/stars/starCataloguesMerge/output/star_charter_stars.dat", "r");
    if (in == NULL) stch_fatal(__FILE__, __LINE__, "Could not open Tycho ASCII catalogue");

    out = fopen(SRCDIR "../data/stars/starCataloguesMerge/output/star_charter_stars.bin", "w");
    if (out == NULL) stch_fatal(__FILE__, __LINE__, "Could not open Tycho binary catalogue");
    fwrite(&count, sizeof(long int), 1, out);

    // Loop over the lines of the text-based input star catalogue
    while ((!feof(in)) && (!ferror(in))) {
        file_readline(in, line);
        scan = line;
        while ((*scan > '\0') && (*scan <= ' ')) scan++;
        if (scan[0] == '\0') continue; // Blank line
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
        sd.para = get_float(scan, NULL);
        scan = next_word(scan);
        sd.dist = get_float(scan, NULL);
        scan = next_word(scan);
        strcpy(sd.name1, copy_name(scan)); // Bayer letter
        scan = next_word(scan);
        strcpy(sd.name2, copy_name(scan)); // Full Bayer designation
        scan = next_word(scan);
        strcpy(sd.name3, copy_name(scan)); // Name of star
        scan = next_word(scan);
        strcpy(sd.name4, copy_name(scan)); // Flamsteed number

        // Write this star into the binary output file
        fwrite(&sd, sizeof(star_definition), 1, out);
        count++;
    }

    // The first entry in the binary output is the number of stars in the file
    if (fseek(out, 0, SEEK_SET)) stch_fatal(__FILE__, __LINE__, "Could not fseek Tycho binary catalogue");
    fwrite(&count, sizeof(long int), 1, out);

    fclose(in);
    fclose(out);
}

//! Maximum number of <mag_step> intervals allowed between <mag_max> and <mag_min>

#define LEN_NSTARS 63

//! tweak_mag_limits - Tweak the values of <mag_max> and <mag_min>, defining the magnitude limits of the stars we
//! plot, to ensure that (a) there are no <mag_step> intervals at the bright end with no stars in them, and (b) that
//! at the faint end we have no more than <> stars.
//! \param s - A <chart_config> structure defining the properties of the star chart to be drawn.

void tweak_magnitude_limits(chart_config *s) {
    FILE *file;
    long int N, j, k, l;

    // A histogram of the number of stars in each <mag_step> interval
    int Nstars[LEN_NSTARS + 1];

    // Open the binary catalogue listing all the stars
    file = fopen(SRCDIR "../data/stars/starCataloguesMerge/output/star_charter_stars.bin", "r");
    if (file == NULL) {
        star_list_to_binary();
        file = fopen(SRCDIR "../data/stars/starCataloguesMerge/output/star_charter_stars.bin", "r");
        if (file == NULL) stch_fatal(__FILE__, __LINE__, "Could not open Tycho catalogue");
    }

    // Read the number of stars in the binary catalogue
    fread(&N, sizeof(long int), 1, file);

    // Zero the histogram we're going to create of the magnitudes of the stars
    for (j = 0; j <= LEN_NSTARS; j++) Nstars[j] = 0;

    // Work out how many histogram bins lie between <mag_max> and <mag_min>
    l = (int) floor((s->mag_max - s->mag_min) / s->mag_step);

    // Loop over each star in turn
    for (j = 0; j < N; j++) {
        star_definition sd;
        double mag2, x, y;

        // Read the star from disk
        fread(&sd, sizeof(star_definition), 1, file);

        // Work out which histogram bin this star falls into
        mag2 = floor((s->mag_max - sd.mag) / s->mag_step);

        // If the star is too faint, ignore it (and stop, since catalogue is in descending order of brightness)
        if (mag2 < l) break;

        // If the star is brighter than <mag_max>, pretend it has magnitude <mag_max>
        if (mag2 > 0) mag2 = 0;

        // Work out where star appears on chart
        plane_project(&x, &y, s, sd.ra, sd.dec, 0);

        // Ignore this star if it falls outside the plot area
        if ((!gsl_finite(x)) || (!gsl_finite(y)) || (x < s->x_min) || (x > s->x_max) || (y < s->y_min) ||
            (y > s->y_max))
            continue;

        // Add this star to the histogram
        Nstars[LEN_NSTARS + (int) mag2]++;
    }

    // Loop over the histogram bins
    double new_mag_max = s->mag_max;

    for (j = k = 0; j <= -l; j++) {
        // The maximum brightness of stars in this histogram bin
        double m = s->mag_max + j * s->mag_step;

        // k counts how many stars we've seen so far
        k += Nstars[LEN_NSTARS - j];

        // If we've not seen any stars, then reduce the upper brightness limit
        if (k == 0) {
            new_mag_max = m + s->mag_step;
        }

        // print debugging message
        if (DEBUG) {
            snprintf(temp_err_string, FNAME_LENGTH, "Number of stars brighter than mag %6.2f = %6ld", m, k);
            stch_log(temp_err_string);
        }

        // If we've exceeded the maximum allowable number of stars, then truncate at magnitude <m>
        if (k > s->maximum_star_count) {
            s->mag_min = m;
            if (DEBUG) {
                snprintf(temp_err_string, FNAME_LENGTH, "Truncating stars to mag %6.2f", s->mag_min);
                stch_log(temp_err_string);
            }
            break;
        }
    }

    // Update the upper brightness limit
    s->mag_max = new_mag_max;

    // Close the binary file listing all the stars in the sky
    fclose(file);
}

//! get_star_size - Calculate the radius of this star, in canvas coordinates
//! \param s - A <chart_config> structure defining the properties of the star chart to be drawn.
//! \param mag - The magnitude of the star whose radius we are to calculate.
//! \return The radius of the circle we should draw on the page

double get_star_size(const chart_config *s, double mag) {
    // Normalise the star's brightness into a number of <mag_step> intervals fainter than <mag_max>
    double mag2 = (s->mag_max - mag) / s->mag_step;

    // Truncate size of stars at magMax. But this can make very bright stars look much too faint
    // if ( mag2 > 0) mag2 = 0;

    // Physical radius of this star on the page, logarithmically scaled as a function of brightness
    mag2 = s->mag_size_norm * 46.6 * pow(s->mag_alpha, floor(mag2));

    const double pt = 1. / 72; // 1 pt
    const double size = 0.75 * 3 * pt * mag2 * 0.0014552083 * 60;
    return size;
}

//! plot_stars - Plot stars onto the star chart
//! \param s - A <chart_config> structure defining the properties of the star chart to be drawn.
//! \param page - A <cairo_page> structure defining the cairo drawing context.

void plot_stars(chart_config *s, cairo_page *page) {
    FILE *file;
    long int n, j;

    // Open the binary catalogue listing all the stars
    file = fopen(SRCDIR "../data/stars/starCataloguesMerge/output/star_charter_stars.bin", "r");
    if (file == NULL) {
        star_list_to_binary();
        file = fopen(SRCDIR "../data/stars/starCataloguesMerge/output/star_charter_stars.bin", "r");
        if (file == NULL) stch_fatal(__FILE__, __LINE__, "Could not open Tycho catalogue");
    }

    // Read the number of stars in the binary catalogue
    fread(&n, sizeof(long int), 1, file);

    // Count the number of stars we have labelled, and make sure it doesn't exceed <s->maximum_star_label_count>
    int label_counter = 0;

    // Loop over the stars in the binary file
    for (j = 0; j < n; j++) {
        star_definition sd;
        double x, y;

        // Read the star's <star_definition> structure
        fread(&sd, sizeof(star_definition), 1, file);

        // Stars are sorted in order of brightness, so can stop immediately once we find one that is too faint
        if (sd.mag > s->mag_min) break;

        // Work out coordinates of this star on the star chart
        plane_project(&x, &y, s, sd.ra, sd.dec, 0);

        // Ignore this star if it falls outside the plot area
        if ((!gsl_finite(x)) || (!gsl_finite(y)) || (x < s->x_min) || (x > s->x_max) || (y < s->y_min) ||
            (y > s->y_max))
            continue;

        // Keep track of the brightest star in the field
        if (sd.mag < s->mag_highest) s->mag_highest = sd.mag;

        // Calculate the radius of this star on tha canvas
        const double size = get_star_size(s, sd.mag);

        // Draw a circular splodge on the star chart
        double x_canvas, y_canvas;
        fetch_canvas_coordinates(&x_canvas, &y_canvas, x, y, s);
        cairo_set_source_rgb(s->cairo_draw, s->star_col.red, s->star_col.grn, s->star_col.blu);
        cairo_new_path(s->cairo_draw);
        cairo_arc(s->cairo_draw, x_canvas, y_canvas, size * s->dpi, 0, 2 * M_PI);
        cairo_fill(s->cairo_draw);

        // Consider whether to write a text label nest to this star
        if ((sd.mag < s->star_label_mag_min) && (label_counter < s->maximum_star_label_count)) {

            // Do we show an English name for this star?
            const int show_name3 = (s->star_names) && (sd.name3[0] != '\0') && (sd.name3[0] != '-') &&
                                   (strcmp_ascii(sd.name3, sd.name2) != 0);

            // Do we show a Bayer designation for this star?
            const int show_name1 = (s->star_bayer_labels) && (sd.name1[0] != '\0') && (sd.name1[0] != '-');

            // Do we show a Flamsteed number for this star?
            const int show_name4 = (s->star_flamsteed_labels) && (sd.name4[0] != '\0') && (sd.name4[0] != '-');

            // Do we show a catalogue number for this star
            const int show_cat = s->star_catalogue_numbers &&
                                 ((s->star_catalogue == SW_CAT_HIP) || (s->star_catalogue == SW_CAT_YBSC) ||
                                  (s->star_catalogue == SW_CAT_HD));

            // Does this star have multiple text labels associated with it?
            const int multiple_labels = (show_name3 + show_name1 + show_name4 + show_cat + s->star_mag_labels) > 1;

            // How far should we move this label to the side of the star, to avoid writing text on top of the star?
            double horizontal_offset = size * s->dpi + 0.05 * s->cm;

            // Write an English name next to this star
            if (show_name3) {
                strcpy(temp_err_string, sd.name3);

                // Replace underscores with spaces
                for (int k = 0; temp_err_string[k] > '\0'; k++)
                    if (temp_err_string[k] == '_') temp_err_string[k] = ' ';

                chart_label_buffer(page, s, s->star_label_col, temp_err_string,
                                   (label_position[2]) {{x, y, horizontal_offset,  -1, 0},
                                                        {x, y, -horizontal_offset, 1,  0}}, 2,
                                   multiple_labels, 0, 1.2, 0, 0, 0, sd.mag);
                label_counter++;
            }

            // Write a Bayer designation next to this star
            if (show_name1) {
                chart_label_buffer(page, s, s->star_label_col, sd.name1,
                                   (label_position[2]) {{x, y, horizontal_offset,  -1, 0},
                                                        {x, y, -horizontal_offset, 1,  0}}, 2,
                                   multiple_labels, 0, 1.2, 0, 0, 0, sd.mag);
                label_counter++;
            }

            // Write a Flamsteed number next to this star
            if (show_name4) {
                chart_label_buffer(page, s, s->star_label_col, sd.name4,
                                   (label_position[2]) {{x, y, horizontal_offset,  -1, 0},
                                                        {x, y, -horizontal_offset, 1,  0}}, 2,
                                   multiple_labels, 0, 1.2, 0, 0, 0, sd.mag);
                label_counter++;
            }

            // Write a catalogue number next to this star
            if (show_cat) {
                if (s->star_catalogue == SW_CAT_HIP) {
                    // Write a Hipparcos number
                    snprintf(temp_err_string, FNAME_LENGTH, "HIP%d", sd.hip_num);
                    chart_label_buffer(page, s, s->star_label_col, temp_err_string,
                                       (label_position[2]) {{x, y, horizontal_offset,  -1, 0},
                                                            {x, y, -horizontal_offset, 1,  0}}, 2,
                                       multiple_labels, 0, 1.2, 0, 0, 0, sd.mag);
                    label_counter++;
                } else if (s->star_catalogue == SW_CAT_YBSC) {
                    // Write an HR number (i.e. Yale Bright Star Catalog number)
                    snprintf(temp_err_string, FNAME_LENGTH, "HR%d", sd.ybsn_num);
                    chart_label_buffer(page, s, s->star_label_col, temp_err_string,
                                       (label_position[2]) {{x, y, horizontal_offset,  -1, 0},
                                                            {x, y, -horizontal_offset, 1,  0}}, 2,
                                       multiple_labels, 0, 1.2, 0, 0, 0, sd.mag);
                    label_counter++;
                } else if (s->star_catalogue == SW_CAT_HD) {
                    // Write a Henry Draper number
                    snprintf(temp_err_string, FNAME_LENGTH, "HD%d", sd.hd_num);
                    chart_label_buffer(page, s, s->star_label_col, temp_err_string,
                                       (label_position[2]) {{x, y, horizontal_offset,  -1, 0},
                                                            {x, y, -horizontal_offset, 1,  0}}, 2,
                                       multiple_labels, 0, 1.2, 0, 0, 0, sd.mag);
                    label_counter++;
                }
            }

            // Write the magnitude of this star next to it
            if (s->star_mag_labels) {
                snprintf(temp_err_string, FNAME_LENGTH, "mag %.1f", sd.mag);
                chart_label_buffer(page, s, s->star_label_col, temp_err_string,
                                   (label_position[2]) {{x, y, horizontal_offset,  -1, 0},
                                                        {x, y, -horizontal_offset, 1,  0}}, 2,
                                   multiple_labels, 0, 1.2, 0, 0, 0, sd.mag - 0.000001);
                label_counter++;
            }
        }
    }

    // Close the binary file listing all the stars
    fclose(file);
}

//! draw_magnitude_key - Draw a legend underneath the star chart showing the mapping between sizes of splodge and
//! the magnitude of the star.
//! \param s - A <chart_config> structure defining the properties of the star chart to be drawn.

void draw_magnitude_key(chart_config *s) {
    int i;

    // The width of the text saying "Magnitude scale:"
    const double w_tag = 3.8 * s->font_size;

    // The width of each item in the magnitude key
    const double w_item = 1.5 * s->font_size;

    // The number of items in the magnitude key
    int N = (int) floor((s->mag_min - s->mag_highest) / s->mag_step);

    // The number of columns we can fit in the magnitude key, spanning the full width of the star chart
    int Ncol = (int) floor((s->width - w_tag) / w_item);
    if (Ncol < 1) Ncol = 1;
    if (Ncol > N + 1) Ncol = N + 1;

    // Work out how many rows we need
    const int Nrow = (int) ceil((N + 1.0) / Ncol);

    // Positions of the corners of the magnitude key on the canvas
    const double y0 = s->canvas_offset_y * 2 + s->width * s->aspect + 0.8 + s->copyright_gap;
    const double y1 = y0 - 0.4;
    const double x1 = s->canvas_offset_x + s->width / 2;
    const double xw = w_tag + w_item * Ncol;

    double x = x1 - xw / 2, x_left;
    char line[FNAME_LENGTH];
    cairo_text_extents_t extents;

    s->magnitude_key_rows = Nrow;

    // Reset font weight
    cairo_select_font_face(s->cairo_draw, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);

    // Write the heading next to the magnitude key
    const char *heading = "Magnitude scale:";

    cairo_set_source_rgb(s->cairo_draw, 0, 0, 0);
    cairo_set_font_size(s->cairo_draw, 3.6 * s->mm * s->font_size);
    cairo_text_extents(s->cairo_draw, heading, &extents);
    cairo_move_to(s->cairo_draw,
                  (x + w_tag / 2) * s->cm - extents.width / 2 - extents.x_bearing,
                  y1 * s->cm - extents.height / 2 - extents.y_bearing
    );
    cairo_show_text(s->cairo_draw, heading);

    x += w_tag;

    x_left = x;

    // Loop over each magnitude bin in turn
    for (i = 0; i <= N; i++) {
        const double magnitude = s->mag_min - i * s->mag_step;

        // Calculate the radius of this star on tha canvas
        const double size = get_star_size(s, magnitude);

        const double x_pos = x_left + (i % Ncol) * w_item;
        const double y_pos = y1 + floor(i / Ncol) * 0.8;
        //if (size > (y0-y1)) continue;

        // Draw a splodge representing a star of a particular magnitude
        cairo_new_path(s->cairo_draw);
        cairo_arc(s->cairo_draw, x_pos * s->cm, y_pos * s->cm, size * s->dpi, 0, 2 * M_PI);
        cairo_fill(s->cairo_draw);

        // Write the magnitude value next to it
        snprintf(line, 1024, "%.1f", magnitude);
        cairo_text_extents(s->cairo_draw, line, &extents);
        cairo_move_to(s->cairo_draw,
                      (x_pos + 0.1) * s->cm + size * 1.25 * s->dpi - extents.x_bearing,
                      y_pos * s->cm - extents.height / 2 - extents.y_bearing
        );
        cairo_show_text(s->cairo_draw, line);
    }
}
