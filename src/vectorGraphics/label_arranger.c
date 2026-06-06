// label_arranger.c
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

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <cairo/cairo.h>
#include <cairo/cairo-ps.h>

#include <gsl/gsl_math.h>

#include "coreUtils/errorReport.h"
#include "listTools/ltMemory.h"
#include "settings/chart_config.h"
#include "vectorGraphics/cairo_page.h"
#include "vectorGraphics/label_arranger.h"

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

int chart_check_label_exclusion(const cairo_page *p, double x_min, double x_max, double y_min, double y_max) {
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
        if (
            (!gsl_finite(pos.x)) || (!gsl_finite(pos.y)) || (!gsl_finite(pos.rotation)) ||
            (!gsl_finite(pos.offset_x)) || (!gsl_finite(pos.offset_y))
        ) {
            continue;
        }

        // Select font
        cairo_text_extents_t extents;
        cairo_set_font_size(s->cairo_draw, 2 * s->mm * font_size * s->font_size);
        cairo_select_font_face(s->cairo_draw, s->font_family,
                               font_italic ? CAIRO_FONT_SLANT_ITALIC : CAIRO_FONT_SLANT_NORMAL,
                               font_bold ? CAIRO_FONT_WEIGHT_BOLD : CAIRO_FONT_WEIGHT_NORMAL);

        // Measure text bounding box
        cairo_text_extents(s->cairo_draw, label, &extents);

        double x_canvas_pivot, y_canvas_pivot;
        fetch_canvas_coordinates(&x_canvas_pivot, &y_canvas_pivot, pos.x, pos.y, s);

        // Apply user-requested text offset
        x_canvas_pivot += pos.offset_x;
        y_canvas_pivot += pos.offset_y;

        // Work out offset to achieve requested text alignment
        double x_alignment_offset = 0;
        double y_alignment_offset = 0;

        switch (pos.h_align) {
            case 1:
                x_alignment_offset -= extents.width + extents.x_bearing; // right align
                break;
            case 0:
                x_alignment_offset -= extents.width / 2 + extents.x_bearing; // centre align
                break;
            case -1:
                x_alignment_offset -= extents.x_bearing; // left align
                break;
            default:
                break;
        }

        switch (pos.v_align) {
            case 1:
                y_alignment_offset -= extents.height + extents.y_bearing; // top align
                break;
            case 0:
                y_alignment_offset -= extents.height / 2 + extents.y_bearing; // centre align
                break;
            case -1:
                y_alignment_offset -= extents.y_bearing; // bottom align
                break;
            default:
                break;
        }

        // Work out final canvas position, neglecting text rotation
        const double x_canvas = x_canvas_pivot + x_alignment_offset;
        const double y_canvas = y_canvas_pivot + y_alignment_offset;

        // Calculate approximate bounding box for this label
        const double margin = multiple_labels ? 0.01 : 0.07;
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
        const double x_margin = margin * (x_max - x_min) * (extra_margin + 1.);
        const double y_margin = margin * (y_max - y_min) * 2.3 * (extra_margin + 1.);

        // Enlarge bounding box by margin
        x_min -= x_margin;
        x_max += x_margin;
        y_min -= y_margin;
        y_max += y_margin;

        // Reject label if it collides with one we've already rendered
        // We do collision detection without applying rotation, to keep things simple
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

        // Put label at origin and rotate as requested
        cairo_save(s->cairo_draw);
        cairo_translate(s->cairo_draw, x_canvas_pivot, y_canvas_pivot);
        cairo_rotate(s->cairo_draw, pos.rotation * M_PI / 180);

        // If <make_background> is set, we smear the background around the label with a dark colour
        if (make_background && s->plot_galaxy_map) {
            const double offset = 0.2 * s->mm;
            cairo_set_source_rgba(s->cairo_draw,
                                  s->galaxy_col0.red, s->galaxy_col0.grn, s->galaxy_col0.blu,
                                  s->galaxy_col0.alpha);
            for (double theta = 0; theta < 359.; theta += 30.) {
                cairo_move_to(s->cairo_draw,
                              x_alignment_offset + offset * sin(theta * M_PI / 180),
                              y_alignment_offset + offset * cos(theta * M_PI / 180));
                cairo_show_text(s->cairo_draw, label);
            }
        }

        // Render the text label itself
        cairo_set_source_rgba(s->cairo_draw, colour.red, colour.grn, colour.blu, colour.alpha);
        cairo_move_to(s->cairo_draw, x_alignment_offset, y_alignment_offset);
        cairo_show_text(s->cairo_draw, label);

        // Undo canvas coordinate transformation
        cairo_restore(s->cairo_draw);

        // Finished
        return 0;
    }

    // No acceptable position was found for this label
    return 1;
}
