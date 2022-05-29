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
#include "astroGraphics/stars.h"
#include "coreUtils/errorReport.h"
#include "mathsTools/projection.h"
#include "settings/chart_config.h"
#include "vectorGraphics/cairo_page.h"


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

//! Maximum number of <mag_step> intervals allowed between <mag_max> and <mag_min>
#define STAR_HISTOGRAM_MAX_LEN (128)

//! Absolute limits on magnitudes of stars in the input dataset
const double CATALOGUE_MAG_MAX = -2.0;
const double CATALOGUE_MAG_MIN = 14.0;

//! tweak_mag_limits - Tweak the values of <mag_max> and <mag_min>, defining the magnitude limits of the stars we
//! plot, to ensure that (a) there are no <mag_step> intervals at the bright end with no stars in them, and (b) that
//! at the faint end we have no more than <s->maximum_star_count> stars.
//! \param s - A <chart_config> structure defining the properties of the star chart to be drawn.
void tweak_magnitude_limits(chart_config *s) {
    // First check that magnitude limits are within valid range
    if (s->mag_min < CATALOGUE_MAG_MAX) s->mag_min = CATALOGUE_MAG_MAX;
    if (s->mag_max < CATALOGUE_MAG_MAX) s->mag_max = CATALOGUE_MAG_MAX;
    if (s->mag_min > CATALOGUE_MAG_MIN) s->mag_min = CATALOGUE_MAG_MIN;
    if (s->mag_max > CATALOGUE_MAG_MIN) s->mag_max = CATALOGUE_MAG_MIN;

    // Start reading the binary star catalogue
    FILE *file = open_binary_star_catalogue();

    // Read the header information from the binary catalogue
    tiling_information tiles = read_binary_star_catalogue_headers(file);

    // A histogram of the number of stars in each <mag_step> interval
    int star_histogram[STAR_HISTOGRAM_MAX_LEN + 1];

    // Zero the histogram we're going to create of the magnitudes of the stars
    for (int j = 0; j <= STAR_HISTOGRAM_MAX_LEN; j++) star_histogram[j] = 0;

    // Work out how many histogram bins lie between <mag_max> and <mag_min>
    int histogram_bins = (int) floor((CATALOGUE_MAG_MIN - CATALOGUE_MAG_MAX) / s->mag_step);

    // Keep track of how many stars we have put into the histogram
    int included_stars = 0;

    // Loop over each tiling level
    for (int level = 0;
         (
                 (level < tiles.total_level_count) && // Do not exceed deepest tiling level
                 ((level == 0) ||
                  (object_tilings[level - 1].faintest_mag < s->mag_min) ||
                  (included_stars < s->minimum_star_count + 10)
                 ) && // Tiling level too faint?
                 (included_stars < s->maximum_star_count + 10) // Already got too many stars?
         );
         level++
            ) {
        // Loop over Dec tiles
        for (int dec_index = 0; dec_index < object_tilings[level].dec_bins; dec_index++)
            // Loop over RA tiles
            for (int ra_index = 0; ra_index < object_tilings[level].ra_bins; ra_index++) {
                // Does this tile's sky area fall within field of view?
                if (!test_if_tile_in_field_of_view(s, level, ra_index, dec_index)) continue;

                // Work out position of this tile in the binary file
                const int tile_index_in_level = dec_index * object_tilings[level].ra_bins + ra_index;
                const int tile_index_in_array = tiles.tile_level_start_index[level] + tile_index_in_level;
                const star_tile_info *tile = &tiles.tile_info[tile_index_in_array];
                const unsigned long int tile_file_pos = (tiles.file_stars_start_position +
                                                         tile->file_position * sizeof(star_definition));

                // Seek to correct position in the binary file
                fseek(file, (long) tile_file_pos, SEEK_SET);

                // Loop over each star in turn
                for (int star_index = 0; star_index < tile->star_count; star_index++) {
                    // Read the star from disk
                    star_definition sd;
                    fread(&sd, sizeof(star_definition), 1, file);

                    // Work out which histogram bin this star falls into
                    int mag_bin_index = (int) floor((sd.mag - CATALOGUE_MAG_MAX) / s->mag_step);

                    // If the star is brighter than <mag_max>, pretend it has magnitude <mag_max> to avoid over-running array
                    if (mag_bin_index < 0) mag_bin_index = 0;

                    // Work out where star appears on chart
                    double x, y;
                    plane_project(&x, &y, s, sd.ra, sd.dec, 0);

                    // Ignore this star if it falls outside the plot area
                    if ((!gsl_finite(x)) || (!gsl_finite(y)) || (x < s->x_min) || (x > s->x_max) || (y < s->y_min) ||
                        (y > s->y_max))
                        continue;

                    // Add this star to the histogram
                    star_histogram[mag_bin_index]++;
                    included_stars++;
                }
            }
    }

    // Close the binary file listing all the stars in the sky
    fclose(file);

    // Free up tiling hierarchy information
    free_binary_star_catalogue_headers(&tiles);

    // Loop over the histogram bins, counting the total number of stars
    double new_mag_max = CATALOGUE_MAG_MAX;
    int star_total_count = 0;

    for (int bin_index = 0; bin_index <= histogram_bins; bin_index++) {
        // The maximum brightness of stars in this histogram bin
        double bin_mag_brightest = CATALOGUE_MAG_MAX + bin_index * s->mag_step;

        // star_total_count counts how many stars we've seen so far
        star_total_count += star_histogram[bin_index];

        // If we've not seen fewer than four stars, then reduce the upper brightness used to scale the size of stars
        if (star_total_count < 4) {
            new_mag_max = bin_mag_brightest + s->mag_step;
        }

        // print debugging message
        if (DEBUG) {
            snprintf(temp_err_string, FNAME_LENGTH, "Number of stars brighter than mag %6.2f = %6d", bin_mag_brightest,
                     star_total_count);
            stch_log(temp_err_string);
        }

        // If we've not yet had a minimum allowable number of stars, then include fainter stars
        if ((star_total_count < s->minimum_star_count) && (bin_mag_brightest > s->mag_min)) {
            s->mag_min = bin_mag_brightest;
        }

        // If we've exceeded the maximum allowable number of stars, then truncate at magnitude <bin_mag_brightest>
        if ((star_total_count > s->maximum_star_count) && (bin_mag_brightest < s->mag_min)) {
            s->mag_min = bin_mag_brightest;
            if (DEBUG) {
                snprintf(temp_err_string, FNAME_LENGTH, "Truncating stars to mag %6.2f", s->mag_min);
                stch_log(temp_err_string);
            }
            break;
        }
    }

    // Update the upper brightness limit
    s->mag_max = gsl_max(s->mag_max, new_mag_max);
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
    mag2 = s->mag_size_norm * 46.6 * pow(s->mag_alpha, mag2);

    const double pt = 1. / 72; // 1 pt
    const double size = 0.75 * 3 * pt * mag2 * 0.0014552083 * 60;
    return size;
}

//! plot_stars - Plot stars onto the star chart
//! \param s - A <chart_config> structure defining the properties of the star chart to be drawn.
//! \param page - A <cairo_page> structure defining the cairo drawing context.

void plot_stars(chart_config *s, cairo_page *page) {
    // Start reading the binary star catalogue
    FILE *file = open_binary_star_catalogue();

    // Read the header information from the binary catalogue
    tiling_information tiles = read_binary_star_catalogue_headers(file);

    // Count the number of stars we have labelled, and make sure it doesn't exceed <s->maximum_star_label_count>
    int label_counter = 0;
    int star_counter = 0;

    // Loop over each tiling level
    for (int level = 0;
         (
                 (level < tiles.total_level_count) && // Do not exceed deepest tiling level
                 ((level == 0) || (object_tilings[level - 1].faintest_mag < s->mag_min)) // Tiling level too faint?
         );
         level++
            ) {
        // Loop over Dec tiles
        for (int dec_index = 0; dec_index < object_tilings[level].dec_bins; dec_index++)
            // Loop over RA tiles
            for (int ra_index = 0; ra_index < object_tilings[level].ra_bins; ra_index++) {
                // Does this tile's sky area fall within field of view?
                if (!test_if_tile_in_field_of_view(s, level, ra_index, dec_index)) continue;

                // Work out position of this tile in the binary file
                const int tile_index_in_level = dec_index * object_tilings[level].ra_bins + ra_index;
                const int tile_index_in_array = tiles.tile_level_start_index[level] + tile_index_in_level;
                const star_tile_info *tile = &tiles.tile_info[tile_index_in_array];
                const unsigned long int tile_file_pos = (tiles.file_stars_start_position +
                                                         tile->file_position * sizeof(star_definition));

                // Seek to correct position in the binary file
                fseek(file, (long) tile_file_pos, SEEK_SET);

                // Loop over each star in turn
                for (int star_index = 0; star_index < tile->star_count; star_index++) {
                    // Read the star from disk
                    star_definition sd;
                    fread(&sd, sizeof(star_definition), 1, file);

                    // Stars are sorted in order of brightness, so can stop processing this tile if we find one that is too faint
                    if (sd.mag > s->mag_min) break;

                    // Work out coordinates of this star on the star chart
                    double x, y;
                    plane_project(&x, &y, s, sd.ra, sd.dec, 0);

                    // Ignore this star if it falls outside the plot area
                    if ((!gsl_finite(x)) || (!gsl_finite(y)) ||
                        (x < s->x_min) || (x > s->x_max) || (y < s->y_min) || (y > s->y_max)) {
                        continue;
                    }

                    // Count number of stars
                    star_counter++;

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

                    // Don't allow text labels to be placed over this star
                    {
                        double x_exclusion_region, y_exclusion_region;
                        fetch_graph_coordinates(x_canvas, y_canvas, &x_exclusion_region, &y_exclusion_region, s);
                        chart_add_label_exclusion(page, s,
                                                  x_exclusion_region, x_exclusion_region,
                                                  y_exclusion_region, y_exclusion_region);
                    }

                    // Consider whether to write a text label nest to this star
                    if ((sd.mag < s->star_label_mag_min) && (label_counter < s->maximum_star_label_count)) {

                        // Do we show an English name for this star?
                        const int show_name3 = s->star_names && (sd.name3[0] != '\0') && (sd.name3[0] != '-') &&
                                               (strcmp_ascii(sd.name3, sd.name2) != 0);

                        // Do we show a variable-star designation for this star?
                        const int show_name4 = s->star_variable_labels && (sd.name4[0] != '\0') && (sd.name4[0] != '-');

                        // Do we show a Bayer designation for this star?
                        const int show_name1 = s->star_bayer_labels && (sd.name1[0] != '\0') && (sd.name1[0] != '-');

                        // Do we show a Flamsteed number for this star?
                        const int show_name5 =
                                s->star_flamsteed_labels && (sd.name5[0] != '\0') && (sd.name5[0] != '-');

                        // Do we show a catalogue number for this star
                        const int show_cat = s->star_catalogue_numbers &&
                                             ((s->star_catalogue == SW_CAT_HIP) || (s->star_catalogue == SW_CAT_YBSC) ||
                                              (s->star_catalogue == SW_CAT_HD));

                        // Does this star have multiple text labels associated with it?
                        const int star_label_count =
                                show_name3 + show_name1 + show_name4 + show_name5 + show_cat + s->star_mag_labels;
                        const int multiple_labels = (star_label_count > 1) && s->star_allow_multiple_labels;

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
                                               multiple_labels, 0, 1.2 * s->label_font_size_scaling,
                                               0, 0, 0, sd.mag);
                            label_counter++;
                            if (!s->star_allow_multiple_labels) continue;
                        }

                        // Write a Bayer designation next to this star
                        if (show_name1) {
                            chart_label_buffer(page, s, s->star_label_col, sd.name1,
                                               (label_position[2]) {{x, y, horizontal_offset,  -1, 0},
                                                                    {x, y, -horizontal_offset, 1,  0}}, 2,
                                               multiple_labels, 0, 1.2 * s->label_font_size_scaling,
                                               0, 0, 0, sd.mag);
                            label_counter++;
                            if (!s->star_allow_multiple_labels) continue;
                        }

                        // Write a Flamsteed number next to this star
                        if (show_name5) {
                            chart_label_buffer(page, s, s->star_label_col, sd.name5,
                                               (label_position[2]) {{x, y, horizontal_offset,  -1, 0},
                                                                    {x, y, -horizontal_offset, 1,  0}}, 2,
                                               multiple_labels, 0, 1.2 * s->label_font_size_scaling,
                                               0, 0, 0, sd.mag);
                            label_counter++;
                            if (!s->star_allow_multiple_labels) continue;
                        }

                        // Write variable star designation next to this star
                        if (show_name4) {
                            strcpy(temp_err_string, sd.name4);

                            // Replace underscores with spaces
                            for (int k = 0; temp_err_string[k] > '\0'; k++)
                                if (temp_err_string[k] == '_') temp_err_string[k] = ' ';

                            chart_label_buffer(page, s, s->star_label_col, temp_err_string,
                                               (label_position[2]) {{x, y, horizontal_offset,  -1, 0},
                                                                    {x, y, -horizontal_offset, 1,  0}}, 2,
                                               multiple_labels, 0, 1.2 * s->label_font_size_scaling,
                                               0, 0, 0, sd.mag);
                            label_counter++;
                            if (!s->star_allow_multiple_labels) continue;
                        }

                        // Write a catalogue number next to this star
                        if (show_cat) {
                            if ((s->star_catalogue == SW_CAT_HIP) && (sd.hip_num > 0)) {
                                // Write a Hipparcos number
                                snprintf(temp_err_string, FNAME_LENGTH, "HIP%d", sd.hip_num);
                                chart_label_buffer(page, s, s->star_label_col, temp_err_string,
                                                   (label_position[2]) {{x, y, horizontal_offset,  -1, 0},
                                                                        {x, y, -horizontal_offset, 1,  0}}, 2,
                                                   multiple_labels, 0, 1.2 * s->label_font_size_scaling,
                                                   0, 0, 0, sd.mag);
                            } else if ((s->star_catalogue == SW_CAT_YBSC) && (sd.ybsn_num > 0)) {
                                // Write an HR number (i.e. Yale Bright Star Catalog number)
                                snprintf(temp_err_string, FNAME_LENGTH, "HR%d", sd.ybsn_num);
                                chart_label_buffer(page, s, s->star_label_col, temp_err_string,
                                                   (label_position[2]) {{x, y, horizontal_offset,  -1, 0},
                                                                        {x, y, -horizontal_offset, 1,  0}}, 2,
                                                   multiple_labels, 0, 1.2 * s->label_font_size_scaling,
                                                   0, 0, 0, sd.mag);
                            } else if ((s->star_catalogue == SW_CAT_HD) && (sd.hd_num > 0)) {
                                // Write a Henry Draper number
                                snprintf(temp_err_string, FNAME_LENGTH, "HD%d", sd.hd_num);
                                chart_label_buffer(page, s, s->star_label_col, temp_err_string,
                                                   (label_position[2]) {{x, y, horizontal_offset,  -1, 0},
                                                                        {x, y, -horizontal_offset, 1,  0}}, 2,
                                                   multiple_labels, 0, 1.2 * s->label_font_size_scaling,
                                                   0, 0, 0, sd.mag);
                            }
                            label_counter++;
                            if (!s->star_allow_multiple_labels) continue;
                        }

                        // Write the magnitude of this star next to it
                        if (s->star_mag_labels) {
                            snprintf(temp_err_string, FNAME_LENGTH, "mag %.1f", sd.mag);
                            chart_label_buffer(page, s, s->star_label_col, temp_err_string,
                                               (label_position[2]) {{x, y, horizontal_offset,  -1, 0},
                                                                    {x, y, -horizontal_offset, 1,  0}}, 2,
                                               multiple_labels, 0, 1.2 * s->label_font_size_scaling,
                                               0, 0, 0, sd.mag - 0.000001);
                            label_counter++;
                            if (!s->star_allow_multiple_labels) continue;
                        }
                    }
                }
            }
    }

    // Close the binary file listing all the stars
    fclose(file);

    // Free up tiling hierarchy information
    free_binary_star_catalogue_headers(&tiles);

    // print debugging message
    if (DEBUG) {
        snprintf(temp_err_string, FNAME_LENGTH, "Displayed %d stars and %d star labels", star_counter, label_counter);
        stch_log(temp_err_string);
    }
}

//! draw_magnitude_key - Draw a legend underneath the star chart showing the mapping between sizes of splodge and
//! the magnitude of the star.
//! \param s - A <chart_config> structure defining the properties of the star chart to be drawn.
//! \param legend_y_pos - The vertical pixel position of the top of the next legend to go under the star chart.

double draw_magnitude_key(chart_config *s, double legend_y_pos) {

    // The width of the text saying "Magnitude scale:"
    const double w_tag = 3.8 * s->font_size;

    // The width of each item in the magnitude key
    const double w_item = 1.5 * s->font_size;

    // The number of items in the magnitude key
    const int n_items = (int) ceil((s->mag_min - s->mag_highest) / s->mag_step);

    // The number of columns we can fit in the magnitude key, spanning the full width of the star chart
    int n_columns = (int) floor((s->width - s->legend_right_column_width - w_tag) / w_item);
    if (n_columns < 1) n_columns = 1;
    if (n_columns > n_items + 1) n_columns = n_items + 1;

    // Work out how many rows we need
    const int n_rows = (int) ceil((n_items + 1.0) / n_columns);

    // Positions of the corners of the magnitude key on the canvas
    const double y0 = legend_y_pos;
    const double y1 = y0 - 0.4;
    const double x1 = s->canvas_offset_x + (s->width - s->legend_right_column_width) / 2;
    const double xw = w_tag + w_item * n_columns;

    double x = x1 - xw / 2, x_left;
    char line[FNAME_LENGTH];
    cairo_text_extents_t extents;

    s->magnitude_key_rows = n_rows;

    // Reset font weight
    cairo_select_font_face(s->cairo_draw, s->font_family, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);

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
    for (int i = 0; i <= n_items; i++) {
        const double magnitude = s->mag_min - i * s->mag_step;

        // Calculate the radius of this star on tha canvas
        const double size = get_star_size(s, magnitude);

        const double x_pos = x_left + (i % n_columns) * w_item;
        const double y_pos = y1 + floor(i / n_columns) * 0.8;
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

    const double new_bottom_to_legend_items = y0 + 0.2 + 0.8 * s->magnitude_key_rows;
    return new_bottom_to_legend_items;
}
