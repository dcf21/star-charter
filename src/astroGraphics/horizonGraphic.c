// horizonGraphic.c
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
#include <stdint.h>
#include <string.h>

#include <cairo/cairo.h>

#include <gsl/gsl_math.h>

#include "coreUtils/asciiDouble.h"
#include "coreUtils/errorReport.h"
#include "mathsTools/julianDate.h"
#include "mathsTools/projection.h"
#include "png/image.h"
#include "settings/chart_config.h"
#include "vectorGraphics/cairo_page.h"


//! plot_horizon_graphic - Plot the horizon onto the sky.
//! \param s - A <chart_config> structure defining the properties of the star chart to be drawn.
//! \param page - A <cairo_page> structure defining the cairo drawing context.

void plot_horizon_graphic(chart_config *s, cairo_page *page) {
    // Temporarily disable horizon clipping
    const int show_horizon = s->show_horizon;
    s->show_horizon = 0;

    int j;
    const int width = s->galaxy_map_width_pixels;
    const int height = (int) (s->galaxy_map_width_pixels * s->aspect);

    // Generate image RGB data
    const int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);

    // Allocate buffer for image data
    unsigned char *pixel_data = malloc(stride * height);

    // Initialise a transparent pixel array
#pragma omp parallel for shared(pixel_data) private(j)
    for (j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            // Work out base colour to use for this pixel
            const colour colour_init = {0, 0, 0, 0};

            // Cairo's ARGB32 pixel format stores pixels as 32-bit ints, with alpha in most significant byte.
            *(uint32_t *) (pixel_data + j * stride + 4 * i) =
                    (uint32_t) (255 * colour_init.blu) + // blue
                    ((uint32_t) (255 * colour_init.grn) << (unsigned) 8) + // green
                    ((uint32_t) (255 * colour_init.red) << (unsigned) 16) + // red
                    ((uint32_t) (255 * colour_init.alpha) << (unsigned) 24); // alpha
        }
    }

    // Overlay each horizon graphic in turn
    for (int k = 0; k < s->horizon_graphics_count; k++) {
        // Parse horizon graphics specification string
        const char *in_scan = s->horizon_graphics[k];
        char image_filename[FNAME_LENGTH];
        double azimuth_central = 0, angular_width = 360, y_position_horizon, angular_height;
        str_comma_separated_list_scan(&in_scan, image_filename);
        image_ptr input_image = image_get(image_filename);
        if (*in_scan != '\0') {
            char buffer[FNAME_LENGTH], msg[FNAME_LENGTH];
            str_comma_separated_list_scan(&in_scan, buffer);
            int l = (int) strlen(buffer);
            if (!valid_float(buffer, &l)) {
                snprintf(msg, FNAME_LENGTH,
                         "Bad 'horizon_graphic' config. Item '%s' should be float (azimuth_central).", buffer);
                stch_error(msg);
            }
            azimuth_central = get_float(buffer, NULL);
        }
        if (*in_scan != '\0') {
            char buffer[FNAME_LENGTH], msg[FNAME_LENGTH];
            str_comma_separated_list_scan(&in_scan, buffer);
            int l = (int) strlen(buffer);
            if (!valid_float(buffer, &l)) {
                snprintf(msg, FNAME_LENGTH,
                         "Bad 'horizon_graphic' config. Item '%s' should be float (angular_width).", buffer);
                stch_error(msg);
            }
            angular_width = get_float(buffer, NULL);
        }
        if (*in_scan != '\0') {
            char buffer[FNAME_LENGTH], msg[FNAME_LENGTH];
            str_comma_separated_list_scan(&in_scan, buffer);
            int l = (int) strlen(buffer);
            if (!valid_float(buffer, &l)) {
                snprintf(msg, FNAME_LENGTH,
                         "Bad 'horizon_graphic' config. Item '%s' should be float (y_position_horizon).", buffer);
                stch_error(msg);
            }
            y_position_horizon = get_float(buffer, NULL);
        } else {
            y_position_horizon = input_image.ysize / 2;
        }
        if (*in_scan != '\0') {
            char buffer[FNAME_LENGTH], msg[FNAME_LENGTH];
            str_comma_separated_list_scan(&in_scan, buffer);
            int l = (int) strlen(buffer);
            if (!valid_float(buffer, &l)) {
                snprintf(msg, FNAME_LENGTH,
                         "Bad 'horizon_graphic' config. Item '%s' should be float (angular_height).", buffer);
                stch_error(msg);
            }
            angular_height = get_float(buffer, NULL);
        } else {
            angular_height = angular_width * ((float) (input_image.ysize)) / ((float) (input_image.xsize));
        }

        // Render input image onto pixel array
#pragma omp parallel for shared(pixel_data) private(j)
        for (j = 0; j < height; j++) {
            const double y = (j / ((double) height) - 0.5) * s->aspect * s->wlin;
            for (int i = 0; i < width; i++) {
                const double x = (i / ((double) width) - 0.5) * s->wlin;
                double ra, dec; // radians

                // Work out where each pixel in the star chart maps to, in a rectangular grid of (RA, Dec)
                inv_plane_project(&ra, &dec, s, x, y);

                // Convert (RA, Dec) to local (Alt, Az)
                double alt, az;
                double ra_at_epoch, dec_at_epoch;
                ra_dec_from_j2000(ra, dec, s->julian_date, &ra_at_epoch, &dec_at_epoch);
                alt_az(ra_at_epoch, dec_at_epoch, s->julian_date, s->horizon_latitude, s->horizon_longitude, &alt, &az);
                const double alt_degrees = alt * 180 / M_PI;
                const double az_degrees = az * 180 / M_PI;

                // Work out coordinates of input pixel
                double az_relative = fmod(az_degrees - azimuth_central, 360.);
                while (az_relative > 180) az_relative -= 360;
                while (az_relative < -180) az_relative += 360;

                const int y_in = (int) (y_position_horizon - alt_degrees * input_image.ysize / angular_height);
                const int x_in = (int) (input_image.xsize / 2. + az_relative * input_image.xsize / angular_width);

                // Work out base colour to use for this pixel
                colour colour_new = {0, 0, 0, 0};

                if ((x_in >= 0) && (x_in < input_image.xsize) && (y_in >= 0) && (y_in < input_image.ysize)) {
                    const int offset = x_in + y_in * input_image.xsize;
                    colour_new.red = input_image.data_red[offset] / 255.;
                    colour_new.grn = input_image.data_grn[offset] / 255.;
                    colour_new.blu = input_image.data_blu[offset] / 255.;
                    colour_new.alpha = input_image.data_alpha[offset] / 255.;
                }

                // Blend new colour with old colour
                const uint32_t colour_uint32 = *(uint32_t *) (pixel_data + j * stride + 4 * i);
                const colour colour_previous = {
                    ((float) ((colour_uint32 >> (unsigned) 16) & 255)) / 255.,
                    ((float) ((colour_uint32 >> (unsigned) 8) & 255)) / 255.,
                    ((float) ((colour_uint32) & 255)) / 255.,
                    ((float) ((colour_uint32 >> (unsigned) 24) & 255)) / 255.,
                };

                const colour colour_blended = {
                    (colour_new.red * colour_new.alpha + colour_previous.red * (1 - colour_new.alpha)),
                    (colour_new.grn * colour_new.alpha + colour_previous.grn * (1 - colour_new.alpha)),
                    (colour_new.blu * colour_new.alpha + colour_previous.blu * (1 - colour_new.alpha)),
                    1. - (1. - colour_previous.alpha) * (1. - colour_new.alpha)
                };

                // Cairo's ARGB32 pixel format stores pixels as 32-bit ints, with alpha in most significant byte.
                *(uint32_t *) (pixel_data + j * stride + 4 * i) =
                        (uint32_t) (255 * colour_blended.blu) + // blue
                        ((uint32_t) (255 * colour_blended.grn) << (unsigned) 8) + // green
                        ((uint32_t) (255 * colour_blended.red) << (unsigned) 16) + // red
                        ((uint32_t) (255 * colour_blended.alpha) << (unsigned) 24); // alpha
            }
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

    // Re-enable horizon clipping
    s->show_horizon = show_horizon;
}
