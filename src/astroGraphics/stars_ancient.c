// stars_ancient.c
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

// This file implements ornate star graphics, based on Johann Bayer's 1603 star atlas, Uranometria.

#include <math.h>

#include <gsl/gsl_math.h>

#include "astroGraphics/stars_ancient.h"

// Data structure used to hold settings for the star renderer
typedef struct {
    cairo_t *cr; // Cairo drawing context used for all drawing operations.

    double x0; // Centre X coordinate of the star (Cairo pixels).
    double y0; // Centre Y coordinate of the star (Cairo pixels).

    double ps; // Base point size used to derive all radii and stroke lengths.

    double r_star_outer, r_star_inner; // Inner and outer radii for the main six-point star (Cairo pixels).
    double r_shading_outer, r_shading_inner; // Inner and outer radii for the narrow outer spikes (Cairo pixels).

    double thin_lw; // Thin stroke width used for outlines and fine detail.
    double bold_lw; // Bold stroke width used for the main star outline.
} StarContext;

// Generic shader used to render the outer halo of shading around the stars
typedef void (*ShadeFn)(const StarContext *, double theta);

//! init_star_context - Initialize data structure used to hold settings for the star renderer
//! @param [out] ctx - Star drawing context to populate.
//! @param [in] cr - Cairo drawing context.
//! @param [in] x0 - X coordinate of the centre of the star (Cairo pixels).
//! @param [in] y0 - Y coordinate of the centre of the star (Cairo pixels).
//! @param [in] ps - Point size.
//! @param [in] r_shading_outer - Outer radius for the halo of shading around the star (Cairo pixels).
static void init_star_context(StarContext *const ctx, cairo_t *const cr, const double x0, const double y0,
                              const double ps, const double r_shading_outer) {
    ctx->cr = cr;
    ctx->x0 = x0;
    ctx->y0 = y0;
    ctx->ps = ps;
    ctx->r_star_outer = ps; // Outer vertices of the main star
    ctx->r_star_inner = ps / 3.0; // Inner vertices of the main star
    ctx->r_shading_outer = r_shading_outer; // Outer edge of the narrow outer spikes
    ctx->r_shading_inner = ps / 6.0; // Inner radius of the base of the narrow outer spikes
    ctx->bold_lw = (ps / 50.0) * 1.5;
    ctx->thin_lw = ctx->bold_lw / 3.0;
}

//! cartesian - Convert polar coordinates to Cartesian coordinates.
//! @param [in] x0 - X coordinate of the centre of the star (Cairo pixels).
//! @param [in] y0 - Y coordinate of the centre of the star (Cairo pixels).
//! @param [in] r - Radius (Cairo pixels).
//! @param [in] theta_deg - Azimuth (degrees).
//! @param [out] x - Output X coordinate (Cairo pixels).
//! @param [out] y - Output Y coordinate (Cairo pixels).
static void cartesian(const double x0, const double y0, const double r, const double theta_deg,
                      double *const x, double *const y) {
    const double t = theta_deg * M_PI / 180.0;

    *x = x0 + r * cos(t);
    *y = y0 + r * sin(t);
}

//! outer_spike_path - Create a path for one side of one of the narrow outer spikes around the star.
//! @param [in] ctx - Drawing context.
//! @param [in] theta - Base azimuth for the segment we are drawing (degrees).
//! @param [in] r_inner - Inner radius (Cairo pixels).
//! @param [in] r_outer - Outer radius (Cairo pixels).
static void outer_spike_path(StarContext *const ctx, const double theta, const double r_inner, const double r_outer) {
    double x;
    double y;

    cartesian(ctx->x0, ctx->y0, r_inner, theta + 30.0, &x, &y);
    cairo_line_to(ctx->cr, x, y);

    cartesian(ctx->x0, ctx->y0, r_outer, theta + 60.0, &x, &y);
    cairo_line_to(ctx->cr, x, y);
}

//! shade_outer_sector - Perform the shading around the outside of the star, for a single sector.
//! @param [in] ctx - Drawing context.
//! @param [in] theta - Base azimuth for the segment we are drawing (degrees).
//! @param [in] sign - Boolean toggle to select which side of the sector has the outer spike.
//! @param [in] shade - Shading function.
static void shade_outer_sector(StarContext *const ctx, const double theta, const int sign, ShadeFn const shade) {
    cairo_t *const cr = ctx->cr;

    // First clip to the outer sector, to ensure that the shading does not overlap the star itself
    cairo_save(cr);
    cairo_new_path(cr);
    {
        double x, y;

        if (sign == 0) {
            const double theta0 = theta - 90.;
            const double theta1 = theta - 60.;
            const double theta2 = theta - 30.;

            cartesian(ctx->x0, ctx->y0, ctx->r_shading_outer, theta1, &x, &y);
            cairo_move_to(cr, x, y);
            outer_spike_path(ctx, theta0, ctx->r_shading_inner, ctx->r_shading_outer);
            cairo_arc_negative(cr, ctx->x0, ctx->y0, ctx->r_shading_outer, theta2 * M_PI / 180.0,
                               theta1 * M_PI / 180.0);
        } else {
            const double theta0 = theta - 60.;
            const double theta1 = theta - 30.;
            const double theta2 = theta;

            cartesian(ctx->x0, ctx->y0, ctx->r_shading_outer, theta2, &x, &y);
            cairo_move_to(cr, x, y);
            cairo_arc_negative(cr, ctx->x0, ctx->y0, ctx->r_shading_outer, theta2 * M_PI / 180.0,
                               theta1 * M_PI / 180.0);
            outer_spike_path(ctx, theta0, ctx->r_shading_outer, ctx->r_shading_inner);
        }
        cairo_close_path(cr);
        cairo_clip(cr);
    }

    // Perform the shading
    cairo_new_path(cr);
    const double theta_shade = theta + (sign ? 0 : -30);
    shade(ctx, theta_shade);

    // Release clipping mask
    cairo_restore(cr);
}

//! draw_outer_sector - Draw one of the outer sectors of the star, comprising the outer spikes and shading between them.
//! @param [in] ctx - Drawing context.
//! @param [in] theta - Base azimuth for the segment we are drawing (degrees).
//! @param [in] shade - Shading function.
static void draw_outer_sector(StarContext *const ctx, const double theta, ShadeFn const shade) {
    cairo_t *const cr = ctx->cr;

    // First clip to the outer sector, to ensure that the shading does not overlap the star itself
    cairo_save(cr);
    cairo_new_path(cr);

    {
        double x, y;

        cartesian(ctx->x0, ctx->y0, ctx->r_star_outer, theta - 60.0, &x, &y);
        cairo_move_to(cr, x, y);
        outer_spike_path(ctx, theta - 60.0, ctx->r_star_inner, ctx->r_star_outer); // Trace the edge of the main star
        cartesian(ctx->x0, ctx->y0, ctx->r_shading_outer, theta, &x, &y);
        cairo_line_to(cr, x, y);
        cairo_arc_negative(cr, ctx->x0, ctx->y0, ctx->r_shading_outer, theta * M_PI / 180.0,
                           (theta - 60.0) * M_PI / 180.0);
        cairo_close_path(cr);
    }

    // Set clipping mask
    cairo_clip(cr);

    {
        // Draw the outer spike
        double x, y;

        cairo_new_path(cr);
        cartesian(ctx->x0, ctx->y0, ctx->r_shading_inner, theta - 60.0, &x, &y);
        cairo_move_to(cr, x, y);
        outer_spike_path(ctx, theta - 60.0, ctx->r_shading_outer, ctx->r_shading_inner);
        cairo_close_path(cr);
        cairo_stroke(cr);
    }

    // Add radial lines shading the region between the spikes
    shade_outer_sector(ctx, theta, 0, shade);
    shade_outer_sector(ctx, theta, 1, shade);

    // Release clipping mask
    cairo_restore(cr);
}

//! draw_outer_decoration - Draw the outer spikes and shading around the inner star, looping around six sectors.
//! @param [in] ctx - Star drawing context.
//! @param [in] angle - Base azimuth for the first segment we draw (degrees).
//! @param [in] shade - Shading function.
static void draw_outer_decoration(StarContext *const ctx, const double angle, ShadeFn const shade) {
    for (int i = 0; i < 6; ++i) {
        draw_outer_sector(ctx, angle + i * 60.0, shade);
    }
}

//! radial_line - Draw a single radial line segment. This is used many times to create a shaded effect
//! @param [in] ctx - Drawing context.
//! @param [in] inner - Starting radius (Cairo pixels).
//! @param [in] length - Length of the line segment (Cairo pixels).
//! @param [in] theta - Azimuth (degrees).
static void radial_line(const StarContext *ctx, const double inner, const double length, const double theta) {
    double x1, y1, x2, y2;

    // Inner edge of radial line
    cartesian(ctx->x0, ctx->y0, inner, theta, &x1, &y1);

    // Outer edge of radial line
    cartesian(ctx->x0, ctx->y0, inner + length, theta, &x2, &y2);

    // Draw line
    cairo_move_to(ctx->cr, x1, y1);
    cairo_line_to(ctx->cr, x2, y2);
    cairo_stroke(ctx->cr);
}

//! star_point - Append one single vertex of a star to the current path.
//! @param [in] cr - Cairo drawing context.
//! @param [in] x0 - X coordinate of the centre (Cairo pixels).
//! @param [in] y0 - Y coordinate of the centre (Cairo pixels).
//! @param [in] r_inner - Inner radius (Cairo pixels).
//! @param [in] r_outer - Outer radius (Cairo pixels).
//! @param [in] theta - Base azimuth for the vertex (degrees).
static void star_point(cairo_t *const cr, const double x0, const double y0,
                       const double r_inner, const double r_outer, const double theta) {
    double x, y;

    cartesian(x0, y0, r_inner, theta + 30.0, &x, &y);
    cairo_line_to(cr, x, y);

    cartesian(x0, y0, r_outer, theta + 60.0, &x, &y);
    cairo_line_to(cr, x, y);
}

//! star - Build a path describing a six-pointed star.
//! @param [in] cr - Cairo drawing context.
//! @param [in] x0 - X coordinate of the centre (Cairo pixels).
//! @param [in] y0 - Y coordinate of the centre (Cairo pixels).
//! @param [in] r_inner - Inner radius (Cairo pixels).
//! @param [in] r_outer - Outer radius (Cairo pixels).
//! @param [in] theta - Starting azimuth (degrees).
static void star(cairo_t *const cr, const double x0, const double y0,
                 const double r_inner, const double r_outer, const double theta) {
    double x, y;

    // Starting point for the star
    cartesian(x0, y0, r_outer, theta, &x, &y);
    cairo_move_to(cr, x, y);

    // Loop around the six vertices of the star
    for (int i = 0; i < 6; i++)
        star_point(cr, x0, y0, r_inner, r_outer, theta + i * 60.0);

    // Close the path around the star
    cairo_close_path(cr);
}

//! q_shade - Render the outer shading pattern for Q-Orionis.
//! @param [in] ctx - Drawing context.
//! @param [in] theta - Reference azimuth for the outer shading (degrees).
static void q_shade(const StarContext *ctx, const double theta) {
    const double theta_base = theta - 30.0;
    const double theta_step = 3.0;

    for (int i = 0; i < 10; i++) {
        const double azimuth = theta_base + i * theta_step;

        radial_line(ctx, ctx->ps / 3, ctx->ps / 4, azimuth);
        radial_line(ctx, ctx->ps * 0.75, ctx->ps / 10, azimuth);
        radial_line(ctx, ctx->ps, ctx->ps / 10, azimuth);
    }
}

//! alpha_shade - Render the outer shading pattern for Alpha-Orionis.
//! @param [in] ctx - Drawing context.
//! @param [in] theta - Reference azimuth for the outer shading (degrees).
static void alpha_shade(const StarContext *ctx, const double theta) {
    // Reuse the common Q-Orionis shading pattern
    q_shade(ctx, theta);

    // The extra Alpha-Orionis shading starts 30 degrees before the current reference angle.
    const double theta_base = theta - 30.0;
    const double theta_step = 2.0;

    for (int i = 0; i < 15; i++) {
        double azimuth = theta_base + i * theta_step;

        // Draw a short radial line beginning at radius 1.25*ps with length ps/10.
        radial_line(ctx, ctx->ps * 1.25, ctx->ps / 10.0, azimuth);
    }
}

//! beta_shade - Render the outer shading pattern for Beta-Orionis.
//! @param [in] ctx - Drawing context.
//! @param [in] theta - Reference azimuth for the outer shading (degrees).
static void beta_shade(const StarContext *ctx, const double theta) {
    // Reuse the common Q-Orionis shading pattern
    q_shade(ctx, theta);

    // The extra Alpha-Orionis shading starts 30 degrees before the current reference angle.
    const double theta_base = theta - 30.0;
    const double theta_step = 2.0;

    for (int i = 0; i < 7; i++) {
        // Draw a short radial line beginning at radius 1.25*ps with length ps/10.
        const double azimuth = theta_base + i * theta_step;
        radial_line(ctx, ctx->ps * 1.25, ctx->ps / 10.0, azimuth);
    }

    {
        // Central longer stroke
        const double azimuth = theta_base + 14.0;
        radial_line(ctx, ctx->ps * 1.25 - ctx->ps / 10.0, 3.0 * (ctx->ps / 10.0), azimuth);
    }

    // Seven further short strokes
    for (int i = 0; i < 7; i++) {
        const double azimuth = theta_base + 16.0 + i * theta_step;
        radial_line(ctx, ctx->ps * 1.25, ctx->ps / 10.0, azimuth);
    }
}

//! gamma_shade - Render the outer shading pattern for Gamma-Orionis.
//! @param [in] ctx - Drawing context.
//! @param [in] theta - Reference azimuth for the outer shading (degrees).
static void gamma_shade(const StarContext *ctx, const double theta) {
    // First pass: Ten pairs of radial strokes spaced 3 degrees apart.
    const double theta_base = theta - 30.0;

    for (int i = 0; i < 10; ++i) {
        const double theta1_step = 3.0;
        const double azimuth = theta_base + i * theta1_step;
        // Inner hatch
        radial_line(ctx, ctx->ps / 3.0, ctx->ps / 4.0, azimuth);
        // Outer hatch
        radial_line(ctx, 1.25 * ctx->ps, ctx->ps / 10.0, azimuth);
    }

    // Second pass: Seven middle strokes spanning the same 30-degree sector.
    const double theta_mid = theta - 30.0;
    const double theta2_step = 30.0 / 7.0;

    // First three short strokes
    for (int i = 0; i < 3; ++i) {
        const double azimuth = theta_mid + i * theta2_step;
        radial_line(ctx, 0.95 * ctx->ps, ctx->ps / 8.0, azimuth);
    }

    {
        // Central long stroke
        const double azimuth = theta_mid + 3.0 * theta2_step;
        radial_line(ctx, 0.95 * ctx->ps - ctx->ps / 10.0, 3.0 * (ctx->ps / 10.0), azimuth);
    }

    // Final three short strokes
    for (int i = 0; i < 3; ++i) {
        const double azimuth = theta_mid + (4.0 + i) * theta2_step;
        radial_line(ctx, 0.95 * ctx->ps, ctx->ps / 8.0, azimuth);
    }
}

//! epsilon_shade - Render the outer shading pattern for Epsilon-Orionis.
//! @param [in] ctx - Drawing context.
//! @param [in] theta - Reference azimuth for the outer shading (degrees).
static void epsilon_shade(const StarContext *ctx, const double theta) {
    // Begin 30 degrees before the current reference angle.
    const double theta_base = theta - 30.0;
    // The nine strokes are evenly spaced over the 30° sector.
    const double theta_step = 10.0 / 3.0;

    // First four short strokes.
    for (int i = 0; i < 4; ++i) {
        const double azimuth = theta_base + i * theta_step;
        radial_line(ctx, ctx->ps, ctx->ps / 8.0, azimuth);
    }

    // Central long stroke
    {
        const double azimuth = theta_base + 4.0 * theta_step;
        radial_line(ctx, ctx->ps - ctx->ps / 8.0, 3.0 * (ctx->ps / 8.0), azimuth);
    }

    // Final four short strokes.
    for (int i = 0; i < 4; ++i) {
        const double azimuth = theta_base + (5.0 + i) * theta_step;
        radial_line(ctx, ctx->ps, ctx->ps / 8.0, azimuth);
    }
}

//! m_shade - Render the outer shading pattern for M-Orionis.
//! @param [in] ctx - Drawing context.
//! @param [in] theta - Reference azimuth for the outer shading (degrees).
static void m_shade(const StarContext *ctx, const double theta) {
    // Begin 30 degrees before the current reference angle.
    const double theta_base = theta - 30.0;

    // Six strokes at 5° spacing.
    for (int i = 0; i < 6; ++i) {
        const double azimuth = theta_base + i * 5.0;
        radial_line(ctx, ctx->ps / 3.0, ctx->ps / 2.0, azimuth);
    }
}

//! tau_shade - Render the outer shading pattern for Tau-Orionis.
//! @param [in] ctx - Drawing context.
//! @param [in] theta - Reference azimuth for the outer shading (degrees).
static void tau_shade(const StarContext *ctx, const double theta) {
    // Begin 30 degrees before the current reference angle.
    const double theta_base = theta - 30.0;

    // Five pairs of hatch lines at 6° spacing.
    for (int i = 0; i < 5; ++i) {
        const double azimuth = theta_base + i * 6.0;
        // Inner hatch line
        radial_line(ctx, ctx->ps / 3.0, ctx->ps / 4.0, azimuth);
        // Outer hatch line
        radial_line(ctx, ctx->ps, ctx->ps / 4.0, azimuth);
    }
}

//! draw_star_type1 - Draw star type 1 - ALPHA ORIONIS
//! @param [in] cr - Cairo drawing context.
//! @param [in] x0 - X coordinate of the centre (Cairo pixels).
//! @param [in] y0 - Y coordinate of the centre (Cairo pixels).
//! @param [in] ps - Point size (Cairo pixels).
//! @param [in] azimuth - Reference azimuth (degrees).
void draw_star_type1(cairo_t *const cr, const double x0, const double y0, const double ps, const double azimuth) {
    StarContext ctx;
    init_star_context(&ctx, cr, x0, y0, ps, ps * 2.0);

    // Main six-point star outline.
    cairo_set_line_width(cr, ctx.bold_lw);
    star(cr, x0, y0, ctx.r_star_inner, ctx.r_star_outer, azimuth + 90.0);
    cairo_stroke(cr);

    // Draw the outer spikes and shading around the main star
    cairo_set_line_width(cr, ctx.thin_lw);
    draw_outer_decoration(&ctx, azimuth + 90.0, alpha_shade);

    // Internal shading inside the main star body.
    for (int i = 0; i < 6; ++i) {
        // Work on one point of the star at a time
        const double theta = azimuth + 90.0 + i * 60.0;

        // Clip to one half of one of the points of the main star
        cairo_save(cr);
        cairo_new_path(cr);
        {
            double x, y;
            cartesian(x0, y0, ps, theta, &x, &y);
            cairo_move_to(cr, x, y);
            cairo_line_to(cr, x0, y0);
            cartesian(x0, y0, ps / 3.0, theta + 30.0, &x, &y);
            cairo_line_to(cr, x, y);
            cairo_close_path(cr);
        }
        cairo_clip(cr);

        // Fill the clipped main-point wedge with repeated radial strokes.
        for (int j = 0; j < 8; ++j) {
            cartesian(x0, y0, j * ps / 30, theta + 60., &ctx.x0, &ctx.y0);
            radial_line(&ctx, 0, ps, theta);
        }
        cairo_restore(cr);
        ctx.x0 = x0;
        ctx.y0 = y0;
    }

    // Three spokes across the main star.
    cairo_set_line_width(cr, ctx.thin_lw);
    for (int i = 0; i < 3; ++i) {
        const double theta = azimuth + 120.0 + i * 60.0;
        double x1, y1, x2, y2;
        cartesian(x0, y0, ps / 3.0, theta, &x1, &y1);
        cartesian(x0, y0, -(ps / 3.0), theta, &x2, &y2);
        cairo_move_to(cr, x1, y1);
        cairo_line_to(cr, x2, y2);
        cairo_stroke(cr);
    }
}

//! draw_star_type2 - Draw star type 2 - BETA ORIONIS
//! @param [in] cr - Cairo drawing context.
//! @param [in] x0 - X coordinate of the centre (Cairo pixels).
//! @param [in] y0 - Y coordinate of the centre (Cairo pixels).
//! @param [in] ps - Point size (Cairo pixels).
//! @param [in] azimuth - Reference azimuth (degrees).
void draw_star_type2(cairo_t *const cr, const double x0, const double y0, const double ps, const double azimuth) {
    StarContext ctx;
    init_star_context(&ctx, cr, x0, y0, ps, 1.5 * ps);

    // Main six-point star outline.
    cairo_set_line_width(cr, ctx.bold_lw);
    star(cr, x0, y0, ctx.r_star_inner, ctx.r_star_outer, azimuth + 90.0);
    cairo_stroke(cr);

    // Draw the outer spikes and shading around the main star
    cairo_set_line_width(cr, ctx.thin_lw);
    draw_outer_decoration(&ctx, azimuth + 90.0, beta_shade);

    // Internal shading inside the main star body.
    for (int i = 0; i < 6; ++i) {
        // Work on one point of the star at a time
        const double theta = azimuth + 90.0 + i * 60.0;

        // Clip to one half of one of the points of the main star
        cairo_save(cr);
        cairo_new_path(cr);
        {
            double x, y;
            cartesian(x0, y0, ctx.r_star_outer, theta, &x, &y);
            cairo_move_to(cr, x, y);
            cairo_line_to(cr, x0, y0);
            cartesian(x0, y0, ctx.r_star_inner, theta + 30.0, &x, &y);
            cairo_line_to(cr, x, y);
            cairo_close_path(cr);
        }
        cairo_clip(cr);

        // Fill the clipped main-point wedge with denser radial strokes.
        for (int j = 0; j < 10; ++j) {
            cartesian(x0, y0, j * ps / 40, theta + 60., &ctx.x0, &ctx.y0);
            radial_line(&ctx, 0, ps, theta);
        }
        cairo_restore(cr);
        ctx.x0 = x0;
        ctx.y0 = y0;
    }

    // Three alternating spokes across the main star.
    cairo_set_line_width(cr, ctx.thin_lw);
    for (int i = 0; i < 3; ++i) {
        const double theta = azimuth + 90.0 + i * 60.0;
        radial_line(&ctx, 1.5 * ps, -3.0 * ps, theta);
        radial_line(&ctx, ps / 3.0, -2.0 * (ps / 3.0), theta + 30.0);
    }
}

//! draw_star_type3 - Draw star type 3 - EPSILON ORIONIS
//! @param [in] cr - Cairo drawing context.
//! @param [in] x0 - X coordinate of the centre (Cairo pixels).
//! @param [in] y0 - Y coordinate of the centre (Cairo pixels).
//! @param [in] ps - Point size (Cairo pixels).
//! @param [in] azimuth - Reference azimuth (degrees).
void draw_star_type3(cairo_t *const cr, const double x0, const double y0, const double ps, const double azimuth) {
    StarContext ctx;
    init_star_context(&ctx, cr, x0, y0, ps, 1.5 * ps);

    // Main six-point star outline.
    cairo_set_line_width(cr, ctx.bold_lw);
    star(cr, x0, y0, ctx.r_star_inner, ctx.r_star_outer, azimuth + 90.0);
    cairo_stroke(cr);

    // Draw the outer spikes and shading around the main star
    cairo_set_line_width(cr, ctx.thin_lw);
    draw_outer_decoration(&ctx, azimuth + 90.0, epsilon_shade);

    // Internal shading inside the main star body.
    for (int i = 0; i < 6; ++i) {
        // Work on one point of the star at a time
        const double theta = azimuth + 90.0 + i * 60.0;

        // Clip to one half of one of the points of the main star
        cairo_save(cr);
        cairo_new_path(cr);
        {
            double x, y;
            cartesian(x0, y0, ctx.r_star_outer, theta, &x, &y);
            cairo_move_to(cr, x, y);
            cairo_line_to(cr, x0, y0);
            cartesian(x0, y0, ctx.r_star_inner, theta + 30.0, &x, &y);
            cairo_line_to(cr, x, y);
            cairo_close_path(cr);
        }
        cairo_clip(cr);

        // Fill the clipped main-point wedge with repeated radial strokes.
        for (int j = 0; j < 8; ++j) {
            cartesian(x0, y0, j * ps / 30, theta + 60., &ctx.x0, &ctx.y0);
            radial_line(&ctx, 0, ps, theta);
        }
        cairo_restore(cr);
        ctx.x0 = x0;
        ctx.y0 = y0;
    }

    // Three long spokes and three shorter spokes across the main star.
    cairo_set_line_width(cr, ctx.thin_lw);
    for (int i = 0; i < 3; ++i) {
        const double theta = azimuth + 90.0 + i * 60.0;
        radial_line(&ctx, ps + ps / 4.0, -2.0 * (ps + ps / 4.0), theta);
        radial_line(&ctx, ps / 3.0, -2.0 * (ps / 3.0), theta + 30.0);
    }
}

//! draw_star_type4 - Draw star type 4 - Q ORIONIS
//! @param [in] cr - Cairo drawing context.
//! @param [in] x0 - X coordinate of the centre (Cairo pixels).
//! @param [in] y0 - Y coordinate of the centre (Cairo pixels).
//! @param [in] ps - Point size (Cairo pixels).
//! @param [in] azimuth - Reference azimuth (degrees).
void draw_star_type4(cairo_t *const cr, const double x0, const double y0, const double ps, const double azimuth) {
    StarContext ctx;
    init_star_context(&ctx, cr, x0, y0, ps, 1.5 * ps);

    // Main six-point star outline.
    cairo_set_line_width(cr, ctx.thin_lw);
    cairo_save(cr);
    star(cr, x0, y0, ctx.r_star_inner, ctx.r_star_outer, azimuth + 90.0);
    cairo_stroke_preserve(cr);

    // Broad cross-hatched fill across the centre of the star.
    cairo_clip(cr);

    for (int i = -16; i < 16; ++i) {
        const double theta = azimuth + 90.0;
        const double x1 = x0 + 1.5 * ps * cos((theta + 90.0) * M_PI / 180.0) -
                          i * (ps / 15.0) * cos(theta * M_PI / 180.0);
        const double y1 = y0 + 1.5 * ps * sin((theta + 90.0) * M_PI / 180.0) -
                          i * (ps / 15.0) * sin(theta * M_PI / 180.0);
        const double x2 = x0 + 1.5 * ps * cos((theta - 90.0) * M_PI / 180.0) -
                          i * (ps / 15.0) * cos(theta * M_PI / 180.0);
        const double y2 = y0 + 1.5 * ps * sin((theta - 90.0) * M_PI / 180.0) -
                          i * (ps / 15.0) * sin(theta * M_PI / 180.0);

        cairo_new_path(cr);
        cairo_move_to(cr, x1, y1);
        cairo_line_to(cr, x2, y2);
        cairo_stroke(cr);
    }
    cairo_restore(cr);

    // Three bold spokes from each inner vertex to the corresponding outer tip.
    cairo_set_line_width(cr, ctx.bold_lw);
    for (int side = 0; side < 2; ++side) {
        const double delta = side == 0 ? 30.0 : -30.0;

        for (int i = 0; i < 3; ++i) {
            const double theta = azimuth + 90.0 + i * 60.0;
            double x1, y1, x2, y2;
            cartesian(x0, y0, ctx.r_star_inner, theta + delta, &x1, &y1);
            cartesian(x0, y0, ctx.r_star_outer, theta, &x2, &y2);
            cairo_move_to(cr, x1, y1);
            cairo_line_to(cr, x2, y2);
            cairo_stroke(cr);
        }
    }

    // Draw the outer spikes and shading around the main star
    cairo_set_line_width(cr, ctx.thin_lw);
    draw_outer_decoration(&ctx, azimuth + 90.0, q_shade);

    // Three spokes across the main star.
    cairo_set_line_width(cr, ctx.thin_lw);
    for (int i = 0; i < 3; ++i) {
        const double theta = azimuth + 90.0 + i * 60.0;
        radial_line(&ctx, ctx.r_star_inner, -2.0 * ctx.r_star_inner, theta + 30.0);
        radial_line(&ctx, ps * 1.25, -2.5 * ps, theta);
    }
}

//! draw_star_type5 - Draw star type 5 - M ORIONIS
//! @param [in] cr - Cairo drawing context.
//! @param [in] x0 - X coordinate of the centre (Cairo pixels).
//! @param [in] y0 - Y coordinate of the centre (Cairo pixels).
//! @param [in] ps - Point size (Cairo pixels).
//! @param [in] azimuth - Reference azimuth (degrees).
void draw_star_type5(cairo_t *const cr, const double x0, const double y0, const double ps, const double azimuth) {
    StarContext ctx;
    init_star_context(&ctx, cr, x0, y0, ps, ps * 1.5);

    // Main six-point star outline.
    cairo_set_line_width(cr, ctx.bold_lw);
    star(cr, x0, y0, ctx.r_star_inner, ctx.r_star_outer, azimuth + 90.0);
    cairo_stroke(cr);

    // Draw the outer spikes and shading around the main star
    cairo_set_line_width(cr, ctx.thin_lw);
    draw_outer_decoration(&ctx, azimuth + 90.0, m_shade);

    // Three central spokes across the main star.
    cairo_set_line_width(cr, ctx.thin_lw);
    for (int i = 0; i < 3; ++i) {
        double x1, y1, x2, y2;
        const double theta = azimuth + 120.0 + i * 60.0;
        cartesian(x0, y0, ps / 3.0, theta, &x1, &y1);
        cartesian(x0, y0, -(ps / 3.0), theta, &x2, &y2);
        cairo_move_to(cr, x1, y1);
        cairo_line_to(cr, x2, y2);
        cairo_stroke(cr);
    }
}

//! draw_star_type6 - Draw star type 6 - TAU ORIONIS
//! @param [in] cr - Cairo drawing context.
//! @param [in] x0 - X coordinate of the centre (Cairo pixels).
//! @param [in] y0 - Y coordinate of the centre (Cairo pixels).
//! @param [in] ps - Point size (Cairo pixels).
//! @param [in] azimuth - Reference azimuth (degrees).
void draw_star_type6(cairo_t *const cr, const double x0, const double y0, const double ps, const double azimuth) {
    StarContext ctx;
    init_star_context(&ctx, cr, x0, y0, ps, ps * 1.5);

    // Main six-point star outline.
    cairo_set_line_width(cr, ctx.bold_lw);
    star(cr, x0, y0, ctx.r_star_inner, ctx.r_star_outer, azimuth + 90.0);
    cairo_stroke(cr);

    // Draw the outer spikes and shading around the main star
    cairo_set_line_width(cr, ctx.thin_lw);
    draw_outer_decoration(&ctx, azimuth + 90.0, tau_shade);

    // Three central spokes across the main star.
    cairo_set_line_width(cr, ctx.thin_lw);
    for (int i = 0; i < 3; ++i) {
        double x1, y1, x2, y2;
        const double theta = azimuth + 120.0 + i * 60.0;
        cartesian(x0, y0, ps / 3.0, theta, &x1, &y1);
        cartesian(x0, y0, -(ps / 3.0), theta, &x2, &y2);
        cairo_move_to(cr, x1, y1);
        cairo_line_to(cr, x2, y2);
        cairo_stroke(cr);
    }
}

//! draw_star_type7 - Draw star type 7 - PI ORIONIS
//! @param [in] cr - Cairo drawing context.
//! @param [in] x0 - X coordinate of the centre (Cairo pixels).
//! @param [in] y0 - Y coordinate of the centre (Cairo pixels).
//! @param [in] ps - Point size (Cairo pixels).
//! @param [in] azimuth - Reference azimuth (degrees).
void draw_star_type7(cairo_t *const cr, const double x0, const double y0, const double ps, const double azimuth) {
    // Thin outer spikes, then reuse the Tau Orionis core.
    cairo_set_line_width(cr, (ps / 50.0) * 1.5 / 3.0);
    for (int i = 0; i < 6; ++i) {
        const double theta = azimuth + 90.0 + i * 60.0;
        double x1, y1, x2, y2;
        cartesian(x0, y0, ps, theta, &x1, &y1);
        cartesian(x0, y0, ps * 1.5, theta, &x2, &y2);
        cairo_move_to(cr, x1, y1);
        cairo_line_to(cr, x2, y2);
        cairo_close_path(cr);
        cairo_stroke(cr);
    }

    draw_star_type6(cr, x0, y0, ps, azimuth);
}

//! draw_star_type8 - Draw star type 8 - GAMMA ORIONIS
//! @param [in] cr - Cairo drawing context.
//! @param [in] x0 - X coordinate of the centre (Cairo pixels).
//! @param [in] y0 - Y coordinate of the centre (Cairo pixels).
//! @param [in] ps - Point size (Cairo pixels).
//! @param [in] azimuth - Reference azimuth (degrees).
void draw_star_type8(cairo_t *const cr, const double x0, const double y0, const double ps, const double azimuth) {
    StarContext ctx;
    init_star_context(&ctx, cr, x0, y0, ps, 1.5 * ps);

    const double r_star_outer = ps;
    const double r_star_inner = ps / 3.0;

    // Main six-point star outline.
    cairo_set_line_width(cr, ctx.bold_lw);
    star(cr, x0, y0, r_star_inner, r_star_outer, azimuth + 90.0);
    cairo_stroke(cr);

    // Draw the outer spikes and shading around the main star
    cairo_set_line_width(cr, ctx.thin_lw);
    draw_outer_decoration(&ctx, azimuth + 90.0, gamma_shade);

    // Three spokes across the main star.
    cairo_set_line_width(cr, ctx.thin_lw);
    for (int i = 0; i < 3; ++i) {
        radial_line(&ctx, ps / 3.0, -2.0 * (ps / 3.0), azimuth + 120.0 + i * 60.0);
        radial_line(&ctx, ps * 1.25, -2.0 * (ps * 1.25), azimuth + 90.0 + i * 60.0);
    }
}

//! draw_star_type9 - Draw star type 9 - GENERIC STAR
//! @param [in] cr - Cairo drawing context.
//! @param [in] x0 - X coordinate of the centre (Cairo pixels).
//! @param [in] y0 - Y coordinate of the centre (Cairo pixels).
//! @param [in] ps - Point size (Cairo pixels).
//! @param [in] azimuth - Reference azimuth (degrees).
void draw_star_type9(cairo_t *const cr, const double x0, const double y0, const double ps, const double azimuth) {
    const double inner_radius = ps / 4.;
    const double outer_radius = ps;

    cairo_new_path(cr);
    {
        // Start at the top outer tip of the five-point star.
        const double rad = 126.0 * M_PI / 180.0;
        const double x = x0 + outer_radius * cos(rad);
        const double y = y0 + outer_radius * sin(rad);
        cairo_move_to(cr, x, y);
    }

    // Walk around the star by alternating inner and outer vertices.
    for (int i = 0; i < 5; ++i) {
        // Inner vertex of the current arm.
        const double theta = 162.0 + i * 72.0;
        const double rad = theta * M_PI / 180.0;
        const double x = x0 + inner_radius * cos(rad);
        const double y = y0 + inner_radius * sin(rad);
        cairo_line_to(cr, x, y);

        // Outer tip of the next arm.
        const double rad2 = (theta + 36.0) * M_PI / 180.0;
        const double x2 = x0 + outer_radius * cos(rad2);
        const double y2 = y0 + outer_radius * sin(rad2);
        cairo_line_to(cr, x2, y2);
    }

    cairo_close_path(cr);
    cairo_fill(cr);
}

typedef void (*AncientStarRenderer)(cairo_t *cr, double x0, double y0, double ps, double angle);

//! draw_ancient_star - Draw a star, based on Johann Bayer's 1603 star atlas, Uranometria.
//! @param [in] cr - Cairo drawing context.
//! @param [in] x0 - X coordinate of the centre (Cairo pixels).
//! @param [in] y0 - Y coordinate of the centre (Cairo pixels).
//! @param [in] mag - The magnitude of the star.
//! @param [in] ps - Point size (Cairo pixels).
void draw_ancient_star(cairo_t *const cr, const double x0, const double y0, double mag, const double ps) {
    // Reference V-band magnitudes for each ancient-style star renderer, based on the star it was taken from.
    static const double reference_magnitudes[] = {0.18, 0.45, 1.64, 1.69, 2.07, 2.25, 3.39, 4.50, 6.00};
    static const AncientStarRenderer renderers[] = {
        draw_star_type2, draw_star_type1, draw_star_type8, draw_star_type3,
        draw_star_type6, draw_star_type5, draw_star_type7, draw_star_type4,
        draw_star_type9
    };
    const int n_renderers = (int) (sizeof(reference_magnitudes) / sizeof(reference_magnitudes[0]));

    // Assume a faint magnitude for stars with no magnitude
    if (!gsl_finite(mag)) mag = 20;

    // Select which renderer to use
    int best_index = 0;
    double best_distance = fabs(mag - reference_magnitudes[0]);

    for (int i = 1; i < n_renderers; ++i) {
        const double distance = fabs(mag - reference_magnitudes[i]);
        if (distance < best_distance) {
            best_distance = distance;
            best_index = i;
        }
    }

    // Render the star
    renderers[best_index](cr, x0, y0, ps, -45.);
}
