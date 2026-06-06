// image_in.c
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
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <gsl/gsl_math.h>
#include <png.h>

#include "coreUtils/errorReport.h"

#include "image.h"

// Colour channel configurations
#define BMP_COLOUR_PALETTE    1002
#define BMP_COLOUR_GREY       1003
#define BMP_COLOUR_GREY_ALPHA 1004
#define BMP_COLOUR_RGB        1005
#define BMP_COLOUR_RGB_ALPHA  1006

//! image_alloc - allocate an image_ptr structure to hold an image of dimensions (x, y)
//! \param [out] out The image_ptr structure to populate
//! \param [in] x The horizontal size of the image
//! \param [in] y The vertical size of the image

void image_alloc(image_ptr *out, int x, int y) {
    const int j = x * y;

    out->xsize = x;
    out->ysize = y;
    out->data_w = (double *) malloc(x * y * sizeof(double));
    out->data_red = (double *) malloc(x * y * sizeof(double));
    out->data_grn = (double *) malloc(x * y * sizeof(double));
    out->data_blu = (double *) malloc(x * y * sizeof(double));
    out->data_alpha = (double *) malloc(x * y * sizeof(double));
    for (int i = 0; i < j; i++) out->data_red[i] = 0.0;
    for (int i = 0; i < j; i++) out->data_grn[i] = 0.0;
    for (int i = 0; i < j; i++) out->data_blu[i] = 0.0;
    for (int i = 0; i < j; i++) out->data_alpha[i] = 0.0;
    for (int i = 0; i < j; i++) out->data_w[i] = 0.0;
}

//! image_dealloc - free the storage associated with an image_ptr structure
//! \param [in] in The image_ptr structure to free

void image_dealloc(image_ptr *in) {
    if (in->data_red != NULL) free(in->data_red);
    if (in->data_grn != NULL) free(in->data_grn);
    if (in->data_blu != NULL) free(in->data_blu);
    if (in->data_alpha != NULL) free(in->data_alpha);
    if (in->data_w != NULL) free(in->data_w);

    in->data_red = NULL;
    in->data_grn = NULL;
    in->data_blu = NULL;
    in->data_alpha = NULL;
    in->data_w = NULL;
}

//! image_cp - copy the image contained within an image_ptr structure
//! \param [in] in The image to copy
//! \param [out] out The image_ptr structure to populate with the copied image

void image_cp(image_ptr *in, image_ptr *out) {
    image_alloc(out, in->xsize, in->ysize);
    memcpy(out->data_red, in->data_red, in->xsize * in->ysize * sizeof(double));
    memcpy(out->data_grn, in->data_grn, in->xsize * in->ysize * sizeof(double));
    memcpy(out->data_blu, in->data_blu, in->xsize * in->ysize * sizeof(double));
    memcpy(out->data_alpha, in->data_alpha, in->xsize * in->ysize * sizeof(double));
    memcpy(out->data_w, in->data_w, in->xsize * in->ysize * sizeof(double));
}

//! image_deweight - Divide the pixel data in an image by the weight field. This is useful if N images have been
//! co-added into the pixel data, and the <data_w> structure member contains the number of images which have been
//! added together. The resulting image is then properly normalised.
//! \param out The image to deweight

void image_deweight(image_ptr *out) {
    int i, j = out->xsize * out->ysize;
    for (i = 0; i < j; i++) {
        out->data_red[i] /= out->data_w[i];
        if (!gsl_finite(out->data_red[i])) out->data_red[i] = 0.0;
    }
    for (i = 0; i < j; i++) {
        out->data_grn[i] /= out->data_w[i];
        if (!gsl_finite(out->data_grn[i])) out->data_grn[i] = 0.0;
    }
    for (i = 0; i < j; i++) {
        out->data_blu[i] /= out->data_w[i];
        if (!gsl_finite(out->data_blu[i])) out->data_blu[i] = 0.0;
    }
    for (i = 0; i < j; i++) {
        out->data_alpha[i] /= out->data_w[i];
        if (!gsl_finite(out->data_alpha[i])) out->data_alpha[i] = 0.0;
    }
}

//! image_get - Read a PNG image from disk and convert it into an image_ptr structure
//! \param [in] filename The filename of the PNG image to read
//! \return image_ptr structure containing the pixel data

image_ptr image_get(const char *filename) {
    // Output structure to hold the image that we're about to read
    image_ptr output;
    output.xsize = output.ysize = -1;
    output.data_red = output.data_grn = output.data_blu = output.data_alpha = output.data_w = NULL;

    // Open the PNG image file
    FILE *infile;

    if ((infile = fopen(filename, "rb")) == NULL) {
        fprintf(stderr, "ERROR: Cannot open input file %s\n", filename);
        return output;
    }

    // Read the contents of the image using libpng
    unsigned char *data = NULL, *trans = NULL;
    uint16_t *palette = NULL;
    int pal_len = 0, width = 0, height = 0, colour = 0;

    int depth = 0, ncols = 0, ntrans = 0, png_colour_type = 0, i = 0, j = 0;
    unsigned row_bytes = 0;
    static unsigned char index[3];

    png_structp png_ptr;
    png_infop info_ptr;
    png_bytep *row_ptrs, trans_colours;
    png_color_16p trans_val;
    png_colorp pngpalette;

    // Initialise libpng data structures
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL) {
        stch_error("Out of memory");
        goto finalise;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL) {
        stch_error("Out of memory");
        goto finalise;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        stch_error("Unexpected error in libpng while trying to decode PNG image file");
        goto finalise;
    }

    png_init_io(png_ptr, infile);
    png_set_sig_bytes(png_ptr, 0);

    // Let libpng read header, and deal with outcome
    png_read_info(png_ptr, info_ptr);

    width = png_get_image_width(png_ptr, info_ptr);
    height = png_get_image_height(png_ptr, info_ptr);
    depth = png_get_bit_depth(png_ptr, info_ptr);
    png_colour_type = png_get_color_type(png_ptr, info_ptr);

    if (DEBUG) {
        sprintf(temp_err_string, "Size %dx%d", width, height);
        stch_log(temp_err_string);
    }
    if (DEBUG) {
        sprintf(temp_err_string, "Depth %d", depth);
        stch_log(temp_err_string);
    }

    if (png_colour_type & PNG_COLOR_MASK_ALPHA) {
        if (DEBUG) stch_log("PNG uses transparency");
    }

    if (png_colour_type == PNG_COLOR_TYPE_RGB) colour = BMP_COLOUR_RGB;
    if (png_colour_type == PNG_COLOR_TYPE_RGB_ALPHA) colour = BMP_COLOUR_RGB_ALPHA;
    if (png_colour_type == PNG_COLOR_TYPE_GRAY) colour = BMP_COLOUR_GREY;
    if (png_colour_type == PNG_COLOR_TYPE_GRAY_ALPHA) colour = BMP_COLOUR_GREY_ALPHA;

    if (png_colour_type == PNG_COLOR_TYPE_PALETTE) {
        colour = BMP_COLOUR_PALETTE;
        i = png_get_PLTE(png_ptr, info_ptr, &pngpalette, &ncols);
        if (i == 0) {
            stch_error("PNG image file claims to be paletted, but no palette was found");
            goto finalise;
        }
        pal_len = ncols;
        palette = (uint16_t *) malloc(ncols * 3 * sizeof(uint16_t));
        if (palette == NULL) {
            stch_error("Out of memory");
            goto finalise;
        }
        for (i = 0; i < ncols; i++) {
            *(palette + 3 * i) = pngpalette[i].red;
            *(palette + 3 * i + 1) = pngpalette[i].green;
            *(palette + 3 * i + 2) = pngpalette[i].blue;
        }
        if (DEBUG) {
            sprintf(temp_err_string, "PNG image file contains a palette of %d colours", ncols);
            stch_log(temp_err_string);
        }
    }

    // Update png info to reflect any requested conversions (e.g. 16 bit to 8 bit or alpha to non-alpha)
    png_read_update_info(png_ptr, info_ptr);

    // Now rowbytes will reflect what we will get, not what we had originally
    row_bytes = png_get_rowbytes(png_ptr, info_ptr);

    // Allocate block of memory for uncompressed image
    data = malloc(row_bytes * height);
    if (data == NULL) {
        stch_error("Out of memory");
        goto finalise;
    }

    // libpng requires a separate pointer to each row of image
    row_ptrs = (png_bytep *) malloc(height * sizeof(png_bytep));
    if (row_ptrs == NULL) {
        stch_error("Out of memory");
        data = NULL;
        goto finalise;
    }

    for (i = 0; i < height; i++) {
        row_ptrs[i] = data + row_bytes * i;
    }

    // Get uncompressed image
    png_read_image(png_ptr, row_ptrs);

    // Free everything we don't need
    png_read_end(png_ptr, NULL);

    // Deal with transparency (we can only support images with single transparent palette entries)
    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
        if (DEBUG) stch_log("PNG has transparency");
        png_get_tRNS(png_ptr, info_ptr, &trans_colours, &ntrans, &trans_val);
        if (DEBUG) {
            sprintf(temp_err_string, "PNG has %d transparent entries in palette", ntrans);
            stch_log(temp_err_string);
        }
        if (png_colour_type == PNG_COLOR_TYPE_PALETTE) {
            // We can cope with just one, fully transparent, entry in palette
            j = 0;
            for (i = 0; i < ntrans; i++)
                if (trans_colours[i] == 0) j++;
                else if (trans_colours[i] != 255) j += 10;
            if (j != 1) {
                stch_warning(
                    "PNG has transparency, but not in the form of a single fully colour in its palette. Such transparency is not supported.");
            } else {
                for (i = 0; (i < ntrans) && (trans_colours[i] == 255); i++);
                trans = index;
                *trans = i;
            }
        } else {
            trans = index;
            *trans = trans_val->gray;
            if (colour == BMP_COLOUR_RGB) {
                trans[0] = trans_val->red;
                trans[1] = trans_val->green;
                trans[2] = trans_val->blue;
            }
        }
    }
    png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);

    // Put all necessary information into the output data structure
    image_alloc(&output, width, height);
    const int frame_size = width * height;
    for (i = 0; i < frame_size; i++) output.data_w[i] = 1;

    if (colour == BMP_COLOUR_RGB) {
        for (i = 0; i < frame_size; i++) {
            if (depth == 8) {
                output.data_red[i] = data[3 * i + 0];
                output.data_grn[i] = data[3 * i + 1];
                output.data_blu[i] = data[3 * i + 2];
                output.data_alpha[i] = 255;
            } else {
                output.data_red[i] = 256 * data[6 * i + 0] + data[6 * i + 1];
                output.data_grn[i] = 256 * data[6 * i + 2] + data[6 * i + 3];
                output.data_blu[i] = 256 * data[6 * i + 4] + data[6 * i + 5];
                output.data_alpha[i] = 255;
            }
        }
    } else if (colour == BMP_COLOUR_RGB_ALPHA) {
        for (i = 0; i < frame_size; i++) {
            if (depth == 8) {
                output.data_red[i] = data[4 * i + 0];
                output.data_grn[i] = data[4 * i + 1];
                output.data_blu[i] = data[4 * i + 2];
                output.data_alpha[i] = data[4 * i + 3];
            } else {
                output.data_red[i] = 256 * data[8 * i + 0] + data[8 * i + 1];
                output.data_grn[i] = 256 * data[8 * i + 2] + data[8 * i + 3];
                output.data_blu[i] = 256 * data[8 * i + 4] + data[8 * i + 5];
                output.data_alpha[i] = 256 * data[8 * i + 6] + data[8 * i + 7];
            }
        }
    } else if (colour == BMP_COLOUR_PALETTE) {
        for (i = 0; i < frame_size; i++) {
            int j = data[i];
            if (j > pal_len - 1) j = pal_len - 1;
            output.data_red[i] = palette[3 * j + 0];
            output.data_grn[i] = palette[3 * j + 1];
            output.data_blu[i] = palette[3 * j + 2];
            output.data_alpha[i] = 255;
        }
    } else if (colour == BMP_COLOUR_GREY) {
        for (i = 0; i < frame_size; i++) {
            if (depth == 8) {
                output.data_red[i] = data[i];
                output.data_grn[i] = data[i];
                output.data_blu[i] = data[i];
            } else {
                output.data_red[i] = 256 * data[2 * i] + data[2 * i + 1];
                output.data_grn[i] = output.data_red[i];
                output.data_blu[i] = output.data_red[i];
            }
        }
    } else {
        for (i = 0; i < frame_size; i++) {
            if (depth == 8) {
                output.data_red[i] = data[2 * i];
                output.data_grn[i] = data[2 * i];
                output.data_blu[i] = data[2 * i];
                output.data_alpha[i] = data[2 * i + 1];
            } else {
                output.data_red[i] = 256 * data[4 * i] + data[4 * i + 1];
                output.data_grn[i] = output.data_red[i];
                output.data_blu[i] = output.data_red[i];
                output.data_alpha[i] = 256 * data[4 * i + 2] + data[4 * i + 3];
            }
        }
    }

finalise:
    if (data != NULL) free(data);
    if (palette != NULL) free(palette);
    if (trans != NULL) free(trans);
    if (row_ptrs != NULL) free(row_ptrs);
    return output;
}
