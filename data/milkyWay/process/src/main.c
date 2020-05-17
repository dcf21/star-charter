// main.c
// 
// -------------------------------------------------
// Copyright 2015-2020 Dominic Ford
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

// We process Axel Mellinger's photograph of the Milky Way to produce some shading which we use in the
// background of star charts to indicate the Galaxy. We do this by blurring the image and brightening
// it, and reprojecting it from galactic coordinates into RA and Dec.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <gsl/gsl_errno.h>
#include <gsl/gsl_math.h>

#include "coreUtils/errorReport.h"

#include "main.h"
#include "image.h"

//! galactic_project - Project a point from RA, Dec (equatorial coordinates) into galactic coordinates
//! \param [in] ra The right ascension of the point to project (radians)
//! \param [in] dec The declination of the point to project (radians)
//! \param [out] l_out The galactic longitude of the point (radians)
//! \param [out] b_out The galactic latitude of the point (radians)

void galactic_project(double ra, double dec, double *l_out, double *b_out) {
    // On Binney & Merrifield pp30-31 this reads 123.932. We use value to match Mellinger.
    double l_cp = 123. * M_PI / 180;

    double ra_gp = 192.85948 * M_PI / 180;
    double dec_gp = 27.12825 * M_PI / 180;

    // See pp 30-31 of Binney & Merrifield
    double b = asin(sin(dec) * sin(dec_gp) + cos(dec_gp) * cos(dec) * cos(ra - ra_gp));
    double lsin = cos(dec) * sin(ra - ra_gp) / cos(b);
    double lcos = (cos(dec_gp) * sin(dec) - sin(dec_gp) * cos(dec) * cos(ra - ra_gp)) / cos(b);
    double l = l_cp - atan2(lsin, lcos);

    // Clamp output values within range
    while (b < -M_PI) b += 2 * M_PI;
    while (b > M_PI) b -= 2 * M_PI;
    while (l < -M_PI) l += 2 * M_PI;
    while (l > M_PI) l -= 2 * M_PI;
    *l_out = l;
    *b_out = b; // Fudge things and proceed to project (l,b) onto plane as if they were (ra,dec)
}

int main() {
    int i, j;
    dataArray *d1;
    FILE *out;

    if (DEBUG) galmap_log("Initialising galaxy map processor.");

    // Turn off GSL's automatic error handler
    gsl_set_error_handler_off();

    // Median smooth image input JPEG photograph of the whole sky
    char command[1024];
    sprintf(command,
            "convert " SRCDIR "../../mwpan2_Merc_2000x1200.jpg -scale 8000 -median %d /tmp/mellinger.png",
            SMOOTH);
    if (system(command)) galmap_fatal(__FILE__, __LINE__, "ImageMagick fail");

    // Load input JPEG photograph of the whole sky
    image_ptr InputImage;

    // Read image into an image_ptr structure
    InputImage = image_get("/tmp/mellinger.png");
    if (InputImage.data_red == NULL) galmap_fatal(__FILE__, __LINE__, "Could not read input image file");

    // Allocate memory for the processed output version of the image
    d1 = (dataArray *) malloc(sizeof(dataArray));
    if (d1 == NULL) galmap_fatal(__FILE__, __LINE__, "Malloc fail");
    for (i = 0; i < V_SIZE_MAP1; i++) for (j = 0; j < H_SIZE_MAP1; j++) d1->C[i][j] = 0;

    // Parameter used to invert Mercator projection
    double R = InputImage.xsize / (2 * M_PI);

    // Array used to generate brightness histogram
    int histogram[256];
    for (i = 0; i < 256; i++) histogram[i] = 0;

    // Generate galaxy map for use by StarCharter
    // We do this by inverting the Mercator projection of Axel Mellinger's Milky Way panorama
    // Loop over all the pixels in the output map
    for (i = 0; i < V_SIZE_MAP1; i++)
        for (j = 0; j < H_SIZE_MAP1; j++) {
            // Work out the RA and Dec of this point in the output map
            double longitude = j * (2 * M_PI) / H_SIZE_MAP1; // RA 0 at left; RA 2pi at right
            double latitude = i * M_PI / V_SIZE_MAP1 - M_PI / 2; // Dec 0 in middle

            // Convert RA and Dec to galactic coordinates
            double l, b;
            galactic_project(longitude, latitude, &l, &b);

            // Project onto a Mercator projection to get a pixel coordinate in Axel Mellinger's image
            // longitude inverted here, because it runs from right to left
            int x = (int) (R * (-l) + InputImage.xsize / 2);
            int y = (int) (R * log(tan(M_PI / 4 + (-b) / 2)) + InputImage.ysize / 2);

            // Mellinger's image cuts off the galactic poles; set non-existant pixels to zero
            if (fabs(b) > 50 * M_PI / 180) continue; // Impose a cut-off at Galactic latitude of 60 deg
            if ((y < 0) || (y >= InputImage.ysize)) continue;

            // Average the three colour channels of input image to get greyscale brightness
            int offset = y * InputImage.xsize + x;
            double val = (InputImage.data_red[offset] + InputImage.data_grn[offset] + InputImage.data_blu[offset]) / 3;

            // Transfer function to increase brightness of image
            val = (val - 50) * 3;

            // Clip output brightness onto a 0-255 scale
            if (val < 0) val = 0;
            if (val > 255) val = 255;
            int val_int = (int) floor(val);
            d1->C[i][j] = (unsigned char) val_int;
            histogram[val_int]++;
        }

    // Print histogram
    printf("Brightness histogram:\n");
    for (i = 0; i < 256; i++) {
        printf("%3d %8d\n", i, histogram[i]);
    }

    // Output galaxy map data as a big binary file
    if (system("mkdir -p " SRCDIR "../output")) galmap_fatal(__FILE__, __LINE__, "Directory creation fail");
    out = fopen(SRCDIR "../output/galaxymap.dat", "w");
    i = H_SIZE_MAP1;
    fwrite(&i, sizeof(int), 1, out);
    i = V_SIZE_MAP1;
    fwrite(&i, sizeof(int), 1, out);
    fwrite(&d1->C, H_SIZE_MAP1 * V_SIZE_MAP1, 1, out);
    fclose(out);

    // Generate separate galaxy map for use by HTML5 planetarium
    image_ptr OutputImage;
    image_alloc(&OutputImage, H_SIZE_MAP2, V_SIZE_MAP2);
    const int margin = 1;

    // We do this by inverting the Mercator projection of Axel Mellinger's Milky Way panorama
    // Loop over all the pixels in the output map
    for (i = 0; i < V_SIZE_MAP2; i++)
        for (j = 0; j < H_SIZE_MAP2; j++) {
            // Work out the RA and Dec of this point in the output map
            const double longitude = j * (2 * M_PI) / H_SIZE_MAP2; // l=pi at left; l=-pi at right
            const double latitude = (i - V_SIZE_MAP2/2) * (2 * M_PI) / H_SIZE_MAP2; // b=0 in middle

            // Project onto a Mercator projection to get a pixel coordinate in Axel Mellinger's image
            const int x = (int) (R * longitude);
            const int y = (int) (R * log(tan(M_PI / 4 + (-latitude) / 2)) + InputImage.ysize / 2);

            const int x_min = (int) gsl_max(x - margin, 0);
            const int y_min = (int) gsl_max(y - margin, 0);
            const int x_max = (int) gsl_min(x + margin, InputImage.xsize - 1);
            const int y_max = (int) gsl_min(y + margin, InputImage.ysize - 1);

            // Average the three colour channels to get greyscale brightness
            double final_value = 255;
            for (int y_chk = y_min; y_chk <= y_max; y_chk++)
                for (int x_chk = x_min; x_chk <= x_max; x_chk++) {
                    int offset = y_chk * InputImage.xsize + x_chk;
                    double val = (InputImage.data_red[offset] +
                                  InputImage.data_grn[offset] +
                                  InputImage.data_blu[offset]) / 3;
                    if (val < final_value) final_value = val;
                }

            // Transfer function to increase brightness
            final_value = (final_value - 50) * 3;

            // Clip output brightness onto a 0-255 scale
            if (final_value < 0) final_value = 0;
            if (final_value > 255) final_value = 255;
            int val_int = (int) floor(final_value);

            int offset2 = i * OutputImage.xsize + j;
            OutputImage.data_red[offset2] = OutputImage.data_grn[offset2] = OutputImage.data_blu[offset2] = val_int;
        }

    // Output galaxy map as a PNG file
    image_put(SRCDIR "../output/galaxymap.png", OutputImage, 1);

    // Finish
    free(d1);
    if (DEBUG) galmap_log("Terminating normally.");
    return 0;
}
