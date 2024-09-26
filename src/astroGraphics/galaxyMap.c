// galaxyMap.c
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
#include <stdint.h>
#include <string.h>
#include <math.h>

#include <cairo/cairo.h>

#include <gsl/gsl_math.h>

#include "listTools/ltMemory.h"

#include "astroGraphics/galaxyMap.h"
#include "coreUtils/asciiDouble.h"
#include "coreUtils/errorReport.h"
#include "mathsTools/julianDate.h"
#include "mathsTools/projection.h"
#include "mathsTools/sphericalTrig.h"
#include "settings/chart_config.h"
#include "vectorGraphics/lineDraw.h"
#include "vectorGraphics/cairo_page.h"

//! The dimensions of the binary map of the Milky Way, as read from disk.
static int map_h_size, map_v_size;

//! Array into which we read the binary file defining the map of the Milky Way
static unsigned char *galaxy_data = NULL;

//! read_galaxy_map - Read the map of the Milky Way from binary file on disk into the array <galaxy_data>.
//! \param s - A <chart_config> structure defining the properties of the star chart to be drawn.

void read_galaxy_map(chart_config *s) {
    FILE *in;
    if (galaxy_data != NULL) return;

    in = fopen(s->galaxy_map_filename, "r");
    if (in == NULL) stch_fatal(__FILE__, __LINE__, "Could not open galaxy map data file");
    dcf_fread((void *) &map_h_size, sizeof(int), 1, in, s->galaxy_map_filename, __FILE__, __LINE__);
    dcf_fread((void *) &map_v_size, sizeof(int), 1, in, s->galaxy_map_filename, __FILE__, __LINE__);
    galaxy_data = (unsigned char *) lt_malloc(map_h_size * map_v_size);
    if (galaxy_data == NULL) stch_fatal(__FILE__, __LINE__, "Malloc fail");
    dcf_fread((void *) galaxy_data, sizeof(char), map_h_size * map_v_size, in, s->galaxy_map_filename,
              __FILE__, __LINE__);
    fclose(in);
}

//! plot_galaxy_map - Render a shaded map of the Milky Way into the background of this star chart
//! \param s - A <chart_config> structure defining the properties of the star chart to be drawn.

void plot_galaxy_map(chart_config *s) {
    int j;
    const int width = s->galaxy_map_width_pixels;
    const int height = (int) (s->galaxy_map_width_pixels * s->aspect);

    if (galaxy_data == NULL) read_galaxy_map(s);

    // Generate image RGB data
    const int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);

    // Allocate buffer for image data
    unsigned char *pixel_data = malloc(stride * height);

    // Fetch position of the Sun
    double ra_sun, dec_sun; // hours / degrees
    sun_pos(s->julian_date, &ra_sun, &dec_sun);

    // Render galaxy map into pixel array
#pragma omp parallel for shared(pixel_data, ra_sun, dec_sun) private(j)
    for (j = 0; j < height; j++) {
        int i;
        const double y = (j / ((double) height) - 0.5) * s->aspect * s->wlin;
        for (i = 0; i < width; i++) {
            const double x = (i / ((double) width) - 0.5) * s->wlin;
            double ra, dec; // radians
            // Work out where each pixel in the star chart maps to, in a rectangular grid of (RA, Dec)
            inv_plane_project(&ra, &dec, s, x, y);

            // Work out base colour to use for this pixel
            colour colour_base = s->galaxy_col0;

            // Project point onto galaxy map
            const int x_map = (int) (ra / (2 * M_PI) * map_h_size);
            const int y_map = (int) ((dec + M_PI / 2) / M_PI * map_v_size);
            if ((x_map < 0) || (x_map >= map_h_size) || (y_map < 0) || (y_map >= map_v_size)) {
                // If this pixel falls outside the galaxy map image we loaded, we set it to white.
                // Cairo's ARGB32 pixel format stores pixels as 32-bit ints.
                *(uint32_t *) (pixel_data + j * stride + 4 * i) = 0xffffffff;
                continue;
            }

            // If we're shading twilight, mix this into the base colour
            if (s->shade_twilight) {
                // Calculate scale height of twilight shading above the horizon
                double alt_scale_height = 20.;
                const double angular_height_degrees = (s->angular_width * 180 / M_PI) * s->aspect;
                alt_scale_height = gsl_min(alt_scale_height, angular_height_degrees * 0.5);

                // Convert (RA, Dec) to local (Alt, Az)
                double alt, az;
                double ra_at_epoch, dec_at_epoch;
                ra_dec_from_j2000(ra, dec, s->julian_date, &ra_at_epoch, &dec_at_epoch);
                alt_az(ra_at_epoch, dec_at_epoch, s->julian_date, s->horizon_latitude, s->horizon_longitude, &alt, &az);
                const double alt_degrees = alt * 180 / M_PI;
                const double alt_normalised = gsl_min(1, alt_degrees / alt_scale_height);

                if (alt_normalised >= 0) {
                    // Above the horizon
                    const double w1 = pow(alt_normalised, 0.5);
                    const double w2 = 1 - w1;

                    colour_base.red = s->twilight_horizon_col.red * w2 + s->twilight_zenith_col.red * w1;
                    colour_base.grn = s->twilight_horizon_col.grn * w2 + s->twilight_zenith_col.grn * w1;
                    colour_base.blu = s->twilight_horizon_col.blu * w2 + s->twilight_zenith_col.blu * w1;
                } else {
                    // Below the horizon, shade everything white
                    colour_base.red = 1;
                    colour_base.grn = 1;
                    colour_base.blu = 1;
                }
            }

            // If we're shading the sky near the Sun, mix this into the base colour
            if (s->shade_near_sun) {
                // Calculate angular distance from the Sun
                const double angular_separation = angDist_RADec(ra_sun * M_PI / 12, dec_sun * M_PI / 180, ra, dec);
                const double ang_degrees = angular_separation * 180 / M_PI;
                const double ang_normalised = gsl_max(0, gsl_min(1, ang_degrees / 15.));

                const double w1 = pow(ang_normalised, 0.5);
                const double w2 = 1 - w1;

                colour_base.red = gsl_max(colour_base.red,
                                          s->twilight_horizon_col.red * w2 + s->twilight_zenith_col.red * w1);
                colour_base.grn = gsl_max(colour_base.grn,
                                          s->twilight_horizon_col.grn * w2 + s->twilight_zenith_col.grn * w1);
                colour_base.blu = gsl_max(colour_base.blu,
                                          s->twilight_horizon_col.blu * w2 + s->twilight_zenith_col.blu * w1);
            }

            // If we're shading the region of sky that is not observable, mix this into the base colour
            if (s->shade_not_observable) {
                double best_alt_normalised = 0;

                for (int step = 0; step < 96; step++) {
                    // Unix time for this step
                    const double julian_date_this = s->julian_date + (step / 96. - 0.5);

                    // Fetch position of the zenith
                    double ra_zenith_at_epoch, dec_zenith_at_epoch; // radians
                    double ra_zenith_j2000, dec_zenith_j2000; // radians
                    get_zenith_position(s->horizon_latitude, s->horizon_longitude, julian_date_this,
                                        &ra_zenith_at_epoch, &dec_zenith_at_epoch);
                    ra_dec_to_j2000(ra_zenith_at_epoch, dec_zenith_at_epoch, julian_date_this,
                                    &ra_zenith_j2000, &dec_zenith_j2000);

                    // Calculate angular distance of zenith from the Sun
                    const double angular_separation = angDist_RADec(
                            ra_sun * M_PI / 12, dec_sun * M_PI / 180,
                            ra_zenith_j2000, dec_zenith_j2000);
                    const double solar_altitude = 90 - angular_separation * 180 / M_PI;

                    // Reject time steps which fall in twilight
                    if (solar_altitude > -4) continue;

                    // Convert (RA, Dec) to local (Alt, Az)
                    double alt, az;
                    double ra_at_epoch, dec_at_epoch;
                    ra_dec_from_j2000(ra, dec, julian_date_this, &ra_at_epoch, &dec_at_epoch);
                    alt_az(ra_at_epoch, dec_at_epoch, julian_date_this,
                           s->horizon_latitude, s->horizon_longitude, &alt, &az);
                    const double alt_degrees = alt * 180 / M_PI;
                    const double alt_normalised = gsl_max(0, gsl_min(1, alt_degrees / 20.));
                    if (alt_normalised > best_alt_normalised) best_alt_normalised = alt_normalised;
                }

                const double w1 = pow(best_alt_normalised, 0.5);
                const double w2 = 1 - w1;

                colour_base.red = gsl_max(colour_base.red,
                                          s->twilight_horizon_col.red * w2 + s->twilight_zenith_col.red * w1);
                colour_base.grn = gsl_max(colour_base.grn,
                                          s->twilight_horizon_col.grn * w2 + s->twilight_zenith_col.grn * w1);
                colour_base.blu = gsl_max(colour_base.blu,
                                          s->twilight_horizon_col.blu * w2 + s->twilight_zenith_col.blu * w1);
            }

            // Project point onto galaxy map
            colour colour_final = colour_base;
            if (s->plot_galaxy_map) {
                // Read pixel from the galaxy map image we loaded
                int c = galaxy_data[x_map + y_map * map_h_size];

                const double alt_normalised = gsl_max(0, gsl_min(0.999, c / 255.));

                const double w1 = alt_normalised;
                const double w2 = 1 - w1;

                // Compute final colour
                colour_final.red = colour_base.red * w2 + s->galaxy_col.red * w1;
                colour_final.grn = colour_base.grn * w2 + s->galaxy_col.grn * w1;
                colour_final.blu = colour_base.blu * w2 + s->galaxy_col.blu * w1;

                // If we're shading twilight, then mixing must be additive; galaxy cannot be darker than twilight
                if (s->shade_twilight) {
                    colour_final.red = gsl_max(colour_base.red, colour_final.red);
                    colour_final.grn = gsl_max(colour_base.grn, colour_final.grn);
                    colour_final.blu = gsl_max(colour_base.blu, colour_final.blu);
                }
            }

            // Cairo's ARGB32 pixel format stores pixels as 32-bit ints, with alpha in most significant byte.
            *(uint32_t *) (pixel_data + j * stride + 4 * i) =
                    (uint32_t) (255 * colour_final.blu) +  // blue
                    ((uint32_t) (255 * colour_final.grn) << (unsigned) 8) +  // green
                    ((uint32_t) (255 * colour_final.red) << (unsigned) 16) +  // red
                    ((uint32_t) 255 << (unsigned) 24);  // alpha
        }
    }

    // Create cairo surface containing image
    cairo_surface_t *surface = cairo_image_surface_create_for_data(pixel_data, CAIRO_FORMAT_ARGB32,
                                                                   width, height,
                                                                   stride);

    // We need to do some coordinate transformations in order to display the image at the right scale...
    cairo_save(s->cairo_draw);

    // Move our coordinate system so the top-left of the star chart is at (0,0), and the star chart's dimensions
    // match those of the image we're about to paint behind it
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
    free(pixel_data);
}
