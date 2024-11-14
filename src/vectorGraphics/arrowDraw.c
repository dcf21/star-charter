// arrowDraw.c
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

#include <stdlib.h>
#include <math.h>

#include <gsl/gsl_math.h>

#include "coreUtils/errorReport.h"
#include "settings/chart_config.h"
#include "vectorGraphics/arrowDraw.h"
#include "vectorGraphics/cairo_page.h"

//! draw_arrow - Draw an arrow on the Cairo context.
//! \param [in] s - A <chart_config> structure defining the properties of the star chart to be drawn.
//! \param [in] lw - The line width to use when stroking the arrow stalk.
//! \param [in] head_start - Boolean (0 or 1) indicating whether to put an arrow head at (x0, y0)
//! \param [in] head_end - Boolean (0 or 1) indicating whether to put an arrow head at (x1, y1)
//! \param [in] x0 - X coordinate of start of arrow (Cairo pixels)
//! \param [in] y0 - Y coordinate of start of arrow (Cairo pixels)
//! \param [in] x1 - X coordinate of end of arrow (Cairo pixels)
//! \param [in] y1 - Y coordinate of end of arrow (Cairo pixels)

void draw_arrow(chart_config *s, const double lw, const int head_start, const int head_end,
                const double x0, const double y0, const double x1, const double y1) {
    double x_start, y_start, x_end, y_end, direction;

    // Set line width
    cairo_set_line_width(s->cairo_draw, lw);

    // Work out direction of arrow
    if (hypot(x1 - x0, y1 - y0) < 1e-200) direction = 0.0;
    else direction = atan2(x1 - x0, y1 - y0);

    // Draw arrowhead on beginning of arrow if desired
    if (head_start) {
        // Pointy back of arrowhead on one side
        const double x3 = x0 - CONST_ARROW_HEADSIZE * lw * sin((direction + M_PI) - CONST_ARROW_ANGLE / 2);
        const double y3 = y0 - CONST_ARROW_HEADSIZE * lw * cos((direction + M_PI) - CONST_ARROW_ANGLE / 2);

        // Pointy back of arrowhead on other side
        const double x5 = x0 - CONST_ARROW_HEADSIZE * lw * sin((direction + M_PI) + CONST_ARROW_ANGLE / 2);
        const double y5 = y0 - CONST_ARROW_HEADSIZE * lw * cos((direction + M_PI) + CONST_ARROW_ANGLE / 2);

        // Point where back of arrowhead crosses stalk
        const double x4 = x0 - CONST_ARROW_HEADSIZE * lw * sin(direction + M_PI) * (1.0 - CONST_ARROW_CONSTRICT) *
                               cos(CONST_ARROW_ANGLE / 2);
        const double y4 = y0 - CONST_ARROW_HEADSIZE * lw * cos(direction + M_PI) * (1.0 - CONST_ARROW_CONSTRICT) *
                               cos(CONST_ARROW_ANGLE / 2);

        // Draw arrow head
        cairo_new_path(s->cairo_draw);
        cairo_move_to(s->cairo_draw, x4, y4);
        cairo_line_to(s->cairo_draw, x3, y3);
        cairo_line_to(s->cairo_draw, x0, y0);
        cairo_line_to(s->cairo_draw, x5, y5);
        cairo_close_path(s->cairo_draw);
        cairo_fill(s->cairo_draw);

        // Start stalk of the arrow from the back of the arrow head
        x_start = x4;
        y_start = y4;
    } else {
        // Start stalk from the supplied coordinates (there is no arrow head)
        x_start = x0;
        y_start = y0;
    }

    // Draw arrowhead on end of arrow if desired
    if (head_end) {
        // Pointy back of arrowhead on one side
        const double x3 = x1 - CONST_ARROW_HEADSIZE * lw * sin(direction - CONST_ARROW_ANGLE / 2);
        const double y3 = y1 - CONST_ARROW_HEADSIZE * lw * cos(direction - CONST_ARROW_ANGLE / 2);

        // Pointy back of arrowhead on other side
        const double x5 = x1 - CONST_ARROW_HEADSIZE * lw * sin(direction + CONST_ARROW_ANGLE / 2);
        const double y5 = y1 - CONST_ARROW_HEADSIZE * lw * cos(direction + CONST_ARROW_ANGLE / 2);

        // Point where back of arrowhead crosses stalk
        const double x4 = x1 - CONST_ARROW_HEADSIZE * lw * sin(direction) * (1.0 - CONST_ARROW_CONSTRICT) *
                               cos(CONST_ARROW_ANGLE / 2);
        const double y4 = y1 - CONST_ARROW_HEADSIZE * lw * cos(direction) * (1.0 - CONST_ARROW_CONSTRICT) *
                               cos(CONST_ARROW_ANGLE / 2);

        // Draw arrow head
        cairo_new_path(s->cairo_draw);
        cairo_move_to(s->cairo_draw, x4, y4);
        cairo_line_to(s->cairo_draw, x3, y3);
        cairo_line_to(s->cairo_draw, x1, y1);
        cairo_line_to(s->cairo_draw, x5, y5);
        cairo_close_path(s->cairo_draw);
        cairo_fill(s->cairo_draw);

        // Start stalk of the arrow from the back of the arrow head
        x_end = x4;
        y_end = y4;
    } else {
        // Start stalk from the supplied coordinates (there is no arrow head)
        x_end = x1;
        y_end = y1;
    }

    // Draw stalk of arrow
    cairo_new_path(s->cairo_draw);
    cairo_move_to(s->cairo_draw, x_start, y_start);
    cairo_line_to(s->cairo_draw, x_end, y_end);
    cairo_stroke(s->cairo_draw);
}

//! draw_thick_arrow_segment - Draw a thick arrow on the Cairo context, whose outline can be stroked around.
//! \param [in] s - A <chart_config> structure defining the properties of the star chart to be drawn.
//! \param [in] lw - The line width to use when stroking the arrow stalk.
//! \param [in] head_start - Boolean (0 or 1) indicating whether to put an arrow head at (x0, y0)
//! \param [in] head_end - Boolean (0 or 1) indicating whether to put an arrow head at (x1, y1)
//! \param [in] x_list - List of X coordinates of points along the arrow
//! \param [in] y_list - List of Y coordinates of points along the arrow
//! \param [in] theta - List of direction of travel at each point along the arrow
//! \param [in] index_start - Index within array for the start of the arrow (inclusive of this index)
//! \param [in] index_end - Index within array for the end of the arrow (inclusive of this index)

void draw_thick_arrow_segment(
        chart_config *s, const double lw, const int head_start, const int head_end,
        const double *x_pixels, const double *y_pixels, const double *theta,
        const int index_start, const int index_end) {
    // We cannot draw segments with fewer than two points
    if (index_end < index_start + 1) return;

    // Arrow head geometry
    const double arrow_angle = CONST_ARROW_ANGLE_THICK;
    const double arrow_head_hypotenuse = CONST_ARROW_HEADSIZE_THICK * lw;
    const double arrow_head_back_half_height = arrow_head_hypotenuse * sin(arrow_angle / 2);
    const double arrow_head_total_length = arrow_head_hypotenuse * cos(arrow_angle / 2);
    const double arrow_head_back_x_ingress = arrow_head_total_length * CONST_ARROW_CONSTRICT;
    //const double arrow_head_back_angle = atan2(arrow_head_back_half_height, arrow_head_back_x_ingress);
    const double arrow_head_back_fraction_hidden_by_line = lw / arrow_head_back_half_height;
    const double arrow_head_back_x_ingress_unobscured = arrow_head_back_x_ingress *
                                                        (1 - arrow_head_back_fraction_hidden_by_line);
    const double arrow_head_distance_x0_to_tip = arrow_head_total_length - arrow_head_back_x_ingress_unobscured;

    // Draw arrowhead on beginning of arrow if desired
    if (head_start) {
        const double direction = theta[index_start];
        const double x0 = x_pixels[index_start] - arrow_head_distance_x0_to_tip * cos(direction);
        const double y0 = y_pixels[index_start] - arrow_head_distance_x0_to_tip * sin(direction);

        // Pointy back of arrowhead on one side
        const double x3 = x0 - arrow_head_hypotenuse * cos((direction + M_PI) - arrow_angle / 2);
        const double y3 = y0 - arrow_head_hypotenuse * sin((direction + M_PI) - arrow_angle / 2);

        // Pointy back of arrowhead on other side
        const double x5 = x0 - arrow_head_hypotenuse * cos((direction + M_PI) + arrow_angle / 2);
        const double y5 = y0 - arrow_head_hypotenuse * sin((direction + M_PI) + arrow_angle / 2);

        // Draw arrow head
        cairo_move_to(s->cairo_draw, x3, y3);
        cairo_line_to(s->cairo_draw, x0, y0);
        cairo_line_to(s->cairo_draw, x5, y5);
    } else {
        // Start stalk from the supplied coordinates (there is no arrow head)
        const double x0 = x_pixels[index_start];
        const double y0 = y_pixels[index_start];
        const double direction = theta[index_start];
        cairo_move_to(s->cairo_draw, x0 + lw * cos(direction + M_PI / 2),
                      y0 + lw * sin(direction + M_PI / 2));
    }

    // Stroke along top edge of the arrow
    for (int i = index_start + 1; i <= index_end; i++) {
        const double x0 = x_pixels[i];
        const double y0 = y_pixels[i];
        const double direction = theta[i];
        cairo_line_to(s->cairo_draw, x0 + lw * cos(direction + M_PI / 2),
                      y0 + lw * sin(direction + M_PI / 2));

    }

    // Draw arrowhead on end of arrow if desired
    if (head_end) {
        const double direction = theta[index_end];
        const double x1 = x_pixels[index_end] + arrow_head_distance_x0_to_tip * cos(direction);
        const double y1 = y_pixels[index_end] + arrow_head_distance_x0_to_tip * sin(direction);

        // Pointy back of arrowhead on one side
        const double x3 = x1 - arrow_head_hypotenuse * cos(direction - arrow_angle / 2);
        const double y3 = y1 - arrow_head_hypotenuse * sin(direction - arrow_angle / 2);

        // Pointy back of arrowhead on other side
        const double x5 = x1 - arrow_head_hypotenuse * cos(direction + arrow_angle / 2);
        const double y5 = y1 - arrow_head_hypotenuse * sin(direction + arrow_angle / 2);

        // Draw arrow head
        cairo_line_to(s->cairo_draw, x3, y3);
        cairo_line_to(s->cairo_draw, x1, y1);
        cairo_line_to(s->cairo_draw, x5, y5);
    }

    // Stroke along top edge of the arrow
    for (int i = index_end; i >= index_start; i--) {
        const double x0 = x_pixels[i];
        const double y0 = y_pixels[i];
        const double direction = theta[i];
        cairo_line_to(s->cairo_draw, x0 + lw * cos(direction - M_PI / 2),
                      y0 + lw * sin(direction - M_PI / 2));
    }

    // Close the path
    cairo_close_path(s->cairo_draw);
}

//! arrow_path_extend - Extend the path of an arrow by <distance> pixels beyond its last data point.
//! \param [in|out] path_len - The length of the array containing the arrow; will be increased by one point.
//! \param [in|out] x_list - The list of x coordinates along the path; Cairo pixels
//! \param [in|out] y_list - The list of y coordinates along the path; Cairo pixels
//! \param [in|out] theta_list - The list of position angles of travel along the path; Cairo pixels
//! \param [in] distance - The number of pixels by which the arrow should be extended

void arrow_path_extend(int *path_len, double *x_list, double *y_list, double *theta_list, const double distance) {
    // Fetch the index of the last point in the arrow. Return if there is no last point
    const int i_last = (*path_len) - 1;
    if (i_last < 0) return;

    // Fetch the coordinates of the last point
    const double x_last = x_list[i_last];
    const double y_last = y_list[i_last];
    const double theta_last = theta_list[i_last];

    // Fetch the coordinates of a point some way back, in case planet turns direction at the very end of the arrow
    int i0 = i_last;
    arrow_path_retract(&i0, x_list, y_list, theta_list, distance / 2);
    const double theta0 = atan2(y_last - y_list[i0], x_last - x_list[i0]);

    // Select which direction vector to use
    double theta_this = theta0;
    if (!gsl_finite(theta_this)) theta_this = theta_last;

    // Check that last point is not null
    if ((!gsl_finite((x_last))) || (!gsl_finite((y_last))) || (!gsl_finite((theta_this)))) return;

    // Add additional new points (add two points to make sure extended arrow is drawn with parallel sides)
    x_list[*path_len] = x_last + distance * 0.01 * cos(theta_this);
    y_list[*path_len] = y_last + distance * 0.01 * sin(theta_this);
    theta_list[*path_len] = theta_this;
    (*path_len)++;

    x_list[*path_len] = x_last + distance * cos(theta_this);
    y_list[*path_len] = y_last + distance * sin(theta_this);
    theta_list[*path_len] = theta_this;
    (*path_len)++;
}

//! arrow_path_retract - Retract the path of an arrow by <distance> pixels from its last data point.
//! \param [in|out] path_len - The length of the array containing the arrow; will be reduced as points are removed.
//! \param [in|out] x_list - The list of x coordinates along the path; Cairo pixels
//! \param [in|out] y_list - The list of y coordinates along the path; Cairo pixels
//! \param [in|out] theta_list - The list of position angles of travel along the path; Cairo pixels
//! \param [in] distance - The number of pixels by which the arrow should be retracted

void arrow_path_retract(int *path_len, const double *x_list, const double *y_list, const double *theta_list,
                        const double distance) {
    // Fetch the index of the last point in the arrow. Return if there is no last point
    const int i_last = (*path_len) - 1;
    if (i_last < 0) return;

    // Fetch the coordinates of the last point
    const double x_last = x_list[i_last];
    const double y_last = y_list[i_last];
    const double theta_last = theta_list[i_last];

    // Check that last point is not null
    if ((!gsl_finite((x_last))) || (!gsl_finite((y_last))) || (!gsl_finite((theta_last)))) return;

    // Work back along the arrow's path until we are a distance of <distance> from the final point
    for (int i = i_last; i >= 0; i--) {
        // Fetch the position of this point
        const double x_this = x_list[i];
        const double y_this = y_list[i];

        // If we've reached a break in the path, abort trying to shorten this arrow
        if ((!gsl_finite((x_this))) || (!gsl_finite((y_this)))) return;

        // Work out the distance of this point from the end of the arrow
        const double distance_this = hypot(x_this - x_last, y_this - y_last);

        if (distance_this > distance) {
            *path_len = i + 1;
            return;
        }
    }

    // We didn't find a point we could shorten the arrow to, so give up
    return;
}

//! draw_thick_arrow - Draw a thick arrow on the Cairo context, whose outline can be stroked around.
//! \param [in] s - A <chart_config> structure defining the properties of the star chart to be drawn.
//! \param [in] lw - The line width to use when stroking the arrow stalk.
//! \param [in] head_start - Boolean (0 or 1) indicating whether to put an arrow head at (x0, y0)
//! \param [in] head_end - Boolean (0 or 1) indicating whether to put an arrow head at (x1, y1)
//! \param [in] x_list - List of X coordinates of points along the arrow
//! \param [in] y_list - List of Y coordinates of points along the arrow
//! \param [in] theta - List of direction of travel at each point along the arrow
//! \param [in] point_count - Number of points along the arrow

void draw_thick_arrow(chart_config *s, const double lw, const int head_start, const int head_end,
                      const double *x_list, const double *y_list, const double *theta, const int point_count) {
    // Start path
    cairo_new_path(s->cairo_draw);

    // We cannot draw arrows with fewer than two points
    if (point_count < 2) return;

    // Create arrays to convert tangent plane coordinates to pixels
    const int array_max = point_count + 32;
    int x_pixels_length = 0;
    double *x_pixels = (double *) malloc(array_max * sizeof(double));
    double *y_pixels = (double *) malloc(array_max * sizeof(double));
    double *theta_pixels = (double *) malloc(array_max * sizeof(double));

    if ((x_pixels == NULL) || (y_pixels == NULL) || (theta_pixels == NULL)) {
        stch_fatal(__FILE__, __LINE__, "Malloc fail.");
        exit(1);
    }

    // Convert tangent plane coordinates to pixels
    for (int i_in = 0, start_i = 0, i_out = 0; ((i_in < point_count) && (i_out < array_max - 1)); i_in++) {
        double x, y;
        fetch_canvas_coordinates(&x, &y, x_list[i_in], y_list[i_in], s);

        // Check whether we've gone off the side of the canvas
        if ((!gsl_finite(x)) || (!gsl_finite(y))) {
            // If we have recorded a path prior to going off-side, then retract the arrow head so it's on the canvas
            if (start_i < i_out - 1) {
                arrow_path_retract(&i_out, x_pixels, y_pixels, theta_pixels, lw * 6);
            }

            // Start recording the next path once we re-enter the canvas
            start_i = i_out + 1;
        }

        // Check whether we've looped around left/right edge. If so, insert a NaN point to break the arrow
        if (i_out > 0) {
            const double x_last = x_pixels[i_out - 1];
            //const double y_last = y_pixels[i_out - 1];

            const double x_fraction = (x / s->cm - s->canvas_offset_x) / s->width;
            const double x_fraction_last = (x_last / s->cm - s->canvas_offset_x) / s->width;

            if (((x_fraction > 0.8) && (x_fraction_last < 0.2)) || ((x_fraction_last > 0.8) && (x_fraction < 0.2))) {
                if (start_i < i_out - 1) {
                    arrow_path_retract(&i_out, x_pixels, y_pixels, theta_pixels, lw * 6);
                }

                x_pixels[i_out] = GSL_NAN;
                y_pixels[i_out] = GSL_NAN;
                theta_pixels[i_out] = GSL_NAN;
                i_out++;
            }
        }

        // Output pixel coordinates
        x_pixels[i_out] = x;
        y_pixels[i_out] = y;
        theta_pixels[i_out] = theta[i_in];
        i_out++;
        x_pixels_length = i_out;
    }

    // Extend the end of the arrow beyond the last data point
    arrow_path_extend(&x_pixels_length, x_pixels, y_pixels, theta_pixels, lw * 8);

    // Loop along arrow path looking for segments that are separated by NaN segments
    for (int i = 0, start_i = 0; i <= x_pixels_length; i++) {
        if ((i >= x_pixels_length) || (!gsl_finite(x_pixels[i])) || (!gsl_finite(y_pixels[i]))) {
            if (start_i < i - 1) {
                draw_thick_arrow_segment(s, lw, head_start, head_end, x_pixels, y_pixels, theta_pixels,
                                         start_i, i - 1);
            }
            start_i = i + 1;
        }
    }

    // Free arrays
    if (x_pixels != NULL) free(x_pixels);
    if (y_pixels != NULL) free(y_pixels);
    if (theta_pixels != NULL) free(theta_pixels);
}
