// galaxyMap.c
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
#include <stdint.h>
#include <string.h>
#include <math.h>

#include <cairo/cairo.h>

#include <gsl/gsl_math.h>

#include "listTools/ltMemory.h"

#include "astroGraphics/galaxyMap.h"
#include "coreUtils/asciiDouble.h"
#include "coreUtils/errorReport.h"
#include "mathsTools/projection.h"
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
    if (in == NULL) stch_fatal(__FILE__, __LINE__, "Could not open galaxy map datafile");
    dcf_fread((void *) &map_h_size, sizeof(int), 1, in);
    dcf_fread((void *) &map_v_size, sizeof(int), 1, in);
    galaxy_data = (unsigned char *) lt_malloc(map_h_size * map_v_size);
    if (galaxy_data == NULL) stch_fatal(__FILE__, __LINE__, "Malloc fail");
    dcf_fread((void *) galaxy_data, sizeof(char), map_h_size * map_v_size, in);
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

    // Render galaxy map into pixel array
#pragma omp parallel for shared(pixel_data) private(j)
    for (j = 0; j < height; j++) {
        int i;
        double y = (j / ((double) height) - 0.5) * s->aspect * s->wlin;
        for (i = 0; i < width; i++) {
            double x = (i / ((double) width) - 0.5) * s->wlin;
            double ra, dec;
            int c, x_map, y_map;
            // Work out where each pixel in the star chart maps to, in a rectangular grid of (RA, Dec)
            inv_plane_project(&ra, &dec, s, x, y);
            x_map = (int) (ra / (2 * M_PI) * map_h_size);
            y_map = (int) ((dec + M_PI / 2) / M_PI * map_v_size);
            if ((x_map < 0) || (x_map >= map_h_size) || (y_map < 0) || (y_map >= map_v_size)) {
                // If this pixel falls outside the galaxy map image we loaded, we set it to white.
                // Cairo's ARGB32 pixel format stores pixels as 32-bit ints.
                *(uint32_t *) (pixel_data + j * stride + 4 * i) = 0xffffffff;
            } else {
                // Read pixel from the galaxy map image we loaded
                c = galaxy_data[x_map + y_map * map_h_size];
                if (c > 254) c = 254;
                if (c < 1) c = 1;

                // Cairo's ARGB32 pixel format stores pixels as 32-bit ints, with alpha in most significant byte.
                *(uint32_t *) (pixel_data + j * stride + 4 * i) =
                        (uint32_t) (s->galaxy_col0.blu * (255 - c) + s->galaxy_col.blu * c) +  // blue
                        ((uint32_t) (s->galaxy_col0.grn * (255 - c) + s->galaxy_col.grn * c) << (unsigned) 8) +  // green
                        ((uint32_t) (s->galaxy_col0.red * (255 - c) + s->galaxy_col.red * c) << (unsigned) 16) + // red
                        ((uint32_t) 255 << (unsigned) 24);  // alpha
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
}
