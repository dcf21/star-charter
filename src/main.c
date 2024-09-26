// main.c
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

#include <gsl/gsl_errno.h>

#include "coreUtils/asciiDouble.h"
#include "coreUtils/strConstants.h"
#include "coreUtils/errorReport.h"

#include "listTools/ltMemory.h"

#include "settings/chart_config.h"
#include "settings/read_config.h"
#include "settings/render_chart.h"


//! Main entry point for rendering a single star chart, or a sequence of star charts, as described in a configuration
//! file. On the command line, the user should either supply a single filename for a configuration file to read, or
//! else the configuration is expected to be supplied on stdin.
//! \param argc - Command line arguments
//! \param argv - Command line arguments
//! \return - Exit status

int main(int argc, char **argv) {
    char help_string[LSTR_LENGTH], version_string[FNAME_LENGTH], version_string_underline[FNAME_LENGTH];
    char *filename = NULL;
    int have_filename = 0;
    chart_config *settings_destination = NULL;
    int got_chart = 0;

    // Initialise sub-modules
    if (DEBUG) stch_log("Initialising StarCharter");
    lt_memoryInit(&stch_error, &stch_log);

    // Turn off GSL's automatic error handler
    gsl_set_error_handler_off();

    // Make help and version strings
    snprintf(version_string, FNAME_LENGTH, "StarCharter %s", DCFVERSION);

    snprintf(help_string, FNAME_LENGTH, "StarCharter %s\n"
                                        "%s\n\n"
                                        "Usage: starchart.bin <filename>\n"
                                        "-h, --help:       Display this help.\n"
                                        "-v, --version:    Display version number.",
             DCFVERSION, str_underline(version_string, version_string_underline));

    // Scan command line options for any switches
    for (int i = 1; i < argc; i++) {
        // Ignore empty arguments
        if (strlen(argv[i]) == 0) continue;

        if (argv[i][0] != '-') {
            // If the switch doesn't start with a -, then treat it as a filename of a configuration file
            have_filename++;
            filename = argv[i];
            continue;
        }

        if ((strcmp(argv[i], "-v") == 0) || (strcmp(argv[i], "-version") == 0) || (strcmp(argv[i], "--version") == 0)) {
            // Switches -v and --version cause the version number to be printed
            stch_report(version_string);
            return 0;
        } else if ((strcmp(argv[i], "-h") == 0) || (strcmp(argv[i], "-help") == 0) ||
                   (strcmp(argv[i], "--help") == 0)) {
            // Switches -h and --help cause the usage string to be displayed
            stch_report(help_string);
            return 0;
        } else {
            // Return an error if an unknown switch is received
            snprintf(temp_err_string, FNAME_LENGTH,
                     "Received switch '%s' which was not recognised.\nType 'starchart.bin -help' for a list of "
                     "available commandline options.",
                     argv[i]);
            stch_error(temp_err_string);
            return 1;
        }
    }

    // Check that we have been provided with no more than one filename on the command line
    if (have_filename > 1) {
        snprintf(temp_err_string, FNAME_LENGTH,
                 "starchart.bin should be provided with only one filename on the command line to act upon. "
                 "Multiple filenames appear to have been supplied. Type 'starchart.bin -help' for a list of "
                 "available commandline options.");
        stch_error(temp_err_string);
        return 1;
    }

    // Allocate chart configuration data structures
    chart_config *chart_defaults = (chart_config *) malloc(sizeof(chart_config));
    chart_config *this_chart_config = (chart_config *) malloc(sizeof(chart_config));

    if ((chart_defaults == NULL) || (this_chart_config == NULL)) {
        stch_fatal(__FILE__, __LINE__, "Malloc fail.");
        exit(1);
    }

    // Set up default settings for star charts
    if (DEBUG) stch_log("Setting up default star chart parameters.");
    default_config(chart_defaults);

    if (DEBUG) {
        char buffer[FNAME_LENGTH];
        snprintf(buffer, FNAME_LENGTH, "Size of configuration data structure: %ld bytes", sizeof(chart_config));
        stch_log(buffer);
    }

    // Open and read the input configuration file
    read_configuration_file(have_filename ? filename : NULL, 0,
                            &got_chart, chart_defaults,
                            this_chart_config, &settings_destination);

    // Render final star chart
    if (got_chart) render_chart(this_chart_config);

    // Clean up and exit
    free(this_chart_config);
    free(chart_defaults);
    lt_freeAll(0);
    lt_memoryStop();
    if (DEBUG) stch_log("Terminating normally.");
    return 0;
}
