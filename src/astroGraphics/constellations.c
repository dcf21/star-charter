// constellations.c
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
#include <string.h>
#include <math.h>

#include <gsl/gsl_math.h>

#include "coreUtils/asciiDouble.h"
#include "coreUtils/errorReport.h"
#include "mathsTools/projection.h"
#include "settings/chart_config.h"
#include "vectorGraphics/lineDraw.h"
#include "vectorGraphics/cairo_page.h"

#define BUFLEN 1020

//! replace_at_with_space - Copy the input string into a static buffer. Search the input string for @ characters, and
//! replace these with spaces. Useful as constellations are stored in text file with white-space separated columns, so
//! spaces in constellation names are encoded as @ signs.
//! \param in - The input string
//! \return - Pointer to a static character buffer with the processed copy of the string.

static char *replace_at_with_space(const char *in) {
    int i, j;
    static char buf[BUFLEN + 4];
    char x;
    for (i = 0, j = 0; ((j < BUFLEN) && ((in[i] < '\0') || (in[i] > ' '))); i++, j++) {
        buf[j] = ((x = in[i]) == '@') ? ' ' : x;
    }
    buf[j] = '\0';
    return buf;
}

//! plot_constellation_boundaries - Draw lines around the boundaries of the constellations.
//! \param s - A <chart_config> structure defining the properties of the star chart to be drawn.
//! \param ld - A <line_drawer> structure used to draw lines on a cairo surface.

void plot_constellation_boundaries(chart_config *s, line_drawer *ld) {
    FILE *file;
    double x_first = 0, y_first = 0;
    char line[FNAME_LENGTH], *scan, constellation[6] = "@@@@";

    // This must be set to true initially, to ensure that colour is set when we start tracing the first constellation
    int was_highlighted = 1;

    // Set up line-drawing class
    ld_pen_up(ld, GSL_NAN, GSL_NAN, NULL, 1);
    ld_label(ld, NULL, 1, 1);

    // Open file defining the celestial coordinates of the constellation boundaries
    file = fopen(SRCDIR "../data/constellations/downloads/boundaries.dat", "r");
    if (file == NULL) stch_fatal(__FILE__, __LINE__, "Could not open constellation boundary data");

    while ((!feof(file)) && (!ferror(file))) {
        double ra, dec, x, y;
        file_readline(file, line);
        if ((line[0] == '#') || (strlen(line) < 28)) continue; // Comment line
        scan = line + 0;
        while ((scan[0] > '\0') && (scan[0] <= ' ')) scan++;
        ra = get_float(scan, NULL);
        scan = line + 12;
        while ((scan[0] > '\0') && (scan[0] <= ' ')) scan++;
        dec = get_float(scan, NULL);
        if (line[11] == '-') dec = -dec;

        // The boundary of Ursa Minor is a bit dodgy, and skirt around the pole star in the wrong direction.
        // If we don't draw it, then the boundary of Cepheus is in the right place
        if ((dec > 87) && (line[23] == 'U')) {
            ld_pen_up(ld, GSL_NAN, GSL_NAN, NULL, 1);
            continue;
        }

        plane_project(&x, &y, s, ra * M_PI / 12, dec * M_PI / 180, 0);

        if (s->zodiacal_only && (strncmp(line + 23, "AQR ", 4) != 0) && (strncmp(line + 23, "ARI ", 4) != 0) &&
            (strncmp(line + 23, "LEO ", 4) != 0) && (strncmp(line + 23, "CNC ", 4) != 0) &&
            (strncmp(line + 23, "CAP ", 4) != 0) && (strncmp(line + 23, "GEM ", 4) != 0) &&
            (strncmp(line + 23, "LIB ", 4) != 0) && (strncmp(line + 23, "OPH ", 4) != 0) &&
            (strncmp(line + 23, "TAU ", 4) != 0) && (strncmp(line + 23, "SGR ", 4) != 0) &&
            (strncmp(line + 23, "SCO ", 4) != 0) && (strncmp(line + 23, "VIR ", 4) != 0) &&
            (strncmp(line + 23, "PSC ", 4) != 0))
            continue;

        if (strncmp(line + 23, constellation, 4) != 0) {
            if (constellation[0] != '@') { ld_point(ld, x_first, y_first, NULL); }
            strncpy(constellation, line + 23, 4);
            constellation[5] = '\0';
            ld_pen_up(ld, GSL_NAN, GSL_NAN, NULL, 1);
            x_first = x;
            y_first = y;

            // Set the line colour and width for the boundary of this constellation
            if (strncmp(constellation, s->constellation_highlight, 3) == 0) {
                cairo_set_source_rgb(s->cairo_draw, s->star_col.red, s->star_col.grn,
                                     s->star_col.blu);
                cairo_set_line_width(s->cairo_draw, 2);
                was_highlighted = 1;
            } else if (was_highlighted) {
                cairo_set_source_rgb(s->cairo_draw, s->constellation_boundary_col.red, s->constellation_boundary_col.grn,
                                     s->constellation_boundary_col.blu);
                cairo_set_line_width(s->cairo_draw, 0.8);
                was_highlighted = 0;
            }
        }

        ld_point(ld, x, y, NULL);
    }

    fclose(file);
    ld_pen_up(ld, GSL_NAN, GSL_NAN, NULL, 1);
}

//! plot_constellation_sticks - Draw stick figures to represent the constellations.
//! \param s - A <chart_config> structure defining the properties of the star chart to be drawn.
//! \param ld - A <line_drawer> structure used to draw lines on a cairo surface.

void plot_constellation_sticks(chart_config *s, line_drawer *ld) {
    FILE *file;
    char line[FNAME_LENGTH];

    // Set line colour
    ld_pen_up(ld, GSL_NAN, GSL_NAN, NULL, 1);
    cairo_set_source_rgb(s->cairo_draw, s->constellation_stick_col.red,
                         s->constellation_stick_col.grn, s->constellation_stick_col.blu);
    cairo_set_line_width(s->cairo_draw, s->constellation_sticks_line_width);

    // Open file listing constellation stick figures by celestial coordinates
    const char *stick_definitions = "constellation_lines_simplified_by_RA_Dec.dat";
    if (s->constellation_stick_design == SW_STICKS_REY) stick_definitions = "constellation_lines_rey_by_RA_Dec.dat";

    char stick_definitions_path[FNAME_LENGTH];
    snprintf(stick_definitions_path, FNAME_LENGTH, "%s%s%s",
             SRCDIR,
             "../data/constellations/process_stick_figures/output/",
             stick_definitions);

    file = fopen(stick_definitions_path, "r");
    if (file == NULL) stch_fatal(__FILE__, __LINE__, "Could not open constellation stick figures");

    while ((!feof(file)) && (!ferror(file))) {
        double ra0, dec0, ra1, dec1, x0, y0, x1, y1;
        file_readline(file, line);
        if (line[0] == '#') continue; // Comment line
        const char *scan = line;
        while ((scan[0] != '\0') && (scan[0] <= ' ')) scan++;
        if (s->zodiacal_only && (strncmp(scan, "Aqua", 4) != 0) && (strncmp(scan, "Arie", 4) != 0) &&
            (strncmp(scan, "Leo ", 4) != 0) && (strncmp(scan, "Canc", 4) != 0) && (strncmp(scan, "Capr", 4) != 0) &&
            (strncmp(scan, "Gemi", 4) != 0) && (strncmp(scan, "Libr", 4) != 0) && (strncmp(scan, "Ophi", 4) != 0) &&
            (strncmp(scan, "Taur", 4) != 0) && (strncmp(scan, "Sagittar", 8) != 0) && (strncmp(scan, "Scor", 4) != 0) &&
            (strncmp(scan, "Virg", 4) != 0) && (strncmp(scan, "Pisc", 4) != 0))
            continue;
        while ((scan[0] != '\0') && (scan[0] != '-') && (scan[0] != '.') && (scan[0] != '+') &&
               ((scan[0] < '0') || (scan[0] > '9')))
            scan++;
        if (scan[0] == '\0') continue; // Blank line
        ra0 = get_float(scan, NULL);
        scan = next_word(scan);
        dec0 = get_float(scan, NULL);
        scan = next_word(scan);
        ra1 = get_float(scan, NULL);
        scan = next_word(scan);
        dec1 = get_float(scan, NULL);
        plane_project(&x0, &y0, s, ra0 * M_PI / 180, dec0 * M_PI / 180, 0);
        plane_project(&x1, &y1, s, ra1 * M_PI / 180, dec1 * M_PI / 180, 0);
        ld_pen_up(ld, GSL_NAN, GSL_NAN, NULL, 1);
        ld_point(ld, x0, y0, NULL);
        ld_point(ld, x1, y1, NULL);
        ld_pen_up(ld, GSL_NAN, GSL_NAN, NULL, 1);
    }

    fclose(file);
    ld_pen_up(ld, GSL_NAN, GSL_NAN, NULL, 1);
}

//! plot_constellation_names - Write the names of the constellations on the star chart.
//! \param s - A <chart_config> structure defining the properties of the star chart to be drawn.
//! \param page - A <cairo_page> structure defining the cairo drawing context.

void plot_constellation_names(chart_config *s, cairo_page *page) {
    FILE *file;
    char line[FNAME_LENGTH];

    strcpy(line, SRCDIR "../data/constellations/name_places.dat");
    if (s->language == SW_LANG_FR) strcpy(line, SRCDIR "../data/constellations/name_places_fr.dat");
    file = fopen(line, "r");
    if (file == NULL) stch_fatal(__FILE__, __LINE__, "Could not open constellation name data");

    while ((!feof(file)) && (!ferror(file))) {
        file_readline(file, line);
        if (line[0] == '#') continue; // Comment line
        const char *scan = line;
        while ((scan[0] > '\0') && (scan[0] <= ' ')) scan++;
        const char *name = scan;
        if (scan[0] == '\0') continue; // Blank line
        scan = next_word(scan);

        if (s->zodiacal_only && (strncmp(name, "Aqua", 4) != 0) && (strncmp(name, "Arie", 4) != 0) &&
            (strncmp(name, "Leo ", 4) != 0) && (strncmp(name, "Canc", 4) != 0) && (strncmp(name, "Capr", 4) != 0) &&
            (strncmp(name, "Gemi", 4) != 0) && (strncmp(name, "Libr", 4) != 0) && (strncmp(name, "Ophi", 4) != 0) &&
            (strncmp(name, "Taur", 4) != 0) && (strncmp(name, "Sagittar", 8) != 0) && (strncmp(name, "Scor", 4) != 0) &&
            (strncmp(name, "Virg", 4) != 0) && (strncmp(name, "Pisc", 4) != 0))
            continue;

        while (1) {
            double ra, dec, x, y;
            if (scan[0] == '\0') break; // End of line
            ra = get_float(scan, NULL);
            scan = next_word(scan);
            dec = get_float(scan, NULL);
            plane_project(&x, &y, s, ra * M_PI / 12, dec * M_PI / 180, 0);
            if ((x < s->x_min) || (x > s->x_max) || (y < s->y_min) || (y > s->y_max)) {
                scan = next_word(scan);
                continue;
            }

            char *label = replace_at_with_space(name);

            chart_label_buffer(page, s, s->constellation_label_col, label,
                               &(label_position) {x, y, 0, 0, 0}, 1,
                               0, 1, 1.4 * s->label_font_size_scaling,
                               0, 1, 0, -1);
            break;
        }
    }
    fclose(file);
}
