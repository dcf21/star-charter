// cairo_page.c
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
#include <math.h>

#include <cairo/cairo.h>
#include <cairo/cairo-svg.h>
#include <cairo/cairo-ps.h>
#include <cairo/cairo-pdf.h>

#include <gsl/gsl_math.h>

#include "coreUtils/asciiDouble.h"
#include "coreUtils/errorReport.h"

#include "listTools/ltList.h"
#include "listTools/ltMemory.h"

#include "astroGraphics/ephemeris.h"

#include "settings/chart_config.h"

#include "vectorGraphics/cairo_page.h"

//! One quarter-turn in radians
#define DEG90  (90.*M_PI/180.)

//! One half-turn in radians
#define DEG180  (180.*M_PI/180.)

//! One three-quarter-turn in radians
#define DEG270  (270.*M_PI/180.)


//! string_make_permanent - Copy a string into a new malloced buffer
//! \param in - The input string to copy
//! \return - A pointer to a new malloced buffer containing a copy of the input string

char *string_make_permanent(const char *in) {
    int len;
    char *out;
    if (in == NULL) return NULL;
    len = (int) strlen(in);
    out = lt_malloc(len + 2);
    strcpy(out, in);
    return out;
}

//! cairo_init - Initialise a cairo drawing surface to render a star chart onto
//! \param p - A structure describing the status of the drawing surface
//! \param s - Settings for the star chart we are to draw

void cairo_init(cairo_page *p, chart_config *s) {

    // Work out what graphics format we are producing from the extension of <s->output_filename>
    const int filename_len = (int) strlen(s->output_filename);
    const int filename_extension_start = (int) gsl_max(filename_len - 3, 0);

    if (str_cmp_no_case(s->output_filename + filename_extension_start, "svg") == 0) {
        s->output_format = SW_FORMAT_SVG;
    } else if (str_cmp_no_case(s->output_filename + filename_extension_start, "png") == 0) {
        s->output_format = SW_FORMAT_PNG;
    } else if (str_cmp_no_case(s->output_filename + filename_extension_start, "eps") == 0) {
        s->output_format = SW_FORMAT_EPS;
    } else if (str_cmp_no_case(s->output_filename + filename_extension_start, "pdf") == 0) {
        s->output_format = SW_FORMAT_PDF;
    } else {
        stch_fatal(__FILE__, __LINE__, "Could not determine output format from file extension.");
    }

    // Some useful units of size / width
    const double png_dpi = 100;
    const double vector_dpi = 72;
    s->dpi = (s->output_format == SW_FORMAT_PNG) ? png_dpi : vector_dpi;  // pixels / inch
    s->pt = s->dpi / 72;  // pixels / pt
    s->cm = 0.393701 * s->dpi;  // pixels / cm
    s->mm = s->cm * 0.1;  // pixels / mm
    s->line_width_base = 0.5 * s->pt; // standard line width

    // Work out the bounding box of the canvas to draw the star chart onto
    const int have_title = strcmp(s->title, "") != 0;
    s->canvas_offset_x = 1.6;
    s->canvas_offset_y = have_title ? 1.4 : 0.7;
    s->canvas_width = (s->width + 2 * s->canvas_offset_x) * s->cm;
    s->canvas_height = (s->width * s->aspect + s->canvas_offset_y + 0.7 + (s->ra_dec_lines ? 0.5 : 0)) * s->cm;

    // Add space to the bounding box for legend items which go beneath the star chart
    double legend_y_pos_left = 0;
    double legend_y_pos_right = 0;
    double legend_right_width = 0;

    // If we're showing a table of the object's magnitude, measure that now
    if (s->ephemeris_table) {
        double ephemeris_table_height = draw_ephemeris_table(s, 0, 0, &legend_right_width);
        legend_y_pos_right += (ephemeris_table_height - 1) * s->cm;
    }

    // Calculate height of the magnitude key
    if (s->magnitude_key) {
        // Work out how many rows of magnitude key there will be
        const double w_tag = 3.8 * s->font_size;
        const double w_item = 1.5 * s->font_size;

        // The number of items in the magnitude key
        const int n_items = (int) floor((s->mag_min - s->mag_highest) / s->mag_step);

        // The number of columns we can fit in the magnitude key, spanning the full width of the star chart
        int n_columns = (int) floor((s->width - legend_right_width - w_tag) / w_item);
        if (n_columns < 1) n_columns = 1;
        if (n_columns > n_items + 1) n_columns = n_items + 1;

        // Work out how many rows we need
        s->magnitude_key_rows = (int) ceil((n_items + 1.0) / n_columns);

        // And how much vertical height they will take up
        legend_y_pos_left += (s->magnitude_key_rows * 0.8) * s->cm;
    }

    if (s->great_circle_key) legend_y_pos_left += 1.0 * s->cm;
    if (s->dso_symbol_key) legend_y_pos_left += 1.0 * s->cm;

    // Calculate full height of the drawing canvas
    s->canvas_height += gsl_max(legend_y_pos_left, legend_y_pos_right);
    s->legend_right_column_width = legend_right_width;

    // Create cairo drawing surface of the appropriate graphics type
    switch (s->output_format) {
        case SW_FORMAT_SVG:
            s->cairo_surface = cairo_svg_surface_create(s->output_filename, s->canvas_width, s->canvas_height);
            break;
        case SW_FORMAT_PNG:
            s->cairo_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
                                                          (int) s->canvas_width,
                                                          (int) s->canvas_height);
            break;
        case SW_FORMAT_EPS:
            s->cairo_surface = cairo_ps_surface_create(s->output_filename, s->canvas_width, s->canvas_height);
            cairo_ps_surface_set_eps(s->cairo_surface, 1);
            break;
        case SW_FORMAT_PDF:
            s->cairo_surface = cairo_pdf_surface_create(s->output_filename, s->canvas_width, s->canvas_height);
            break;
    }

    // Check that surface was created
    int cairo_status = cairo_surface_status(s->cairo_surface);
    if (cairo_status != 0) {
        snprintf(temp_err_string, 4096, "Could not create output file. Error was: %s.",
                 cairo_status_to_string(cairo_status));
        stch_fatal(__FILE__, __LINE__, temp_err_string);
        exit(1);
    }

    // Initialise empty lists of labels we will put on the edges of the star chart
    p->x_labels = listInit();
    p->x2_labels = listInit();
    p->y_labels = listInit();
    p->y2_labels = listInit();

    // Create a buffer into which we write all the text labels we are to write
    // We buffer them in order that we can plot them in order of priority, not the order in which they are added to
    // the buffer. We remove any labels which cannot be printed without overlapping with other labels
    p->labels_buffer = (label_buffer_item *) lt_malloc(MAX_LABELS * sizeof(label_buffer_item));
    p->labels_buffer_counter = 0;

    // A list of the rectangular outlines of all the labels we've already written, such that no future labels are
    // allowed in these regions.
    p->exclusion_regions = (exclusion_region *) lt_malloc(MAX_EXCLUSION_REGIONS * sizeof(exclusion_region));
    p->exclusion_region_counter = 0;

    // Create a cairo drawing context
    s->cairo_draw = cairo_create(s->cairo_surface);
    cairo_set_source_rgb(s->cairo_draw, 0, 0, 0);

    // While we're drawing the star chart, clip graphics operations to the plot area. This prevents text labels
    // spilling over the edges.
    cairo_save(s->cairo_draw);

    if ((s->projection == SW_PROJECTION_SPH) || (s->projection == SW_PROJECTION_ALTAZ)) {
        cairo_arc(s->cairo_draw,
                  (s->canvas_offset_x + s->width / 2) * s->cm,
                  (s->canvas_offset_y + s->width / 2 * s->aspect) * s->cm,
                  s->width * s->cm / s->wlin,
                  0, 2 * M_PI);

    } else {
        cairo_rectangle(s->cairo_draw,
                        s->canvas_offset_x * s->cm, s->canvas_offset_y * s->cm,
                        s->width * s->cm, s->width * s->aspect * s->cm);
    }

    cairo_clip(s->cairo_draw);
}

//! plot_background_image - Render a PNG image in the background behind a star chart
//! \param s - Settings for the star chart we are drawing

void plot_background_image(chart_config *s) {
    // If the filename of the image is empty, render no image
    if (s->photo_filename[0] == '\0') return;

    // Load background image
    cairo_surface_t *surface = cairo_image_surface_create_from_png(s->photo_filename);

    // Read the pixel dimensions of the image we are to paint
    const int width = cairo_image_surface_get_width(surface);
    const int height = cairo_image_surface_get_width(surface);

    // We need to do some coordinate transformations in order to display the image at the right scale...
    cairo_save(s->cairo_draw);

    // Move our coordinate system so the top-left of the star chart is at (0,0), and the star chart's dimensions
    // match those of the PNG image we're about to paint behind it
    cairo_translate(s->cairo_draw, s->canvas_offset_x * s->cm, s->canvas_offset_y * s->cm);
    cairo_scale(s->cairo_draw, (s->width * s->cm) / width, (s->width * s->aspect * s->cm) / height);

    // Paint PNG image onto destination canvas, where it fills up a space of dimensions width x height
    cairo_set_source_surface(s->cairo_draw, surface, 0, 0);
    cairo_rectangle(s->cairo_draw, 0, 0, width, height);
    cairo_fill(s->cairo_draw);

    // Undo the coordinate transformations
    cairo_restore(s->cairo_draw);
    cairo_set_source_rgb(s->cairo_draw, 0, 0, 0);

    // Destroy surface we created
    cairo_surface_finish(surface);
}

//! draw_chart_edging - Draw the lines and labels around the edge of the star chart. First, we stop clipping the
//! drawing context to the star chart area.
//! \param p - A structure describing the status of the drawing surface
//! \param s - Settings for the star chart we are drawing

void draw_chart_edging(cairo_page *p, chart_config *s) {
    cairo_text_extents_t extents;

    // Stop clipping to the plot area
    cairo_restore(s->cairo_draw);

    // Select a font
    cairo_select_font_face(s->cairo_draw, s->font_family, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);

    // Draw title at top
    cairo_set_font_size(s->cairo_draw, 4.5 * s->mm * s->font_size);
    cairo_text_extents(s->cairo_draw, s->title, &extents);
    cairo_move_to(s->cairo_draw,
                  s->canvas_width / 2 - (extents.width / 2 + extents.x_bearing),
                  s->canvas_offset_y * s->cm * 0.32 - (extents.height / 2 + extents.y_bearing));
    cairo_show_text(s->cairo_draw, s->title);

    // Draw outline of chart
    cairo_set_source_rgb(s->cairo_draw, 0, 0, 0);
    cairo_set_line_width(s->cairo_draw, s->chart_edge_line_width * s->line_width_base);
    cairo_new_path(s->cairo_draw);
    if ((s->projection == SW_PROJECTION_SPH) || (s->projection == SW_PROJECTION_ALTAZ)) {
        // On alt/az charts, the chart has a circular shape
        cairo_arc(s->cairo_draw,
                  (s->canvas_offset_x + s->width / 2) * s->cm,
                  (s->canvas_offset_y + s->width / 2 * s->aspect) * s->cm,
                  s->width * s->cm / s->wlin,
                  0, 2 * M_PI);
        cairo_stroke(s->cairo_draw);

        // On alt/az charts, write the cardinal points around the edge of the chart
        if ((s->projection == SW_PROJECTION_ALTAZ) && (s->cardinals)) {
            const double dh = 1.05, dv = 1.08;
            const double a = s->position_angle * M_PI / 180;
            const colour black = (colour) {0, 0, 0};
            chart_label(p, s, black, "N",
                        &(label_position) {-dh * sin(a), -dv * cos(a), 0, 0, -1}, 1,
                        0, 0, 2.5, 1, 0, 0, -1);
            chart_label(p, s, black, "E",
                        &(label_position) {-dh * sin(a + DEG90), -dv * cos(a + DEG90), 0, 0, -1}, 1,
                        0, 0, 2.5, 1, 0, 0, -1);
            chart_label(p, s, black, "S",
                        &(label_position) {-dh * sin(a + DEG180), -dv * cos(a + DEG180), 0, 0, 1}, 1,
                        0, 0, 2.5, 1, 0, 0, -1);
            chart_label(p, s, black, "W",
                        &(label_position) {-dh * sin(a + DEG270), -dv * cos(a + DEG270), 0, 0, 1}, 1,
                        0, 0, 2.5, 1, 0, 0, -1);
        }

    } else {
        // On all other projections, the chart is rectangular
        cairo_rectangle(s->cairo_draw,
                        s->canvas_offset_x * s->cm, s->canvas_offset_y * s->cm,
                        s->width * s->cm, s->width * s->aspect * s->cm);
        cairo_stroke(s->cairo_draw);

        // If requested, write "Right ascension" on the horizontal axis, and "Declination" on the vertical axis
        if (s->axis_label) {
            const char *x_label = "Right ascension";
            cairo_set_source_rgb(s->cairo_draw, 0, 0, 0);
            cairo_set_font_size(s->cairo_draw, 2 * s->mm * s->font_size);
            cairo_text_extents(s->cairo_draw, x_label, &extents);
            cairo_move_to(s->cairo_draw,
                          s->canvas_width / 2 - (extents.width / 2 + extents.x_bearing),
                          (s->canvas_offset_y * 1.6 + s->width * s->aspect) * s->cm
                          - (extents.height / 2 + extents.y_bearing)
            );
            cairo_show_text(s->cairo_draw, x_label);

            const char *y_label = "Declination";
            cairo_text_extents(s->cairo_draw, y_label, &extents);
            cairo_save(s->cairo_draw);
            cairo_translate(s->cairo_draw,
                            s->canvas_offset_x * s->cm * 0.4,
                            (s->canvas_offset_y + s->width * s->aspect) * s->cm / 2
            );
            cairo_rotate(s->cairo_draw, -DEG90);
            cairo_move_to(s->cairo_draw,
                          -(extents.width / 2 + extents.x_bearing),
                          -(extents.height / 2 + extents.y_bearing)
            );
            cairo_show_text(s->cairo_draw, y_label);
            cairo_restore(s->cairo_draw);
        }
    }
}

//! fetch_canvas_coordinates - Convert the point (x_in, y_in) in "star chart" coordinates, into physical coordinates
//! on the cairo page.
//! \param [out] x_out - The physical x coordinate (cairo coordinates)
//! \param [out] y_out - The physical y coordinate (cairo coordinates)
//! \param [in] x_in - The star chart x coordinate (star chart angular coordinates)
//! \param [in] y_in - The star chart y coordinate (star chart angular coordinates)
//! \param [in] s - Settings for the star chart we are drawing

void fetch_canvas_coordinates(double *x_out, double *y_out, double x_in, double y_in, chart_config *s) {
    *x_out = ((x_in - s->x_min) / (s->x_max - s->x_min) * s->width + s->canvas_offset_x) * s->cm;
    *y_out = ((y_in - s->y_min) / (s->y_max - s->y_min) * s->width * s->aspect + s->canvas_offset_y) * s->cm;
}

//! fetch_graph_coordinates - Convert the point (x_in, y_in) in physical coordinates, into graph coordinates
//! on the cairo page.
//! \param [in] x_in - The physical x coordinate (cairo coordinates)
//! \param [in] y_in - The physical y coordinate (cairo coordinates)
//! \param [out] x_out - The star chart x coordinate (star chart angular coordinates)
//! \param [out] y_out - The star chart y coordinate (star chart angular coordinates)
//! \param [in] s - Settings for the star chart we are drawing

void fetch_graph_coordinates(double x_in, double y_in, double *x_out, double *y_out, chart_config *s) {
    *x_out = ((x_in - s->x_min) / (s->x_max - s->x_min) * s->width + s->canvas_offset_x) * s->cm;
    *y_out = ((y_in - s->y_min) / (s->y_max - s->y_min) * s->width * s->aspect + s->canvas_offset_y) * s->cm;

    *x_out = (x_in / s->cm - s->canvas_offset_x) / s->width * (s->x_max - s->x_min) + s->x_min;
    *y_out = (y_in / s->cm - s->canvas_offset_y) / (s->width * s->aspect) * (s->y_max - s->y_min) + s->y_min;
}

//! chart_label_buffer - Schedule a label to be rendered onto the star chart next time we call <chart_label_unbuffer>.
//! \param p - A structure describing the status of the drawing surface
//! \param s - Settings for the star chart we are drawing
//! \param colour - The colour to use to write the text
//! \param label - The string of the text label
//! \param possible_positions - A list of possible positions for this label, in order of preference (star chart
//! angular coordinates)
//! \param possible_position_count - The number of positions in <possible_positions>
//! \param multiple_labels - Boolean flag indicating whether this object has multiple labels around it
//! \param make_background - Make a background behind this label
//! \param font_size - Font size of label
//! \param font_bold - Boolean indicating whether to render label in bold
//! \param font_italic - Boolean indicating whether to render label in italic
//! \param extra_margin - Expand the margin around this label by the given numerical factor
//! \param priority - The priority of this label

void chart_label_buffer(cairo_page *p, chart_config *s, colour colour, const char *label,
                        const label_position *possible_positions, int possible_position_count, int multiple_labels,
                        int make_background, double font_size, int font_bold, int font_italic, double extra_margin,
                        double priority) {
    p->labels_buffer[p->labels_buffer_counter].s = s;
    p->labels_buffer[p->labels_buffer_counter].colour = colour;
    p->labels_buffer[p->labels_buffer_counter].label = string_make_permanent(label);

    // Copy list of possible positions
    p->labels_buffer[p->labels_buffer_counter].possible_position_count = possible_position_count;

    p->labels_buffer[p->labels_buffer_counter].possible_positions = lt_malloc(
            possible_position_count * sizeof(label_position)
    );
    memcpy(p->labels_buffer[p->labels_buffer_counter].possible_positions,
           possible_positions,
           possible_position_count * sizeof(label_position)
    );

    p->labels_buffer[p->labels_buffer_counter].multiple_labels = multiple_labels;
    p->labels_buffer[p->labels_buffer_counter].make_background = make_background;
    p->labels_buffer[p->labels_buffer_counter].font_size = font_size;
    p->labels_buffer[p->labels_buffer_counter].font_bold = font_bold;
    p->labels_buffer[p->labels_buffer_counter].font_italic = font_italic;
    p->labels_buffer[p->labels_buffer_counter].extra_margin = extra_margin;
    p->labels_buffer[p->labels_buffer_counter].priority = priority;
    p->labels_buffer_counter++;

    // Check for buffer overrun
    if (p->labels_buffer_counter > MAX_LABELS) {
        stch_fatal(__FILE__, __LINE__, "Exceeded maximum number of text labels");
    }
}

//! chart_label_sorter - Sort all of the text labels which have been buffered via calls to <chart_label_buffer> into
//! order of ascending priority.
//! \param a_void - First label to compare
//! \param b_void - Second label to compare
//! \return - Comparison of the priority of the two labels

int chart_label_sorter(const void *a_void, const void *b_void) {
    const label_buffer_item *a = a_void;
    const label_buffer_item *b = b_void;
    if (a->priority > b->priority) return 1;
    if (a->priority < b->priority) return -1;
    return 0;
}

//! chart_label_unbuffer - Render all of the text labels which have been buffered via calls to <chart_label_buffer>.
//! \param p - A structure describing the status of the drawing surface

void chart_label_unbuffer(cairo_page *p) {
    // Sort labels into order of priority
    qsort(p->labels_buffer, (size_t) p->labels_buffer_counter, sizeof(label_buffer_item), &chart_label_sorter);

    // Render all buffered labels in order of priority
    for (int i = 0; i < p->labels_buffer_counter; i++) {
        const label_buffer_item *x = &p->labels_buffer[i];
        chart_label(p, x->s, x->colour, x->label, x->possible_positions, x->possible_position_count,
                    x->multiple_labels, x->make_background, x->font_size, x->font_bold, x->font_italic,
                    x->extra_margin, x->priority);
    }

    // Clear out label buffer
    p->labels_buffer_counter = 0;
    p->exclusion_region_counter = 0;
}

//! chart_add_label_exclusion - Add an exclusion region where text labels should not be placed.
//! \param p - A structure describing the status of the drawing surface
//! \param s - Settings for the star chart we are drawing
//! \param x_min - The left extent of the exclusion rectangle
//! \param x_max - The right extent of the exclusion rectangle
//! \param y_min - The top extent of the exclusion rectangle
//! \param y_max - The bottom extent of the exclusion rectangle

void chart_add_label_exclusion(cairo_page *p, chart_config *s, double x_min, double x_max, double y_min, double y_max) {
    p->exclusion_regions[p->exclusion_region_counter].x_min = x_min;
    p->exclusion_regions[p->exclusion_region_counter].x_max = x_max;
    p->exclusion_regions[p->exclusion_region_counter].y_min = y_min;
    p->exclusion_regions[p->exclusion_region_counter].y_max = y_max;

    // Debugging code to draw bounding box around text item
    if (0) {
        cairo_set_source_rgb(s->cairo_draw, 1, 0, 0);
        double a, b, c, d;
        fetch_canvas_coordinates(&a, &b, x_min, y_min, s);
        fetch_canvas_coordinates(&c, &d, x_max, y_max, s);
        cairo_move_to(s->cairo_draw, a - 1, b - 1);
        cairo_line_to(s->cairo_draw, a - 1, d + 1);
        cairo_line_to(s->cairo_draw, c + 1, d + 1);
        cairo_line_to(s->cairo_draw, c + 1, b - 1);
        cairo_close_path(s->cairo_draw);
        cairo_stroke(s->cairo_draw);
    }

    p->exclusion_region_counter++;

    // Check for buffer overrun
    if (p->exclusion_region_counter >= MAX_EXCLUSION_REGIONS) {
        stch_fatal(__FILE__, __LINE__, "Exceeded maximum label exclusion regions");
    }
}

//! chart_check_label_exclusion - Check that a new label does not collide with any previous label
//! \param p - A structure describing the status of the drawing surface
//! \param x_min - The left edge of the new label
//! \param x_max - The right edge of the new label
//! \param y_min - The top edge of the new label
//! \param y_max - The bottom edge of the new label
//! \return - Boolean indicating whether this label collides with any previous label

int chart_check_label_exclusion(const cairo_page *p, double x_min, double x_max, double y_min,
                                double y_max) {
    // Do not allow label positions which collide with ones we've already rendered
    int collision = 0;
    for (int i = 0; i < p->exclusion_region_counter; i++) {
        if (
                (x_max > p->exclusion_regions[i].x_min) &&
                (x_min < p->exclusion_regions[i].x_max) &&
                (y_max > p->exclusion_regions[i].y_min) &&
                (y_min < p->exclusion_regions[i].y_max)
                ) {
            collision = 1;
            break;
        }
    }
    return collision;
}

//! chart_label - Write a text label onto a cairo page immediately.
//! \param p - A structure describing the status of the drawing surface
//! \param s - Settings for the star chart we are drawing
//! \param colour - The colour to use to write the text
//! \param label - The string of the text label
//! \param possible_positions - A list of possible positions for this label, in order of preference (star chart
//! angular coordinates)
//! \param possible_position_count - The number of positions in <possible_positions>
//! \param multiple_labels - Boolean flag indicating whether this object has multiple labels around it
//! \param make_background - Make a background behind this label
//! \param font_size - Font size of label
//! \param font_bold - Boolean indicating whether to render label in bold
//! \param font_italic - Boolean indicating whether to render label in italic
//! \param extra_margin - Expand the margin around this label by the given numerical factor
//! \param priority - The priority of this label
//! \return - Zero on success. One if label clashed with an existing label.

int chart_label(cairo_page *p, chart_config *s, colour colour, const char *label,
                const label_position *possible_positions, int possible_position_count, int multiple_labels,
                int make_background, double font_size, int font_bold, int font_italic,
                double extra_margin, double priority) {

    // Loop over all the possible positions for this label, trying each in turn
    int position_count = 0;
    for (position_count = 0; position_count < possible_position_count; position_count++) {
        label_position pos = possible_positions[position_count];

        // Reject this position if it is not finite
        if ((!gsl_finite(pos.x)) || (!gsl_finite(pos.y)) || (!gsl_finite(pos.offset_size)))
            continue;

        // Select font
        cairo_text_extents_t extents;
        cairo_set_font_size(s->cairo_draw, 2 * s->mm * font_size * s->font_size);
        cairo_select_font_face(s->cairo_draw, s->font_family,
                               font_italic ? CAIRO_FONT_SLANT_ITALIC : CAIRO_FONT_SLANT_NORMAL,
                               font_bold ? CAIRO_FONT_WEIGHT_BOLD : CAIRO_FONT_WEIGHT_NORMAL);

        // Measure text bounding box
        cairo_text_extents(s->cairo_draw, label, &extents);

        double x_canvas, y_canvas;
        fetch_canvas_coordinates(&x_canvas, &y_canvas, pos.x, pos.y, s);

        switch (pos.h_align) {
            case 1:
                x_canvas -= extents.width + extents.x_bearing; // right align
                break;
            case 0:
                x_canvas -= extents.width / 2 + extents.x_bearing; // centre align
                break;
            case -1:
                x_canvas -= extents.x_bearing; // left align
                break;
            default:
                break;
        }

        switch (pos.v_align) {
            case 1:
                y_canvas -= extents.height + extents.y_bearing; // top align
                break;
            case 0:
                y_canvas -= extents.height / 2 + extents.y_bearing; // centre align
                break;
            case -1:
                y_canvas -= extents.y_bearing; // bottom align
                break;
            default:
                break;
        }

        // Apply offset
        x_canvas += pos.offset_size;

        // Calculate approximate bounding box for this label
        double margin = multiple_labels ? 0.01 : 0.07;
        double x_min, x_max, y_min, y_max;
        fetch_graph_coordinates(x_canvas, y_canvas, &x_min, &y_min, s);
        fetch_graph_coordinates(x_canvas + extents.width, y_canvas - extents.height, &x_max, &y_max, s);

        // Check that limits are the right way around
        if (x_max < x_min) {
            double tmp = x_max;
            x_max = x_min;
            x_min = tmp;
        }
        if (y_max < y_min) {
            double tmp = y_max;
            y_max = y_min;
            y_min = tmp;
        }

        // Work out how much space to allow around this label
        double x_margin = margin * (x_max - x_min) * (extra_margin + 1.);
        double y_margin = margin * (y_max - y_min) * 2.3 * (extra_margin + 1.);

        // Enlarge bounding box by margin
        x_min -= x_margin;
        x_max += x_margin;
        y_min -= y_margin;
        y_max += y_margin;

        // Reject label if it collides with one we've already rendered
        if (priority >= 0) {
            // Do not allow label positions which go outside edges of the plot
            if ((x_min < s->x_min) || (x_max > s->x_max) || (y_min < s->y_min) || (y_max > s->y_max)) {
                continue;
            }

            // Check that this label does not collide with any previous labels
            const int collision = chart_check_label_exclusion(p, x_min, x_max, y_min, y_max);
            if (collision) continue;
        }

        // Label position has been accepted

        // Add label to collision list
        if (priority >= -1) {
            chart_add_label_exclusion(p, s, x_min, x_max, y_min, y_max);
        }

        // If <make_background> is set, we smear the background around the label with a dark colour
        if (make_background && s->plot_galaxy_map) {
            double theta;
            const double offset = 0.2 * s->mm;
            cairo_set_source_rgb(s->cairo_draw, s->galaxy_col0.red, s->galaxy_col0.grn, s->galaxy_col0.blu);
            for (theta = 0; theta < 359; theta += 30) {
                cairo_move_to(s->cairo_draw, x_canvas + offset * sin(theta), y_canvas + offset * cos(theta));
                cairo_show_text(s->cairo_draw, label);
            }
        }

        // Render the text label itself
        cairo_set_source_rgb(s->cairo_draw, colour.red, colour.grn, colour.blu);
        cairo_move_to(s->cairo_draw, x_canvas, y_canvas);
        cairo_show_text(s->cairo_draw, label);

        return 0;
    }

    // No acceptable position was found for this label
    return 1;
}

//! chart_ticks_draw - Draw labels such as right ascensions and declinations around the edge of the star chart. Each
//! call to this function handles one edge of the star chart.
//! \param p - A structure describing the status of the drawing surface
//! \param s - Settings for the star chart we are drawing
//! \param labels - The list of labels to write along this edge of the star chart
//! \param axis - One of ("x", "y", "x2", "y2") indicating whether this is the (bottom, left, top, right) edge

void chart_ticks_draw(cairo_page *p, chart_config *s, list *labels, char *axis) {
    int N = listLen(labels);
    void *buff;
    cairo_text_extents_t extents;

    // Iterate over the labels in the list
    listIterator *listiter = listIterateInit(labels);

    // Copy the list of labels into a buffer
    buff = lt_malloc(FNAME_LENGTH * N);
    for (int j = 0; (listiter != NULL); j++, listiter = listIterate(listiter, NULL)) {
        memcpy(buff + j * FNAME_LENGTH, listiter->data, FNAME_LENGTH);
    }

    // Set the colour and font size for the labels
    cairo_set_source_rgb(s->cairo_draw, 0, 0, 0);
    cairo_set_font_size(s->cairo_draw, 3.2 * s->mm * s->font_size);

    // Loop over all the labels on this edge of the plot
    for (int j = 0; j < N; j++) {
        const char *tic_text = (char *) (buff + j * FNAME_LENGTH + sizeof(double));
        const double tic_pos = *(double *) (buff + j * FNAME_LENGTH);
        double x_canvas, y_canvas;
        cairo_text_extents(s->cairo_draw, tic_text, &extents);

        if (strcmp(axis, "x") == 0) {
            // Calculate coordinates of this label
            y_canvas = (s->canvas_offset_y + s->width * s->aspect + 0.2) * s->cm;
            x_canvas = (s->canvas_offset_x + s->width * (tic_pos - s->x_min) / (s->x_max - s->x_min)) * s->cm;

            // Check that this label does not collide with previous labels
            const double x_min = x_canvas-extents.width / 2;
            const double x_max = x_canvas+extents.width / 2;
            const double y_min = y_canvas-extents.height/2;
            const double y_max = y_canvas+extents.height/2;
            if (chart_check_label_exclusion(p, x_min, x_max, y_min, y_max)) continue;
            chart_add_label_exclusion(p, s, x_min, x_max, y_min, y_max);

            // Write a label on the bottom edge of the chart
            cairo_save(s->cairo_draw);
            cairo_translate(s->cairo_draw, x_canvas, y_canvas);
            cairo_rotate(s->cairo_draw, s->x_label_slant * M_PI / 180);
            cairo_move_to(s->cairo_draw, -extents.width / 2 - extents.x_bearing, -extents.y_bearing);
            cairo_show_text(s->cairo_draw, tic_text);
            cairo_restore(s->cairo_draw);
        } else if (strcmp(axis, "x2") == 0) {
            // Calculate coordinates of this label
            y_canvas = (s->canvas_offset_y - 0.2) * s->cm;
            x_canvas = (s->canvas_offset_x + s->width * (tic_pos - s->x_min) / (s->x_max - s->x_min)) * s->cm;

            // Check that this label does not collide with previous labels
            const double x_min = x_canvas-extents.width / 2;
            const double x_max = x_canvas+extents.width / 2;
            const double y_min = y_canvas-extents.height/2;
            const double y_max = y_canvas+extents.height/2;
            if (chart_check_label_exclusion(p, x_min, x_max, y_min, y_max)) continue;
            chart_add_label_exclusion(p, s, x_min, x_max, y_min, y_max);

            // Write a label on the top edge of the chart
            cairo_save(s->cairo_draw);
            cairo_translate(s->cairo_draw, x_canvas, y_canvas);
            cairo_rotate(s->cairo_draw, s->x_label_slant * M_PI / 180);
            cairo_move_to(s->cairo_draw, -extents.width / 2 - extents.x_bearing, -extents.height - extents.y_bearing);
            cairo_show_text(s->cairo_draw, tic_text);
            cairo_restore(s->cairo_draw);
        } else if (strcmp(axis, "y") == 0) {
            // Calculate coordinates of this label
            x_canvas = (s->canvas_offset_x - 0.2) * s->cm;
            y_canvas =
                    (s->canvas_offset_y + s->width * s->aspect * (tic_pos - s->y_min) / (s->y_max - s->y_min)) * s->cm;

            // Check that this label does not collide with previous labels
            const double x_min = x_canvas-extents.width / 2;
            const double x_max = x_canvas+extents.width / 2;
            const double y_min = y_canvas-extents.height/2;
            const double y_max = y_canvas+extents.height/2;
            if (chart_check_label_exclusion(p, x_min, x_max, y_min, y_max)) continue;
            chart_add_label_exclusion(p, s, x_min, x_max, y_min, y_max);

            // Write a label on the left edge of the chart
            cairo_save(s->cairo_draw);
            cairo_translate(s->cairo_draw, x_canvas, y_canvas);
            cairo_rotate(s->cairo_draw, s->y_label_slant * M_PI / 180);
            cairo_move_to(s->cairo_draw, -extents.width - extents.x_bearing, -extents.height / 2 - extents.y_bearing);
            cairo_show_text(s->cairo_draw, tic_text);
            cairo_restore(s->cairo_draw);
        } else if (strcmp(axis, "y2") == 0) {
            // Calculate coordinates of this label
            x_canvas = (s->canvas_offset_x + s->width + 0.2) * s->cm;
            y_canvas =
                    (s->canvas_offset_y + s->width * s->aspect * (tic_pos - s->y_min) / (s->y_max - s->y_min)) * s->cm;

            // Check that this label does not collide with previous labels
            const double x_min = x_canvas-extents.width / 2;
            const double x_max = x_canvas+extents.width / 2;
            const double y_min = y_canvas-extents.height/2;
            const double y_max = y_canvas+extents.height/2;
            if (chart_check_label_exclusion(p, x_min, x_max, y_min, y_max)) continue;
            chart_add_label_exclusion(p, s, x_min, x_max, y_min, y_max);

            // Write a label on the right edge of the chart
            cairo_save(s->cairo_draw);
            cairo_translate(s->cairo_draw, x_canvas, y_canvas);
            cairo_rotate(s->cairo_draw, s->y_label_slant * M_PI / 180);
            cairo_move_to(s->cairo_draw, -extents.x_bearing, -extents.height / 2 - extents.y_bearing);
            cairo_show_text(s->cairo_draw, tic_text);
            cairo_restore(s->cairo_draw);
        }
    }
}

//! chart_finish - Finish drawing a star chart onto a cairo drawing surface. If needed, write it to disk now.
//! \param p - A structure describing the status of the drawing surface
//! \param s - Settings for the star chart we are drawing
//! \return - Zero on success

int chart_finish(cairo_page *p, chart_config *s) {
    // Plot buffered labels in order of ascending magnitude
    chart_label_unbuffer(p);

    // Reset font weight
    cairo_select_font_face(s->cairo_draw, s->font_family, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);

    // Write copyright text
    cairo_text_extents_t extents;
    cairo_set_font_size(s->cairo_draw, 3.0 * s->mm * s->font_size);
    cairo_text_extents(s->cairo_draw, s->copyright, &extents);
    cairo_set_source_rgb(s->cairo_draw, 0, 0, 0);
    cairo_move_to(s->cairo_draw,
                  (s->canvas_offset_x * 0.5) * s->cm,
                  (s->canvas_offset_y + 0.7 + (s->ra_dec_lines ? 0.5 : 0) +
                   +s->width * s->aspect
                   + s->copyright_gap - 0.2
                   + s->copyright_gap_2) * s->cm);
    cairo_show_text(s->cairo_draw, s->copyright);

    // Write lists of ticks to put on axes
    chart_ticks_draw(p, s, p->x_labels, "x");
    chart_ticks_draw(p, s, p->y_labels, "y");
    chart_ticks_draw(p, s, p->x2_labels, "x2");
    chart_ticks_draw(p, s, p->y2_labels, "y2");

    // Close cairo drawing context
    cairo_destroy(s->cairo_draw);

    if (s->output_format == SW_FORMAT_PNG) {
        int cairo_status = cairo_surface_write_to_png(s->cairo_surface, s->output_filename);
        if (cairo_status != 0) {
            snprintf(temp_err_string, 4096, "Could not create PNG file. Error was: %s.",
                     cairo_status_to_string(cairo_status));
            stch_fatal(__FILE__, __LINE__, temp_err_string);
            exit(1);
        }

        // Check that surface is OK
        cairo_status = cairo_surface_status(s->cairo_surface);
        if (cairo_status != 0) {
            snprintf(temp_err_string, 4096, "Could not create output file. Error was: %s.",
                     cairo_status_to_string(cairo_status));
            stch_fatal(__FILE__, __LINE__, temp_err_string);
            exit(1);
        }
    }

    cairo_surface_finish(s->cairo_surface);

    return 0;
}
