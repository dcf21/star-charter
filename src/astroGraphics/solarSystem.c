// solarSystem.c
// 
// -------------------------------------------------
// Copyright 2015-2025 Dominic Ford
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

#include <gsl/gsl_math.h>

#include "astroGraphics/ephemeris.h"
#include "astroGraphics/scaleBars.h"
#include "astroGraphics/stars.h"
#include "coreUtils/asciiDouble.h"
#include "coreUtils/errorReport.h"
#include "mathsTools/julianDate.h"
#include "mathsTools/projection.h"
#include "mathsTools/sphericalTrig.h"
#include "settings/chart_config.h"
#include "vectorGraphics/cairo_page.h"

//! solar_system_write_ephemeris_definitions - Write the ephemeris requests for the positions of each solar system
//! object we are to show.
//! \param [in] solar_system_ids - The list of the ID strings of the solar system bodies to show (e.g. `P1` for Mercury).
//! \param [in] julian_date - Julian date for which we are plotting the positions of solar system objects.
//! \param [in] object_count - The final number of solar system objects we are to display.
//! \param [in] ephemeris_definitions_out - Array into which we write the definition strings.

void solar_system_write_ephemeris_definitions(const char (*solar_system_ids)[N_TRACES_MAX][FNAME_LENGTH],
                                              double julian_date, int object_count,
                                              char (*ephemeris_definitions_out)[N_TRACES_MAX][FNAME_LENGTH]) {
    const double jd = julian_date;

    for (int i = 0; i < object_count; i++) {
        snprintf((*ephemeris_definitions_out)[i], FNAME_LENGTH, "%s,%.6f,%.6f,%.6f",
                 (*solar_system_ids)[i], jd, jd + 0.1, 0.5);
        (*ephemeris_definitions_out)[i][FNAME_LENGTH - 1] = '\0';
    }
}

//! draw_solar_system_object - Draw a single solar system object onto the chart.
//! \param [in] s - A <chart_config> structure defining the properties of the star chart to be drawn.
//! \param [in] page - A <cairo_page> structure defining the cairo drawing context.
//! \param [in] object_colour - The colour to use when drawing the object.
//! \param [in] label_colour - The colour to use when labelling the object.
//! \param [in] mag - Magnitude of this object
//! \param [in] x - Tangent-plane coordinates of this object (radians)
//! \param [in] y - Tangent-plane coordinates of this object (radians)
//! \param [in] is_comet - Boolean flag indicating whether to draw a cometary tail on this object
//! \param [in] sun_pa - Position angle of the Sun relative to this object; radians. Used to draw a tail, if shown.
//! \param [in] label - Text label to place next to this object
//! \param [in] priority_in - Optional priority for labelling this object; if NULL then a default value is computed
//! \param [in] possible_positions_in_count - Optional list of possible positions for labelling this object; if NULL
//! then a default set of positions are computed
//! \param [in] possible_positions_in - Optional list of possible positions for labelling this object; if NULL then a
//! default set of positions are computed

void draw_solar_system_object(chart_config *s, cairo_page *page, const colour object_colour, const colour label_colour,
                              const double mag, double x, double y, const int is_comet, const double sun_pa,
                              const char *label, const double *priority_in,
                              const int *possible_positions_in_count, const label_position *possible_positions_in) {
    // Convert tangent-plane coordinates into cairo pixel coordinates
    double x_canvas, y_canvas;
    fetch_canvas_coordinates(&x_canvas, &y_canvas, x, y, s);

    // Calculate the radius of this object on tha canvas, following the same magnitude scheme as for stars
    const double mag_reference = (mag < s->mag_max) ? (s->mag_max) : mag;
    const double size = get_star_size(s, mag_reference);

    // Calculate the radius of this object on tha canvas
    const double size_cairo = size * s->dpi;

    // Draw a tail if this is a comet
    if (is_comet) {
        const double fw = 15. * M_PI / 180; // half-width of comet's tail; radians
        const double tail_length = 8; // tail length, in units of the nucleus size

        // For simplicity, put centre of nucleus at (0,0), and scale coordinates by the nucleus size
        cairo_save(s->cairo_draw);
        cairo_translate(s->cairo_draw, x_canvas, y_canvas);
        cairo_scale(s->cairo_draw, size_cairo, size_cairo);

        // Create a radial pattern with fading opacity
        cairo_pattern_t *p = cairo_pattern_create_radial(0, 0, 0, 0, 0, tail_length);

        const int stop_count = 10;
        for (int i = 0; i < stop_count; i++) {
            const double pos = ((double) i) / stop_count;
            const double alpha = pow(1. - pos, 1.2);
            cairo_pattern_add_color_stop_rgba(p, pos,
                                              object_colour.red, object_colour.grn, object_colour.blu,
                                              alpha);
        }

        // Paint the comet's tail
        cairo_set_source(s->cairo_draw, p);
        cairo_new_path(s->cairo_draw);
        cairo_move_to(s->cairo_draw,
                      sin(sun_pa + M_PI / 2),
                      cos(sun_pa + M_PI / 2));
        cairo_line_to(s->cairo_draw,
                      sin(sun_pa - M_PI / 2),
                      cos(sun_pa - M_PI / 2));
        cairo_line_to(s->cairo_draw,
                      tail_length * sin(sun_pa - fw),
                      tail_length * cos(sun_pa - fw));
        cairo_line_to(s->cairo_draw,
                      tail_length * sin(sun_pa + fw),
                      tail_length * cos(sun_pa + fw));
        cairo_close_path(s->cairo_draw);
        cairo_fill(s->cairo_draw);

        // Free the pattern we used
        cairo_restore(s->cairo_draw);
        cairo_pattern_destroy(p);
    }

    // Draw a circular splodge on the star chart
    cairo_set_source_rgb(s->cairo_draw, object_colour.red, object_colour.grn, object_colour.blu);
    cairo_new_path(s->cairo_draw);
    cairo_arc(s->cairo_draw, x_canvas, y_canvas, size_cairo, 0, 2 * M_PI);

    if (!is_comet) {
        cairo_fill_preserve(s->cairo_draw);
        cairo_set_source_rgb(s->cairo_draw, 0, 0, 0);
        cairo_set_line_width(s->cairo_draw, 0.5);
        cairo_stroke(s->cairo_draw);
    } else {
        cairo_fill(s->cairo_draw);
        cairo_set_source_rgb(s->cairo_draw, 0, 0, 0);
        cairo_set_line_width(s->cairo_draw, 0.5);
        cairo_new_path(s->cairo_draw);
        cairo_arc(s->cairo_draw, x_canvas, y_canvas, size_cairo,
                  M_PI - sun_pa, -sun_pa);
        cairo_stroke(s->cairo_draw);
    }

    // Don't allow text labels to be placed over this object
    {
        double x_exclusion_0, y_exclusion_0;
        double x_exclusion_1, y_exclusion_1;
        fetch_graph_coordinates(x_canvas - size_cairo, y_canvas - size_cairo,
                                &x_exclusion_0, &y_exclusion_0, s);
        fetch_graph_coordinates(x_canvas + size_cairo, y_canvas + size_cairo,
                                &x_exclusion_1, &y_exclusion_1, s);
        chart_add_label_exclusion(page, s,
                                  x_exclusion_0, x_exclusion_1,
                                  y_exclusion_0, y_exclusion_1);
    }

    // How far should we move this label to the side of the planet, to avoid writing text on top of the planet?
    const double horizontal_offset = size * s->dpi + 0.075 * s->cm;

    // Calculate priority for this label
    double priority;

    if (priority_in != NULL) {
        priority = *priority_in;
    } else {
        priority = s->must_show_all_ephemeris_labels ? -1 : 0;
    }

    // Default possible positions for this label
    const label_position *possible_positions_default = (label_position[4]) {
            {x, y, 0, horizontal_offset,  0,                  -1, 0},
            {x, y, 0, -horizontal_offset, 0,                  1,  0},
            {x, y, 0, 0,                  horizontal_offset,  0,  -1},
            {x, y, 0, 0,                  -horizontal_offset, 0,  1}
    };
    const int possible_positions_default_count = 4;

    // Create a list of possible positions for labels for this object
    const label_position *possible_positions = NULL;
    int possible_positions_count = 0;

    if (possible_positions_in != NULL) {
        possible_positions = possible_positions_in;
        possible_positions_count = *possible_positions_in_count;
    } else {
        possible_positions = possible_positions_default;
        possible_positions_count = possible_positions_default_count;
    }


    // Label this solar system object
    chart_label_buffer(page, s, label_colour, label,
                       possible_positions, possible_positions_count,
                       0, 0, 1.2 * s->label_font_size_scaling,
                       0, 0, 0, priority);
}

//! draw_moon - Draw a representation of the Moon's phase onto the chart.
//! \param [in] s - A <chart_config> structure defining the properties of the star chart to be drawn.
//! \param [in] page - A <cairo_page> structure defining the cairo drawing context.
//! \param [in] label_colour - The colour to use when labelling the object.
//! \param [in] x - Tangent-plane coordinates of this object (radians)
//! \param [in] y - Tangent-plane coordinates of this object (radians)
//! \param [in] ra - Right ascension of the Moon (radians; J2000)
//! \param [in] dec - Declination of the Moon (radians; J2000)
//! \param [in] ang_size - Angular size of the Moon (arcseconds)
//! \param [in] julian_date - Julian date of the chart (used to compute the Moon's phase)
//! \param [in] label - Text label to place next to this object

void draw_moon(chart_config *s, cairo_page *page, const colour label_colour,
               const double x, const double y, const double ra, const double dec, const double ang_size,
               const double julian_date, const char *label) {
    // Calculate the position of the Sun
    double ra_sun_hr, dec_sun_deg;
    sun_pos(julian_date, &ra_sun_hr, &dec_sun_deg);
    const double ra_sun = ra_sun_hr * M_PI / 12;
    const double dec_sun = dec_sun_deg * M_PI / 180;

    // Calculate Sun-Moon separation
    const double sun_moon_sep = angDist_RADec(ra_sun, dec_sun, ra, dec);  // radians

    // Calculate Sun's position angle relative to the Moon
    const double crescent_pa_equatorial = position_angle(ra, dec, ra_sun, dec_sun);  // radians
    const double crescent_pa = s->position_angle * M_PI / 180 + crescent_pa_equatorial;

    // Calculate angular size of crescent
    const double crescent_size = sun_moon_sep;  // radians

    // Convert tangent-plane coordinates into cairo pixel coordinates
    double x_canvas, y_canvas;
    fetch_canvas_coordinates(&x_canvas, &y_canvas, x, y, s);

    // Calculate scaling of Cairo units per degree on the sky
    double angle_scaling = cairo_units_per_degree(s);

    // Calculate the radius of this object on tha canvas
    double size_cairo = (ang_size / 3600.) * angle_scaling;

    // Draw a circular splodge on the star chart
    cairo_set_source_rgba(s->cairo_draw, s->solar_system_moon_colour.red, s->solar_system_moon_colour.grn,
                          s->solar_system_moon_colour.blu, s->solar_system_moon_earthshine_intensity);
    cairo_new_path(s->cairo_draw);
    cairo_arc(s->cairo_draw, x_canvas, y_canvas, size_cairo, 0, 2 * M_PI);
    cairo_fill(s->cairo_draw);

    // Trace illuminated portion of the Moon
    cairo_set_source_rgb(s->cairo_draw, s->solar_system_moon_colour.red, s->solar_system_moon_colour.grn,
                         s->solar_system_moon_colour.blu);
    cairo_new_path(s->cairo_draw);

    // Trace illuminated edge of Moon
    const int theta_step_count = 100;
    for (int theta_step = 0; theta_step <= theta_step_count; theta_step++) {
        const double theta = theta_step * M_PI / theta_step_count;

        const double x_c = cos(theta);
        const double y_c = sin(theta);

        const double x_pt = x_canvas + size_cairo * (x_c * cos(crescent_pa) - y_c * sin(crescent_pa));
        const double y_pt = y_canvas - size_cairo * (x_c * sin(crescent_pa) + y_c * cos(crescent_pa));
        if (theta == 0) {
            cairo_move_to(s->cairo_draw, x_pt, y_pt);
        } else {
            cairo_line_to(s->cairo_draw, x_pt, y_pt);
        }
    }

    // Trace lunar terminator
    for (int theta_step = theta_step_count; theta_step >= 0; theta_step--) {
        const double theta = theta_step * M_PI / theta_step_count;

        const double x_c = cos(theta);
        const double y_c = sin(theta) * cos(crescent_size);

        const double x_pt = x_canvas + size_cairo * (x_c * cos(crescent_pa) - y_c * sin(crescent_pa));
        const double y_pt = y_canvas - size_cairo * (x_c * sin(crescent_pa) + y_c * cos(crescent_pa));
        cairo_line_to(s->cairo_draw, x_pt, y_pt);
    }

    // Fill illuminate portion of the Moon
    cairo_close_path(s->cairo_draw);
    cairo_fill(s->cairo_draw);

    // Don't allow text labels to be placed over this object
    {
        double x_exclusion_0, y_exclusion_0;
        double x_exclusion_1, y_exclusion_1;
        fetch_graph_coordinates(x_canvas - size_cairo, y_canvas - size_cairo,
                                &x_exclusion_0, &y_exclusion_0, s);
        fetch_graph_coordinates(x_canvas + size_cairo, y_canvas + size_cairo,
                                &x_exclusion_1, &y_exclusion_1, s);
        chart_add_label_exclusion(page, s,
                                  x_exclusion_0, x_exclusion_1,
                                  y_exclusion_0, y_exclusion_1);
    }

    // How far should we move this label to the side of the planet, to avoid writing text on top of the planet?
    const double horizontal_offset = size_cairo + 0.075 * s->cm;

    // Label this solar system object
    chart_label_buffer(page, s, label_colour, label,
                       (label_position[2]) {
                               {x, y, 0, horizontal_offset,  0, -1, 0},
                               {x, y, 0, -horizontal_offset, 0, 1,  0}
                       }, 2,
                       0, 0, 1.2 * s->label_font_size_scaling, 0, 0, 0, 0);
}

//! draw_sun - Draw a representation of the Moon's phase onto the chart.
//! \param [in] s - A <chart_config> structure defining the properties of the star chart to be drawn.
//! \param [in] page - A <cairo_page> structure defining the cairo drawing context.
//! \param [in] label_colour - The colour to use when labelling the object.
//! \param [in] x - Tangent-plane coordinates of this object (radians)
//! \param [in] y - Tangent-plane coordinates of this object (radians)
//! \param [in] ang_size - Angular size of the Moon (arcseconds)
//! \param [in] label - Text label to place next to this object

void draw_sun(chart_config *s, cairo_page *page, const colour label_colour,
              const double x, const double y, const double ang_size, const char *label) {
    // Convert tangent-plane coordinates into cairo pixel coordinates
    double x_canvas, y_canvas;
    fetch_canvas_coordinates(&x_canvas, &y_canvas, x, y, s);

    // Calculate scaling of Cairo units per degree on the sky
    double angle_scaling = cairo_units_per_degree(s);

    // Calculate the radius of this object on tha canvas
    double size_cairo = (ang_size / 3600.) * angle_scaling;

    // Draw a circular splodge on the star chart
    cairo_set_source_rgba(s->cairo_draw, s->solar_system_sun_col.red, s->solar_system_sun_col.grn,
                          s->solar_system_sun_col.blu, 1);
    cairo_new_path(s->cairo_draw);
    cairo_arc(s->cairo_draw, x_canvas, y_canvas, size_cairo, 0, 2 * M_PI);
    cairo_fill(s->cairo_draw);

    // Don't allow text labels to be placed over this object
    {
        double x_exclusion_0, y_exclusion_0;
        double x_exclusion_1, y_exclusion_1;
        fetch_graph_coordinates(x_canvas - size_cairo, y_canvas - size_cairo,
                                &x_exclusion_0, &y_exclusion_0, s);
        fetch_graph_coordinates(x_canvas + size_cairo, y_canvas + size_cairo,
                                &x_exclusion_1, &y_exclusion_1, s);
        chart_add_label_exclusion(page, s,
                                  x_exclusion_0, x_exclusion_1,
                                  y_exclusion_0, y_exclusion_1);
    }

    // How far should we move this label to the side of the planet, to avoid writing text on top of the planet?
    const double horizontal_offset = size_cairo + 0.075 * s->cm;

    // Label this solar system object
    chart_label_buffer(page, s, label_colour, label,
                       (label_position[2]) {
                               {x, y, 0, horizontal_offset,  0, -1, 0},
                               {x, y, 0, -horizontal_offset, 0, 1,  0}
                       }, 2,
                       0, 0, 1.2 * s->label_font_size_scaling, 0, 0, 0, 0);
}

//! plot_solar_system - Plot the positions of solar system objects.
//! \param s - A <chart_config> structure defining the properties of the star chart to be drawn.
//! \param page - A <cairo_page> structure defining the cairo drawing context.

void plot_solar_system(chart_config *s, cairo_page *page) {
    // Loop over all the objects to display
    for (int obj_id = 0; obj_id < s->solar_system_final_count; obj_id++) {
        const ephemeris *e = &s->solar_system_ephemeris_data[obj_id];

        // Cannot plot objects with empty ephemeris data structures
        if (e->point_count > 0) {
            // Look up the celestial coordinates of this object
            const double jd = e->data[0].jd;
            const double ra = e->data[0].ra; // radians
            const double dec = e->data[0].dec; // radians
            const double ang_size = e->data[0].angular_size; // arcseconds
            const double mag = e->data[0].mag;
            const double sun_pa = e->data[0].sun_pa; // radians
            const int is_comet = e->is_comet;

            // Check whether this is the Moon
            const int is_moon = ((str_cmp_no_case(e->obj_id, "P301") == 0) ||
                                 (str_cmp_no_case(e->obj_id, "moon") == 0));

            // Check whether this is the Sun
            const int is_sun = (str_cmp_no_case(e->obj_id, "sun") == 0);

            // Work out coordinates of this object on the star chart (tangent plane coordinates)
            double x, y;  // radians
            plane_project(&x, &y, s, ra, dec, is_sun);

            // Ignore this object if it falls outside the plot area
            if ((!gsl_finite(x)) || (!gsl_finite(y)) ||
                (x < s->x_min) || (x > s->x_max) || (y < s->y_min) || (y > s->y_max)) {
                continue;
            }

            // Work out which colour to use for this object
            const int colour_index = obj_id % s->solar_system_colour_final_count;
            const colour colour_final = s->solar_system_colour[colour_index];

            const int colour_label_index = obj_id % s->solar_system_label_colour_final_count;
            const colour colour_label_final = s->solar_system_label_colour[colour_label_index];

            // Draw this object
            if (is_moon && s->solar_system_show_moon_phase) {
                // Show Moon with representation of phase
                draw_moon(s, page, colour_label_final, x, y, ra, dec, ang_size, jd,
                          s->solar_system_labels[obj_id]);
            } else if (is_sun && s->solar_system_sun_actual_size) {
                // Show Sun at actual scale
                draw_sun(s, page, colour_label_final, x, y, ang_size, s->solar_system_labels[obj_id]);
            } else {
                // If object is fainter than mag limit, then ensure the splodge we draw is not too small
                const double mag_size = gsl_min(mag, s->solar_system_minimum_size);

                // Draw a circular splodge on the star chart
                draw_solar_system_object(s, page, colour_final, colour_label_final,
                                         mag_size, x, y, is_comet, sun_pa,
                                         s->solar_system_labels[obj_id],
                                         NULL, NULL, NULL);
            }
        }
    }
}
