// solarSystem.c
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
//! \param [in] label - Text label to place next to this object

void draw_solar_system_object(chart_config *s, cairo_page *page, const colour object_colour, const colour label_colour,
                              const double mag, double x, double y, const char *label) {
    // Convert tangent-plane coordinates into cairo pixel coordinates
    double x_canvas, y_canvas;
    fetch_canvas_coordinates(&x_canvas, &y_canvas, x, y, s);

    // Calculate the radius of this object on tha canvas, following the same magnitude scheme as for stars
    const double mag_reference = (mag < s->mag_max) ? (s->mag_max) : mag;
    const double size = get_star_size(s, mag_reference);

    // Calculate the radius of this object on tha canvas
    double size_cairo = size * s->dpi;

    // Draw a circular splodge on the star chart
    cairo_set_source_rgb(s->cairo_draw, object_colour.red, object_colour.grn, object_colour.blu);
    cairo_new_path(s->cairo_draw);
    cairo_arc(s->cairo_draw, x_canvas, y_canvas, size * s->dpi, 0, 2 * M_PI);
    cairo_fill_preserve(s->cairo_draw);
    cairo_set_source_rgb(s->cairo_draw, 0, 0, 0);
    cairo_set_line_width(s->cairo_draw, 0.5);
    cairo_stroke(s->cairo_draw);

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
    const double priority = s->must_show_all_ephemeris_labels ? -1 : 0;

    // Label this solar system object
    chart_label_buffer(page, s, label_colour, label,
                       (label_position[4]) {
                               {x, y, 0, horizontal_offset,  0,                  -1, 0},
                               {x, y, 0, -horizontal_offset, 0,                  1,  0},
                               {x, y, 0, 0,                  horizontal_offset,  0,  -1},
                               {x, y, 0, 0,                  -horizontal_offset, 0,  1}
                       }, 4,
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
//! \param [in] julian_date - Julian date of the chart (used to compute the Moon's phase)
//! \param [in] label - Text label to place next to this object

void draw_moon(chart_config *s, cairo_page *page, const colour label_colour,
               const double x, const double y, const double ra, const double dec, const double julian_date,
               const char *label) {
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
    double size_cairo = 0.5 * angle_scaling;

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

//! plot_solar_system - Plot the positions of solar system objects.
//! \param s - A <chart_config> structure defining the properties of the star chart to be drawn.
//! \param page - A <cairo_page> structure defining the cairo drawing context.

void plot_solar_system(chart_config *s, cairo_page *page) {
    // Loop over all the objects to display
    for (int obj_id = 0; obj_id < s->solar_system_final_count; obj_id++)
        // Cannot plot objects with empty ephemeris data structures
        if (s->solar_system_ephemeris_data[obj_id].point_count > 0) {
            // Look up the celestial coordinates of this object
            const double jd = s->solar_system_ephemeris_data[obj_id].data[0].jd;
            const double ra = s->solar_system_ephemeris_data[obj_id].data[0].ra;
            const double dec = s->solar_system_ephemeris_data[obj_id].data[0].dec;
            const double mag = s->solar_system_ephemeris_data[obj_id].data[0].mag;

            // Check whether this is the Moon
            const int is_moon = (str_cmp_no_case(s->solar_system_ephemeris_data[obj_id].obj_id, "P301") == 0);

            // Check whether this is the Sun
            const int is_sun = (str_cmp_no_case(s->solar_system_ephemeris_data[obj_id].obj_id, "sun") == 0);

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
                draw_moon(s, page, colour_label_final, x, y, ra, dec, jd,
                          s->solar_system_labels[obj_id]);
            } else {
                // Draw a circular splodge on the star chart
                draw_solar_system_object(s, page, colour_final, colour_label_final,
                                         mag, x, y, s->solar_system_labels[obj_id]);
            }
        }
}
