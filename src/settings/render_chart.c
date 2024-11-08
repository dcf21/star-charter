// render_chart.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "coreUtils/asciiDouble.h"
#include "coreUtils/strConstants.h"
#include "coreUtils/errorReport.h"

#include "settings/chart_config.h"

#include "astroGraphics/constellations.h"
#include "astroGraphics/ephemeris.h"
#include "astroGraphics/galaxyMap.h"
#include "astroGraphics/greatCircles.h"
#include "astroGraphics/deepSky.h"
#include "astroGraphics/deepSkyOutlines.h"
#include "astroGraphics/horizon.h"
#include "astroGraphics/gridLines.h"
#include "astroGraphics/meteorShower.h"
#include "astroGraphics/solarSystem.h"
#include "astroGraphics/stars.h"
#include "astroGraphics/scaleBars.h"
#include "astroGraphics/textAnnotations.h"
#include "astroGraphics/zenith.h"
#include "vectorGraphics/lineDraw.h"
#include "vectorGraphics/cairo_page.h"

//! render_chart - Main entry point to render a single star chart
//! \param s - The configuration for the star chart to be rendered

void render_chart(chart_config *s) {
    int i;
    cairo_page page;
    line_drawer ld;
    char line[FNAME_LENGTH];

    // Check star chart configuration, and insert any computed quantities
    config_init_arrays(s);

    // If we're plotting ephemerides for solar system objects, fetch the data now
    // We do this first, as auto-scaling plots use this data to determine which sky area to show
    const int total_ephemeris_points = ephemerides_fetch(
            &s->ephemeris_data, s->ephemeris_final_count, &s->ephemeris_definitions, s->ephemeris_resolution,
            s->ephemeris_coords, s->julian_date,
            s->solar_system_topocentric_correction, s->horizon_latitude, s->horizon_longitude);

    // Automatically scale plot to contain all the computed ephemeris tracks
    ephemerides_autoscale_plot(s, total_ephemeris_points);

    // Place text labels along the ephemeris tracks
    if (s->ephemeris_epochs_final_count < 1) {
        ephemerides_add_automatic_text_labels(s);
    } else {
        ephemerides_add_manual_text_labels(s);
    }

    // If we're showing the positions of solar system objects, fetch those positions now
    if (s->show_solar_system) {
        solar_system_write_ephemeris_definitions(&s->solar_system_ids, s->julian_date, s->solar_system_final_count,
                                                 &s->solar_system_ephemeris_definitions);
        ephemerides_fetch(&s->solar_system_ephemeris_data, s->solar_system_final_count,
                          &s->solar_system_ephemeris_definitions, s->ephemeris_resolution,
                          s->ephemeris_coords, s->julian_date,
                          s->solar_system_topocentric_correction, s->horizon_latitude, s->horizon_longitude);
    }

    // Check star chart configuration, and insert any computed quantities
    config_init_pointing(s);

    // Create a cairo surface object to render the star chart onto
    cairo_init(&page, s);

    // If we're shading the Milky Way behind the star chart, do that first
    if (s->plot_galaxy_map || s->shade_twilight || s->shade_near_sun || s->shade_not_observable) {
        if (DEBUG) {
            snprintf(line, FNAME_LENGTH, "Starting work on galaxy map image.");
            stch_log(line);
        }
        plot_galaxy_map(s);
        if (s->output_multiple_pages) move_to_next_page(s);
    }

    // If we're showing a PNG image behind the star chart, insert that next
    plot_background_image(s);

    // Initialise module for tracing lines on the star chart
    ld_init(&ld, s, page.x_labels, page.x2_labels, page.y_labels, page.y2_labels);

    // Draw the line of the equator
    if (s->plot_equator) plot_equator(s, &ld, &page);

    // Draw the line of the galactic plane
    if (s->plot_galactic_plane) plot_galactic_plane(s, &ld, &page);

    // Draw the line of the ecliptic
    if (s->plot_ecliptic) plot_ecliptic(s, &ld, &page);

    // Draw a grid of lines of constant RA and Dec
    if (s->show_grid_lines) plot_grid_lines(s, &ld);

    // Draw deep sky object outlines
    if (s->plot_dso) {
        if (s->output_multiple_pages) move_to_next_page(s);
        plot_deep_sky_outlines(s, &page);
    }

    // Draw deep sky objects
    if (s->plot_dso) {
        plot_deep_sky_objects(s, &page, s->messier_only);
    }

    // Draw constellation boundaries
    if (s->constellation_boundaries) plot_constellation_boundaries(s, &ld);

    // Draw stick figures to represent the constellations
    if (s->constellation_sticks) plot_constellation_sticks(s, &ld);

    // If we're labelling meteor shower radiants, do so now
    if (s->meteor_radiants_final_count > 0) plot_meteor_showers(s, &page);

    // Draw stars
    if (s->plot_stars) {
        if (s->output_multiple_pages) move_to_next_page(s);
        plot_stars(s, &page);
    }

    // Write the names of the constellations
    if (s->constellation_names) plot_constellation_names(s, &page);

    // If we're plotting ephemerides for solar system objects, draw these now
    for (i = 0; i < s->ephemeris_final_count; i++) {
        if (s->output_multiple_pages) move_to_next_page(s);
        plot_ephemeris(s, &ld, &page, i);
    }

    // If we're showing the positions of solar system objects, draw these now
    if (s->show_solar_system) {
        if (s->output_multiple_pages) move_to_next_page(s);
        plot_solar_system(s, &page);
    }

    // If we're showing the zenith, label it now
    if (s->show_horizon_zenith) plot_zenith(s, &page);
    if (s->show_poles) plot_celestial_poles(s, &page);

    // If we're showing scale bars, do so now
    if (s->scale_bars_final_count > 0) plot_scale_bars(s, &page);

    // If we're drawing arrow/line annotations, do so now
    if (s->arrow_labels_final_count > 0) plot_arrow_annotations(s);

    // If we're writing text labels, do so now
    if (s->text_labels_final_count > 0) plot_text_annotations(s, &page);

    // If we're drawing the horizon, do so now
    if (s->show_horizon) {
        if (s->output_multiple_pages) move_to_next_page(s);
        plot_horizon(s, &ld, &page);
    }

    // Render labels onto the chart while the clipping region is still in force
    if (s->output_multiple_pages) move_to_next_page(s);
    chart_label_unbuffer(&page);

    // Draw axes around the edge of the star chart
    draw_chart_edging(&page, s);

    // Vertical position of top of legends at the bottom of the star chart
    const double legend_y_pos_baseline =
            s->canvas_offset_y + s->width * s->aspect + 0.7 + (s->show_grid_lines ? 0.5 : 0);
    double legend_y_pos_left = legend_y_pos_baseline + 0.8 + s->copyright_gap;
    double legend_y_pos_right = legend_y_pos_baseline - 0.2;

    // If we're to show a key below the chart indicating the magnitudes of stars, draw this now
    if (s->magnitude_key) legend_y_pos_left = draw_magnitude_key(s, legend_y_pos_left);

    // If we're to show a key below the chart indicating the colours of the lines, draw this now
    if (s->great_circle_key) legend_y_pos_left = draw_great_circle_key(s, legend_y_pos_left);

    // If we're to show a key below the chart indicating the deep sky object symbols, draw this now
    if (s->dso_symbol_key) legend_y_pos_left = draw_dso_symbol_key(s, legend_y_pos_left);

    // If we're showing a table of the object's magnitude, draw that now
    if (s->ephemeris_table) legend_y_pos_right = draw_ephemeris_table(s, legend_y_pos_right, 1, NULL);

    // Finish up and write output
    if (DEBUG) {
        stch_log("Finished rendering chart");
    }
    if (chart_finish(&page, s)) { stch_fatal(__FILE__, __LINE__, "cairo close fail."); }

    // Free up storage
    ephemerides_free(s);
    config_close(s);
}

