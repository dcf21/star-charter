// deepSky.c
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

#include <gsl/gsl_math.h>

#include "astroGraphics/deepSky.h"
#include "coreUtils/asciiDouble.h"
#include "coreUtils/errorReport.h"
#include "mathsTools/projection.h"
#include "settings/chart_config.h"
#include "vectorGraphics/cairo_page.h"

// -------------- Renderers for the <coloured> style scheme --------------

//! draw_open_cluster_coloured - Draw an open cluster in whichever is the 'coloured' DSO representation style.
//! \param s - A <chart_config> structure defining the properties of the star chart to be drawn.
//! \param x_canvas - The X position of the DSO, in Cairo coordinates
//! \param y_canvas - The Y position of the DSO, in Cairo coordinates
//! \param radius - The radius of the DSO, in Cairo coordinates

void draw_open_cluster_coloured(chart_config *s, const double x_canvas, const double y_canvas, const double radius) {
    cairo_set_line_width(s->cairo_draw, 1);
    cairo_new_path(s->cairo_draw);
    cairo_arc(s->cairo_draw, x_canvas, y_canvas, radius, 0, 2 * M_PI);
    cairo_set_source_rgb(s->cairo_draw, s->dso_cluster_col.red, s->dso_cluster_col.grn, s->dso_cluster_col.blu);
    cairo_fill_preserve(s->cairo_draw);
    cairo_set_source_rgb(s->cairo_draw, s->dso_outline_col.red, s->dso_outline_col.grn, s->dso_outline_col.blu);
    cairo_stroke(s->cairo_draw);
}

void draw_globular_cluster_coloured(chart_config *s, const double x_canvas, const double y_canvas,
                                    const double radius) {
    cairo_set_line_width(s->cairo_draw, 1);
    cairo_new_path(s->cairo_draw);
    cairo_arc(s->cairo_draw, x_canvas, y_canvas, radius, 0, 2 * M_PI);
    cairo_set_source_rgb(s->cairo_draw, s->dso_cluster_col.red, s->dso_cluster_col.grn, s->dso_cluster_col.blu);
    cairo_fill_preserve(s->cairo_draw);
    cairo_set_source_rgb(s->cairo_draw, s->dso_outline_col.red, s->dso_outline_col.grn, s->dso_outline_col.blu);
    cairo_stroke(s->cairo_draw);
    cairo_set_line_width(s->cairo_draw, 0.5);
    cairo_new_path(s->cairo_draw);
    cairo_move_to(s->cairo_draw, x_canvas - radius, y_canvas);
    cairo_line_to(s->cairo_draw, x_canvas + radius, y_canvas);
    cairo_move_to(s->cairo_draw, x_canvas, y_canvas - radius);
    cairo_line_to(s->cairo_draw, x_canvas, y_canvas + radius);
    cairo_stroke(s->cairo_draw);
}

void draw_planetary_nebula_coloured(chart_config *s, const double x_canvas, const double y_canvas,
                                    const double point_size) {
    const double r0 = point_size * 0.75;
    const double r1 = point_size * 1.5;

    cairo_set_line_width(s->cairo_draw, 1);
    cairo_new_path(s->cairo_draw);
    cairo_arc(s->cairo_draw, x_canvas, y_canvas, r0, 0, 2 * M_PI);
    cairo_set_source_rgb(s->cairo_draw, s->dso_nebula_col.red, s->dso_nebula_col.grn, s->dso_nebula_col.blu);
    cairo_fill_preserve(s->cairo_draw);
    cairo_set_source_rgb(s->cairo_draw, s->dso_outline_col.red, s->dso_outline_col.grn, s->dso_outline_col.blu);
    cairo_stroke(s->cairo_draw);

    for (int i = 0; i < 4; i++) {
        const double theta = M_PI / 2 * i;
        cairo_new_path(s->cairo_draw);

        cairo_move_to(s->cairo_draw, x_canvas + r0 * sin(theta), y_canvas + r0 * cos(theta));
        cairo_line_to(s->cairo_draw, x_canvas + r1 * sin(theta), y_canvas + r1 * cos(theta));
        cairo_stroke(s->cairo_draw);
    }
}

void draw_ellipsoid_coloured(chart_config *s, const double axis_pa, const double x_canvas, const double y_canvas,
                             const double radius_major, const double radius_minor, const double opacity,
                             const double colour_r, const double colour_g, const double colour_b) {
    cairo_new_path(s->cairo_draw);
    cairo_save(s->cairo_draw);
    cairo_translate(s->cairo_draw, x_canvas, y_canvas);
    cairo_rotate(s->cairo_draw, (90 - axis_pa) * M_PI / 180);
    cairo_scale(s->cairo_draw, radius_major, radius_minor);
    cairo_arc(s->cairo_draw, 0, 0, 1, 0, 2 * M_PI);
    cairo_restore(s->cairo_draw);
    cairo_set_source_rgba(s->cairo_draw, colour_r, colour_g, colour_b, opacity);
    cairo_fill_preserve(s->cairo_draw);
    cairo_set_line_width(s->cairo_draw, 0.5);
    cairo_set_source_rgb(s->cairo_draw, s->dso_outline_col.red, s->dso_outline_col.grn, s->dso_outline_col.blu);
    cairo_stroke(s->cairo_draw);
}

void draw_galaxy_coloured(chart_config *s, const double axis_pa, const double x_canvas, const double y_canvas,
                          const double radius_major, const double radius_minor) {
    draw_ellipsoid_coloured(
            s, axis_pa, x_canvas, y_canvas, radius_major, radius_minor,
            1, s->dso_galaxy_col.red, s->dso_galaxy_col.grn, s->dso_galaxy_col.blu);
}

void draw_dark_nebula_coloured(chart_config *s, const double axis_pa, const double x_canvas, const double y_canvas,
                               const double radius_major, const double radius_minor) {
    draw_ellipsoid_coloured(
            s, axis_pa, x_canvas, y_canvas, radius_major, radius_minor,
            0.75,
            gsl_max(s->galaxy_col0.red, s->twilight_zenith_col.red),
            gsl_max(s->galaxy_col0.grn, s->twilight_zenith_col.grn),
            gsl_max(s->galaxy_col0.blu, s->twilight_zenith_col.blu));
}

void draw_galaxy_cluster_coloured(chart_config *s, const double axis_pa, const double x_canvas, const double y_canvas,
                                  const double radius_major, const double radius_minor, const int size_unset) {
    // Set dashed line style
    const double dash_style[1] = {0.5 * s->mm};
    cairo_set_dash(s->cairo_draw, dash_style, 1, 0);

    cairo_new_path(s->cairo_draw);
    cairo_save(s->cairo_draw);
    cairo_translate(s->cairo_draw, x_canvas, y_canvas);
    if (!size_unset) {
        // Draw ellipsoid around clusters of known size
        cairo_rotate(s->cairo_draw, (90 - axis_pa) * M_PI / 180);
        cairo_scale(s->cairo_draw, radius_major, radius_minor);
        cairo_arc(s->cairo_draw, 0, 0, 1, 0, 2 * M_PI);
    } else {
        // Draw a cross-hair for clusters of unknown size
        cairo_new_path(s->cairo_draw);
        cairo_move_to(s->cairo_draw, -radius_major, 0);
        cairo_line_to(s->cairo_draw, radius_major, 0);
        cairo_move_to(s->cairo_draw, 0, -radius_major);
        cairo_line_to(s->cairo_draw, 0, radius_major);
    }
    cairo_restore(s->cairo_draw);
    cairo_set_line_width(s->cairo_draw, 1);
    cairo_set_source_rgb(s->cairo_draw, s->dso_outline_col.red, s->dso_outline_col.grn, s->dso_outline_col.blu);
    cairo_stroke(s->cairo_draw);

    // Unset dashed line style
    cairo_set_dash(s->cairo_draw, NULL, 0, 0);
}

void draw_generic_nebula_coloured(chart_config *s, const double x_canvas, const double y_canvas,
                                  const double point_size) {
    cairo_set_line_width(s->cairo_draw, 0.5);
    cairo_new_path(s->cairo_draw);
    cairo_move_to(s->cairo_draw, x_canvas - point_size, y_canvas - point_size);
    cairo_line_to(s->cairo_draw, x_canvas + point_size, y_canvas - point_size);
    cairo_line_to(s->cairo_draw, x_canvas + point_size, y_canvas + point_size);
    cairo_line_to(s->cairo_draw, x_canvas - point_size, y_canvas + point_size);
    cairo_close_path(s->cairo_draw);
    cairo_set_source_rgb(s->cairo_draw, s->dso_nebula_col.red, s->dso_nebula_col.grn, s->dso_nebula_col.blu);
    cairo_fill_preserve(s->cairo_draw);
    cairo_set_source_rgb(s->cairo_draw, s->dso_outline_col.red, s->dso_outline_col.grn, s->dso_outline_col.blu);
    cairo_stroke(s->cairo_draw);
}

// -------------- Renderers for the <fuzzy> style scheme --------------

//! draw_open_cluster_fuzzy - Draw an open cluster in whichever is the 'fuzzy' DSO representation style.
//! \param s - A <chart_config> structure defining the properties of the star chart to be drawn.
//! \param x_canvas - The X position of the DSO, in Cairo coordinates
//! \param y_canvas - The Y position of the DSO, in Cairo coordinates
//! \param radius - The radius of the DSO, in Cairo coordinates

void draw_generic_object_fuzzy(chart_config *s, const int stop_count, const double *offsets, const double *alphas,
                               const double red, const double grn, const double blu) {
    cairo_pattern_t *p = cairo_pattern_create_radial(0, 0, 0, 0, 0, 1);

    for (int i = 0; i < stop_count; i++) {
        cairo_pattern_add_color_stop_rgba(p, offsets[i],
                                          red, grn, blu, alphas[i]);
    }

    cairo_set_source(s->cairo_draw, p);
    cairo_arc(s->cairo_draw, 0, 0, 1, 0, 2 * M_PI);
    cairo_fill(s->cairo_draw);
    cairo_restore(s->cairo_draw);
    cairo_pattern_destroy(p);
}

void draw_open_cluster_fuzzy(chart_config *s, const double x_canvas, const double y_canvas, const double radius) {
    const double r_feather_end = 1.35;

    cairo_new_path(s->cairo_draw);
    cairo_save(s->cairo_draw);
    cairo_translate(s->cairo_draw, x_canvas, y_canvas);
    cairo_scale(s->cairo_draw, radius * r_feather_end, radius * r_feather_end);

#define stop_count_cluster (10)
    const double offsets[stop_count_cluster] = {0, 0.2, 0.33, 0.47, 0.6, 0.65, 0.7, 0.8, 0.92, 1};
    const double alphas[stop_count_cluster] = {0, 0, 0.1, 0.3, 0.66, 0.75, 0.66, 0.3, 0.1, 0};
    draw_generic_object_fuzzy(s, stop_count_cluster, offsets, alphas,
                              s->dso_cluster_col.red, s->dso_cluster_col.grn, s->dso_cluster_col.blu);
}

void draw_globular_cluster_fuzzy(chart_config *s, const double x_canvas, const double y_canvas, const double radius) {
    draw_open_cluster_fuzzy(s, x_canvas, y_canvas, radius);
}

void draw_planetary_nebula_fuzzy(chart_config *s, const double x_canvas, const double y_canvas,
                                 const double point_size) {
    const double r0 = point_size * 0.75;
    const double r1 = point_size * 1.5;

    cairo_set_line_width(s->cairo_draw, 2);
    cairo_new_path(s->cairo_draw);
    cairo_arc(s->cairo_draw, x_canvas, y_canvas, r0, 0, 2 * M_PI);
    cairo_set_source_rgb(s->cairo_draw, s->dso_nebula_col.red, s->dso_nebula_col.grn, s->dso_nebula_col.blu);
    cairo_stroke(s->cairo_draw);

    for (int i = 0; i < 4; i++) {
        const double theta = M_PI / 2 * i;
        cairo_new_path(s->cairo_draw);

        cairo_move_to(s->cairo_draw, x_canvas + r0 * sin(theta), y_canvas + r0 * cos(theta));
        cairo_line_to(s->cairo_draw, x_canvas + r1 * sin(theta), y_canvas + r1 * cos(theta));
        cairo_stroke(s->cairo_draw);
    }
}

void draw_ellipsoid_fuzzy(chart_config *s, const double axis_pa, const double x_canvas, const double y_canvas,
                          const double radius_major, const double radius_minor, const double opacity,
                          const double colour_r, const double colour_g, const double colour_b) {
    const double r_feather_end = 1.1;

    cairo_new_path(s->cairo_draw);
    cairo_save(s->cairo_draw);
    cairo_translate(s->cairo_draw, x_canvas, y_canvas);
    cairo_rotate(s->cairo_draw, (90 - axis_pa) * M_PI / 180);
    cairo_scale(s->cairo_draw, radius_major * r_feather_end, radius_minor * r_feather_end);

#define stop_count_galaxy (5)
    const double offsets[stop_count_galaxy] = {
            0, 0.3, 0.5, 0.75, 1};
    const double alphas[stop_count_galaxy] = {
            0.8 * opacity, 0.75 * opacity, 0.35 * opacity, 0.11 * opacity, 0};
    draw_generic_object_fuzzy(s, stop_count_galaxy, offsets, alphas,
                              colour_r, colour_g, colour_b);
}

void draw_galaxy_fuzzy(chart_config *s, const double axis_pa, const double x_canvas, const double y_canvas,
                       const double radius_major, const double radius_minor) {
    draw_ellipsoid_fuzzy(
            s, axis_pa, x_canvas, y_canvas, radius_major, radius_minor,
            0.75, s->dso_galaxy_col.red, s->dso_galaxy_col.grn, s->dso_galaxy_col.blu);
}

void draw_dark_nebula_fuzzy(chart_config *s, const double axis_pa, const double x_canvas, const double y_canvas,
                            const double radius_major, const double radius_minor) {
    draw_ellipsoid_fuzzy(
            s, axis_pa, x_canvas, y_canvas, radius_major, radius_minor,
            0.75,
            gsl_max(s->galaxy_col0.red, s->twilight_zenith_col.red),
            gsl_max(s->galaxy_col0.grn, s->twilight_zenith_col.grn),
            gsl_max(s->galaxy_col0.blu, s->twilight_zenith_col.blu));
}

void draw_galaxy_cluster_fuzzy(chart_config *s, const double axis_pa, const double x_canvas, const double y_canvas,
                               const double radius_major, const double radius_minor) {
    draw_ellipsoid_fuzzy(
            s, axis_pa, x_canvas, y_canvas, radius_major, radius_minor,
            0.75, s->dso_galaxy_col.red, s->dso_galaxy_col.grn, s->dso_galaxy_col.blu);
}

void draw_generic_nebula_fuzzy(chart_config *s, const double x_canvas, const double y_canvas, const double point_size) {
    const double r_feather_end = 1.35;

    cairo_new_path(s->cairo_draw);
    cairo_save(s->cairo_draw);
    cairo_translate(s->cairo_draw, x_canvas, y_canvas);
    cairo_scale(s->cairo_draw, point_size * r_feather_end, point_size * r_feather_end);

#define stop_count_nebula (10)
    const double offsets[stop_count_nebula] = {0, 0.2, 0.33, 0.47, 0.6, 0.65, 0.7, 0.8, 0.92, 1};
    const double alphas[stop_count_nebula] = {0, 0, 0.1, 0.3, 0.66, 0.75, 0.66, 0.3, 0.1, 0};
    draw_generic_object_fuzzy(s, stop_count_nebula, offsets, alphas,
                              s->dso_nebula_col.red, s->dso_nebula_col.grn, s->dso_nebula_col.blu);
}

// -------------- Top-level renders for DSO types --------------

//! draw_open_cluster - Draw an open cluster in whichever is the currently selected DSO representation style.
//! \param [in|out] s - A <chart_config> structure defining the properties of the star chart to be drawn.
//! \param [in] x_tangent - The X position of the DSO, in tangent plane coordinates, radians
//! \param [in] y_tangent - The Y position of the DSO, in tangent plane coordinates, radians
//! \param [in] x_canvas - The X position of the DSO, in Cairo pixels
//! \param [in] y_canvas - The Y position of the DSO, in Cairo pixels
//! \param [in] radius - The radius of the DSO, in Cairo coordinates

void draw_open_cluster(chart_config *s, const double x_tangent, const double y_tangent,
                       const double x_canvas, const double y_canvas, const double radius,
                       label_position *possible_positions, int *possible_position_count) {
    // Render object
    switch (s->dso_style) {
        case SW_DSO_STYLE_FUZZY:
            draw_open_cluster_fuzzy(s, x_canvas, y_canvas, radius);
            break;
        case SW_DSO_STYLE_COLOURED:
            draw_open_cluster_coloured(s, x_canvas, y_canvas, radius);
            break;
    }

    // Populate possible positions for the label for this object
    if (possible_positions != NULL) {
        const double offset = 0.8 * s->mm + radius;
        possible_positions[0] = (label_position) {x_tangent, y_tangent, 0, offset, 0, -1, 0};
        possible_positions[1] = (label_position) {x_tangent, y_tangent, 0, -offset, 0, 1, 0};
        possible_positions[2] = (label_position) {x_tangent, y_tangent, 0, 0, offset, 0, -1};
        possible_positions[3] = (label_position) {x_tangent, y_tangent, 0, 0, -offset, 0, 1};
        *possible_position_count = 4;
    }
}

void draw_globular_cluster(chart_config *s, const double x_tangent, const double y_tangent,
                           const double x_canvas, const double y_canvas, const double radius,
                           label_position *possible_positions, int *possible_position_count) {
    // Render object
    switch (s->dso_style) {
        case SW_DSO_STYLE_FUZZY:
            draw_globular_cluster_fuzzy(s, x_canvas, y_canvas, radius);
            break;
        case SW_DSO_STYLE_COLOURED:
            draw_globular_cluster_coloured(s, x_canvas, y_canvas, radius);
            break;
    }

    // Populate possible positions for the label for this object
    if (possible_positions != NULL) {
        const double offset = 0.8 * s->mm + radius;
        possible_positions[0] = (label_position) {x_tangent, y_tangent, 0, offset, 0, -1, 0};
        possible_positions[1] = (label_position) {x_tangent, y_tangent, 0, -offset, 0, 1, 0};
        possible_positions[2] = (label_position) {x_tangent, y_tangent, 0, 0, offset, 0, -1};
        possible_positions[3] = (label_position) {x_tangent, y_tangent, 0, 0, -offset, 0, 1};
        *possible_position_count = 4;
    }
}

void draw_planetary_nebula(chart_config *s, const double x_tangent, const double y_tangent,
                           const double x_canvas, const double y_canvas, const double point_size,
                           label_position *possible_positions, int *possible_position_count) {
    // Render object
    switch (s->dso_style) {
        case SW_DSO_STYLE_FUZZY:
            draw_planetary_nebula_fuzzy(s, x_canvas, y_canvas, point_size);
            break;
        case SW_DSO_STYLE_COLOURED:
            draw_planetary_nebula_coloured(s, x_canvas, y_canvas, point_size);
            break;
    }

    // Populate possible positions for the label for this object
    if (possible_positions != NULL) {
        const double offset = 0.8 * s->mm + point_size;
        possible_positions[0] = (label_position) {x_tangent, y_tangent, 0, offset, 0, -1, 0};
        possible_positions[1] = (label_position) {x_tangent, y_tangent, 0, -offset, 0, 1, 0};
        possible_positions[2] = (label_position) {x_tangent, y_tangent, 0, 0, offset, 0, -1};
        possible_positions[3] = (label_position) {x_tangent, y_tangent, 0, 0, -offset, 0, 1};
        *possible_position_count = 4;
    }
}

void populate_galaxy_label_position(label_position *possible_position,
                                    const double x_tangent, const double y_tangent,
                                    const double radius, const double theta) {
    const double theta_deg = fmod(theta * 180 / M_PI + 3600, 360);
    int h_align = 0, v_align = 0;

    if ((theta_deg >= 315) || (theta_deg < 45)) {
        h_align = 0;
        v_align = -1;
    } else if (theta_deg < 135) {
        h_align = -1;
        v_align = 0;
    } else if (theta_deg < 225) {
        h_align = 0;
        v_align = 1;
    } else if (theta_deg < 315) {
        h_align = 1;
        v_align = 0;
    }

    *possible_position = (label_position) {x_tangent, y_tangent, 0,
                                           radius * sin(theta),
                                           radius * cos(theta),
                                           h_align, v_align};
}

void draw_galaxy(chart_config *s, const double axis_pa, const double x_tangent, const double y_tangent,
                 const double x_canvas, const double y_canvas,
                 const double radius_major, const double radius_minor,
                 label_position *possible_positions, int *possible_position_count) {
    // Render object
    switch (s->dso_style) {
        case SW_DSO_STYLE_FUZZY:
            draw_galaxy_fuzzy(s, axis_pa, x_canvas, y_canvas, radius_major, radius_minor);
            break;
        case SW_DSO_STYLE_COLOURED:
            draw_galaxy_coloured(s, axis_pa, x_canvas, y_canvas, radius_major, radius_minor);
            break;
    }

    // Populate possible positions for the label for this object
    if (possible_positions != NULL) {
        const double offset_major = 0.8 * s->mm + radius_major;
        const double offset_minor = 0.8 * s->mm + radius_minor;
        const double t = axis_pa * M_PI / 180;

        populate_galaxy_label_position(possible_positions + 0, x_tangent, y_tangent, offset_major, t);
        populate_galaxy_label_position(possible_positions + 1, x_tangent, y_tangent, offset_minor, t + M_PI / 2);
        populate_galaxy_label_position(possible_positions + 2, x_tangent, y_tangent, offset_major, t + M_PI);
        populate_galaxy_label_position(possible_positions + 3, x_tangent, y_tangent, offset_minor, t + 3 * M_PI / 2);
        *possible_position_count = 4;
    }
}

void draw_galaxy_cluster(chart_config *s, const double axis_pa, const double x_tangent, const double y_tangent,
                         const double x_canvas, const double y_canvas, const double radius_major,
                         const double radius_minor, const int size_unset, label_position *possible_positions,
                         int *possible_position_count) {
    // Render object
    switch (s->dso_style) {
        case SW_DSO_STYLE_FUZZY:
            draw_galaxy_cluster_fuzzy(s, axis_pa, x_canvas, y_canvas, radius_major, radius_minor);
            break;
        case SW_DSO_STYLE_COLOURED:
            draw_galaxy_cluster_coloured(s, axis_pa, x_canvas, y_canvas, radius_major, radius_minor, size_unset);
            break;
    }

    // Populate possible positions for the label for this object
    if (possible_positions != NULL) {
        const double offset_major = 0.8 * s->mm + radius_major;
        const double offset_minor = 0.8 * s->mm + radius_minor;
        const double t = axis_pa * M_PI / 180;

        populate_galaxy_label_position(possible_positions + 0, x_tangent, y_tangent, offset_major, t);
        populate_galaxy_label_position(possible_positions + 1, x_tangent, y_tangent, offset_minor, t + M_PI / 2);
        populate_galaxy_label_position(possible_positions + 2, x_tangent, y_tangent, offset_major, t + M_PI);
        populate_galaxy_label_position(possible_positions + 3, x_tangent, y_tangent, offset_minor, t + 3 * M_PI / 2);
        *possible_position_count = 4;
    }
}

void draw_dark_nebula(chart_config *s, const double axis_pa, const double x_tangent, const double y_tangent,
                      const double x_canvas, const double y_canvas,
                      const double radius_major, const double radius_minor,
                      label_position *possible_positions, int *possible_position_count) {
    // Render object
    switch (s->dso_style) {
        case SW_DSO_STYLE_FUZZY:
            draw_dark_nebula_fuzzy(s, axis_pa, x_canvas, y_canvas, radius_major, radius_minor);
            break;
        case SW_DSO_STYLE_COLOURED:
            draw_dark_nebula_coloured(s, axis_pa, x_canvas, y_canvas, radius_major, radius_minor);
            break;
    }

    // Populate possible positions for the label for this object
    if (possible_positions != NULL) {
        const double offset_major = 0.8 * s->mm + radius_major;
        const double offset_minor = 0.8 * s->mm + radius_minor;
        const double t = axis_pa * M_PI / 180;

        populate_galaxy_label_position(possible_positions + 0, x_tangent, y_tangent, offset_major, t);
        populate_galaxy_label_position(possible_positions + 1, x_tangent, y_tangent, offset_minor, t + M_PI / 2);
        populate_galaxy_label_position(possible_positions + 2, x_tangent, y_tangent, offset_major, t + M_PI);
        populate_galaxy_label_position(possible_positions + 3, x_tangent, y_tangent, offset_minor, t + 3 * M_PI / 2);
        *possible_position_count = 4;
    }
}

void draw_generic_nebula(chart_config *s, const double x_tangent, const double y_tangent,
                         const double x_canvas, const double y_canvas, const double point_size,
                         label_position *possible_positions, int *possible_position_count) {
    // Render object
    switch (s->dso_style) {
        case SW_DSO_STYLE_FUZZY:
            draw_generic_nebula_fuzzy(s, x_canvas, y_canvas, point_size);
            break;
        case SW_DSO_STYLE_COLOURED:
            draw_generic_nebula_coloured(s, x_canvas, y_canvas, point_size);
            break;
    }

    // Populate possible positions for the label for this object
    if (possible_positions != NULL) {
        const double offset = 0.8 * s->mm + point_size;
        possible_positions[0] = (label_position) {x_tangent, y_tangent, 0, offset, 0, -1, 0};
        possible_positions[1] = (label_position) {x_tangent, y_tangent, 0, -offset, 0, 1, 0};
        possible_positions[2] = (label_position) {x_tangent, y_tangent, 0, 0, offset, 0, -1};
        possible_positions[3] = (label_position) {x_tangent, y_tangent, 0, 0, -offset, 0, 1};
        *possible_position_count = 4;
    }
}

// -------------- Plotting functions --------------

//! plot_deep_sky_objects - Draw deep sky objects onto a star chart
//! \param s - A <chart_config> structure defining the properties of the star chart to be drawn.
//! \param page - A <cairo_page> structure defining the cairo drawing context.
//! \param messier_only - Boolean flag indicating whether we're only displaying Messier objects

void plot_deep_sky_objects(chart_config *s, cairo_page *page, int messier_only) {
    // Path to where deep sky object catalogue is stored
    const char *dso_object_catalogue = s->dso_catalogue_file;

    // Open data dso_data_file listing the positions of the NGC and IC objects
    FILE *dso_data_file = fopen(dso_object_catalogue, "r");
    if (dso_data_file == NULL) {
        char line_buffer[LSTR_LENGTH];
        sprintf(line_buffer, "Could not open deep sky catalogue <%s>", dso_object_catalogue);
        stch_fatal(__FILE__, __LINE__, line_buffer);
        exit(1);
    }

    // Count the number of DSOs we have drawn and labelled, and make sure it doesn't exceed user-specified limits
    int dso_counter = 0;
    int label_counter = 0;

    // Loop over the lines of the data dso_data_file
    while ((!feof(dso_data_file)) && (!ferror(dso_data_file))) {
        char line[FNAME_LENGTH];
        const char *line_ptr = line;

        file_readline(dso_data_file, line);

        // Ignore comment lines
        if ((line[0] == '#') || (line[0] == '\n') || (line[0] == '\0')) continue;

        // Extract data from line of text
        while (*line_ptr == ' ') line_ptr++;
        int messier_num = (int) get_float(line_ptr, NULL);
        line_ptr = next_word(line_ptr);
        int ngc_num = (int) get_float(line_ptr, NULL);
        line_ptr = next_word(line_ptr);
        int ic_num = (int) get_float(line_ptr, NULL);
        line_ptr = next_word(line_ptr);
        double ra = get_float(line_ptr, NULL); // hours; J2000
        line_ptr = next_word(line_ptr);
        double dec = get_float(line_ptr, NULL); // degrees; J2000
        line_ptr = next_word(line_ptr);
        double mag = get_float(line_ptr, NULL); // magnitude
        line_ptr = next_word(line_ptr);
        double axis_major = get_float(line_ptr, NULL); // diameter; arcminutes
        line_ptr = next_word(line_ptr);
        double axis_minor = get_float(line_ptr, NULL); // diameter; arcminutes
        line_ptr = next_word(line_ptr);
        double axis_pa = get_float(line_ptr, NULL); // position angle; degrees

        // Read object type
        char type_string[16];
        line_ptr = next_word(line_ptr);
        get_word(type_string, line_ptr, 14);

        // Read object's custom label, if any
        char custom_object_label[FNAME_LENGTH];
        line_ptr = next_word(line_ptr);
        snprintf(custom_object_label, FNAME_LENGTH, "%s", line_ptr);
        const int have_custom_label = (custom_object_label[0] != '\0');

        // If we're only showing Messier objects; only show them
        if (messier_only && (messier_num == 0)) {
            continue;
        }

        // Too faint; include objects with no magnitudes given (recorded as 0) if mag cutoff > mag 50.
        if ((s->dso_mag_min < 50) && (s->dso_mag_min < mag)) {
            continue;
        }

        // Project RA and Dec of object into tangent-plane coordinates on the star chart (radians)
        double x_tangent, y_tangent;
        plane_project(&x_tangent, &y_tangent, s, ra * M_PI / 12, dec * M_PI / 180, 0);

        // Reject this object if it falls outside the plot area
        if ((!gsl_finite(x_tangent)) || (!gsl_finite(y_tangent))) {
            continue;
        }

        if ((x_tangent < s->x_min) || (x_tangent > s->x_max) || (y_tangent < s->y_min) || (y_tangent > s->y_max)) {
            continue;
        }

        // Check if we've exceeded maximum number of objects
        if (dso_counter > s->maximum_dso_count) continue;
        dso_counter++;

        // Create a name for this object
        char object_name[FNAME_LENGTH] = "";
        if (have_custom_label) {
            snprintf(object_name, FNAME_LENGTH, "%s", custom_object_label);
        } else if (messier_num > 0) {
            snprintf(object_name, FNAME_LENGTH, "M%d", messier_num);
        } else if (ngc_num > 0) {
            snprintf(object_name, FNAME_LENGTH, "NGC%d", ngc_num);
        } else if (ic_num > 0) {
            snprintf(object_name, FNAME_LENGTH, "IC%d", ic_num);
        }

        // Create a list of possible positions on the canvas where this label can go
        label_position possible_positions[6];
        int possible_position_count = 0;

        // Draw a symbol showing the position of this object
        double x_canvas, y_canvas; // Position, in Cairo pixels
        const double pt = s->dpi * 1. / 72; // 1 pt, in Cairo pixels
        const double point_size = 0.75 * 3 * pt * s->dso_point_size_scaling; // Cairo pixels
        double image_scale = s->width * s->cm / (s->x_max - s->x_min); // Cairo pixels per radian
        double arcminute = image_scale / (180 / M_PI * 60); // Cairo pixels per arcminute
        fetch_canvas_coordinates(&x_canvas, &y_canvas, x_tangent, y_tangent, s);

        // Test the type of this object
        const int is_open_cluster = (strncmp(type_string, "OC", 2) == 0);
        const int is_globular_cluster = (strncmp(type_string, "Gb", 2) == 0);
        const int is_planetary_nebula = (strncmp(type_string, "Pl", 2) == 0);
        const int is_galaxy_cluster = (strncmp(type_string, "GxC", 3) == 0);
        const int is_galaxy = (strncmp(type_string, "Gx", 2) == 0) && !is_galaxy_cluster;
        const int is_dark_nebula = (strncmp(type_string, "Dk", 2) == 0);

        // Draw the appropriate symbol for this object
        if (is_open_cluster) {
            // Draw an open cluster
            const double radius = gsl_max((axis_major + axis_minor) / 4 * arcminute, point_size * 1.2);
            draw_open_cluster(s, x_tangent, y_tangent, x_canvas, y_canvas, radius,
                              possible_positions, &possible_position_count);
        } else if (is_globular_cluster) {
            // Draw a globular cluster
            const double radius = gsl_max((axis_major + axis_minor) / 4 * arcminute, point_size * 1.2);
            draw_globular_cluster(s, x_tangent, y_tangent, x_canvas, y_canvas, radius,
                                  possible_positions, &possible_position_count);
        } else if (is_planetary_nebula) {
            // Draw a planetary nebula
            draw_planetary_nebula(s, x_tangent, y_tangent, x_canvas, y_canvas, point_size,
                                  possible_positions, &possible_position_count);
        } else if (is_galaxy || is_galaxy_cluster || is_dark_nebula) {
            // Draw an ellipsoidal object
            double aspect_ratio = gsl_max(axis_minor, 1e-6) / gsl_max(axis_major, 1e-6);
            if ((axis_major == 0) || (axis_minor == 0) || (!gsl_finite(aspect_ratio))) aspect_ratio = 1;
            if (aspect_ratio > 1) aspect_ratio = 1;
            if (aspect_ratio < 0.2) aspect_ratio = 0.2;

            double radius_major = gsl_max(axis_major, axis_minor) / 2 * arcminute;
            int size_unset = 0;

            if (radius_major < point_size) {
                radius_major = point_size;
                size_unset = 1;
            }

            const double radius_minor = radius_major * aspect_ratio;

            // Work out direction of north on the chart
            double x2, y2;
            plane_project(&x2, &y2, s, ra * M_PI / 12, (dec + 1e-3) * M_PI / 180, 0);

            // Check output is finite
            if ((!gsl_finite(x2)) || (!gsl_finite(y2))) {
                continue;
            }

            const double north_direction[2] = {x2 - x_tangent, y2 - y_tangent};
            const double north_theta = atan2(north_direction[0], north_direction[1]) * 180 / M_PI; // degrees

            // Start drawing
            if (is_galaxy) {
                draw_galaxy(s, axis_pa + north_theta, x_tangent, y_tangent, x_canvas, y_canvas,
                            radius_major, radius_minor, possible_positions, &possible_position_count);
            } else if (is_galaxy_cluster) {
                draw_galaxy_cluster(s, axis_pa + north_theta, x_tangent, y_tangent, x_canvas, y_canvas,
                                    radius_major, radius_minor, size_unset,
                                    possible_positions, &possible_position_count);
            } else {
                draw_dark_nebula(s, axis_pa + north_theta, x_tangent, y_tangent, x_canvas, y_canvas,
                                 radius_major, radius_minor, possible_positions, &possible_position_count);
            }
        } else {
            draw_generic_nebula(s, x_tangent, y_tangent, x_canvas, y_canvas, point_size,
                                possible_positions, &possible_position_count);
        }

        // Don't allow text labels to be placed over this deep sky object. Exclusion region is a point of size zero.
        {
            chart_add_label_exclusion(page, s,
                                      x_tangent, x_tangent, y_tangent, y_tangent);
        }

        // Consider whether to write a text label next to this deep sky object
        if ((mag < s->dso_label_mag_min) && (label_counter < s->maximum_dso_label_count)) {

            // Set priority for labelling this object
            double priority = mag;
            if (s->must_label_all_dsos) priority = -2;

            // Write a text label for this object
            const int show_name = s->dso_names;
            const int show_mag = s->dso_mags && (mag < 40);
            const int multiple_labels = show_name && show_mag;

            if (show_name) {
                chart_label_buffer(page, s, s->dso_label_col, object_name,
                                   possible_positions, possible_position_count,
                                   multiple_labels, 0, 1.2 * s->label_font_size_scaling,
                                   0, 0, 0, priority);
                label_counter++;
            }
            if (show_mag) {
                snprintf(temp_err_string, FNAME_LENGTH, "%.1f", mag);
                chart_label_buffer(page, s, s->dso_label_col, temp_err_string,
                                   possible_positions, possible_position_count,
                                   multiple_labels, 0, 1.2 * s->label_font_size_scaling,
                                   0, 0, 0, priority);
                label_counter++;
            }
        }
    }

    // Close data dso_data_file listing deep sky objects
    fclose(dso_data_file);

    // print debugging message
    if (DEBUG) {
        snprintf(temp_err_string, FNAME_LENGTH, "Displayed %d DSOs and %d DSO labels", dso_counter, label_counter);
        stch_log(temp_err_string);
    }
}

//! draw_dso_symbol_key - Draw a legend below the star chart indicating the symbols used to represent deep sky objects.
//! \param s - A <chart_config> structure defining the properties of the star chart to be drawn.
//! \param legend_y_pos - The vertical pixel position of the top of the next legend to go under the star chart.

double draw_dso_symbol_key(chart_config *s, const double legend_y_pos) {
    const double w_left = 0.4; // The left margin
    const double w_item = 3.7 * s->font_size; // The width of each legend item (cm)

    // Positions of legend items (set negative to hide)
    const double position_galaxy = 0;
    const double position_nebula = 0.7;
    const double position_open_cluster = 1.7;
    const double position_globular_cluster = 2.7;
    const double position_planetary_nebula = 3.7;
    const double position_dark_nebula = -1;

    // Number of items to show in legend
    const double N = 4.7;

    // The top (y0) and bottom (y1) of the legend
    const double y0 = legend_y_pos;
    const double y1 = y0 - 0.4;

    // The horizontal position of the centre of the legend
    const double x1 = s->canvas_offset_x + (s->width - s->legend_right_column_width) / 2;

    // The width of the legend
    const double xw = w_left * 1.5 + w_item * N;
    const double size = 0.4;

    // The left edge of the legend
    const double x = x1 - xw / 2;
    cairo_text_extents_t extents;

    cairo_set_font_size(s->cairo_draw, 3.6 * s->mm * s->font_size);
    cairo_set_line_width(s->cairo_draw, 2.5 * s->line_width_base);

    // Reset font weight
    cairo_select_font_face(s->cairo_draw, s->font_family, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);

    // Set point size for all legend entries
    double point_size = 0.2 * s->cm;

    // Draw legend item for galaxy
    if (position_galaxy >= 0) {
        const double x_this = x + w_left + w_item * position_galaxy;

        // Draw galaxy symbol
        draw_galaxy(s, 30, 0, 0, x_this * s->cm, y1 * s->cm,
                    point_size, point_size * 0.5, NULL, NULL);

        // Write a text label next to it
        {
            const char *label = "Galaxy";
            cairo_set_source_rgb(s->cairo_draw, 0, 0, 0);
            cairo_text_extents(s->cairo_draw, label, &extents);
            cairo_move_to(s->cairo_draw,
                          (x_this + 0.1 + size) * s->cm - extents.x_bearing,
                          y1 * s->cm - extents.height / 2 - extents.y_bearing
            );
            cairo_show_text(s->cairo_draw, label);
        }
    }

    // Draw legend item for generic nebula
    if (position_nebula >= 0) {
        const double x_this = x + w_left + w_item * position_nebula;

        // Draw generic nebula symbol
        draw_generic_nebula(s, 0, 0, x_this * s->cm, y1 * s->cm, point_size * 0.5,
                            NULL, NULL);

        // Write a text label next to it
        {
            const char *label = "Bright nebula";
            cairo_set_source_rgb(s->cairo_draw, 0, 0, 0);
            cairo_text_extents(s->cairo_draw, label, &extents);
            cairo_move_to(s->cairo_draw,
                          (x_this + 0.1 + size) * s->cm - extents.x_bearing,
                          y1 * s->cm - extents.height / 2 - extents.y_bearing
            );
            cairo_show_text(s->cairo_draw, label);
        }
    }

    // Draw legend item for open cluster
    if (position_open_cluster >= 0) {
        const double x_this = x + w_left + w_item * position_open_cluster;

        // Draw open cluster symbol
        draw_open_cluster(s, 0, 0, x_this * s->cm, y1 * s->cm, point_size,
                          NULL, NULL);

        // Write a text label next to it
        {
            const char *label = "Open cluster";
            cairo_set_source_rgb(s->cairo_draw, 0, 0, 0);
            cairo_text_extents(s->cairo_draw, label, &extents);
            cairo_move_to(s->cairo_draw,
                          (x_this + 0.1 + size) * s->cm - extents.x_bearing,
                          y1 * s->cm - extents.height / 2 - extents.y_bearing
            );
            cairo_show_text(s->cairo_draw, label);
        }
    }

    // Draw legend item for globular cluster
    if (position_globular_cluster >= 0) {
        const double x_this = x + w_left + w_item * position_globular_cluster;

        // Draw globular cluster symbol
        draw_globular_cluster(s, 0, 0, x_this * s->cm, y1 * s->cm, point_size,
                              NULL, NULL);

        // Write a text label next to it
        {
            const char *label = "Globular cluster";
            cairo_set_source_rgb(s->cairo_draw, 0, 0, 0);
            cairo_text_extents(s->cairo_draw, label, &extents);
            cairo_move_to(s->cairo_draw,
                          (x_this + 0.1 + size) * s->cm - extents.x_bearing,
                          y1 * s->cm - extents.height / 2 - extents.y_bearing
            );
            cairo_show_text(s->cairo_draw, label);
        }
    }

    // Draw legend item for planetary nebula
    if (position_planetary_nebula >= 0) {
        const double x_this = x + w_left + w_item * position_planetary_nebula;

        // Draw planetary nebula symbol
        draw_planetary_nebula(s, 0, 0, x_this * s->cm, y1 * s->cm, point_size * 0.6,
                              NULL, NULL);

        // Write a text label next to it
        {
            const char *label = "Planetary nebula";
            cairo_set_source_rgb(s->cairo_draw, 0, 0, 0);
            cairo_text_extents(s->cairo_draw, label, &extents);
            cairo_move_to(s->cairo_draw,
                          (x_this + 0.1 + size) * s->cm - extents.x_bearing,
                          y1 * s->cm - extents.height / 2 - extents.y_bearing
            );
            cairo_show_text(s->cairo_draw, label);
        }
    }

    // Draw legend item for dark nebula
    if (position_dark_nebula >= 0) {
        const double x_this = x + w_left + w_item * position_dark_nebula;

        // Draw dark nebula symbol
        draw_dark_nebula(s, 30, 0, 0, x_this * s->cm, y1 * s->cm,
                         point_size, point_size * 0.5, NULL, NULL);

        // Write a text label next to it
        {
            const char *label = "Dark nebula";
            cairo_set_source_rgb(s->cairo_draw, 0, 0, 0);
            cairo_text_extents(s->cairo_draw, label, &extents);
            cairo_move_to(s->cairo_draw,
                          (x_this + 0.1 + size) * s->cm - extents.x_bearing,
                          y1 * s->cm - extents.height / 2 - extents.y_bearing
            );
            cairo_show_text(s->cairo_draw, label);
        }
    }

    // Finally, return the vertical position for the next legend item below this one
    const double new_bottom_to_legend_items = y0 + 0.4;
    return new_bottom_to_legend_items;
}
