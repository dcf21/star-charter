// jpl.c
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
#include <math.h>
#include <string.h>

#include <gsl/gsl_math.h>
#include <gsl/gsl_const_mksa.h>

#include "coreUtils/asciiDouble.h"
#include "coreUtils/errorReport.h"
#include "coreUtils/strConstants.h"

#include "listTools/ltDict.h"
#include "listTools/ltMemory.h"

#include "alias.h"
#include "jpl.h"
#include "orbitalElements.h"
#include "magnitudeEstimate.h"

//! The number of the DE4xx JPL ephemeris we are using
static int JPL_EphemNumber = 0;

//! The starting data file number in the DE4xx ephemeris (the starting year of the first file)
static int JPL_ASCII_first = 0;

//! The ending data file number in the DE4xx ephemeris (the starting year of the last file)
static int JPL_ASCII_last = 0;

//! The width of the DE4xx data file numbering (4 or 5 zero-padded characters)
static int JPL_ASCII_number_width = 0;

//! The step in the DE4xx ephemeris data file numbering
static int JPL_ASCII_step = 0;

//! Storage for data read from DE4xx ephemeris

//! The Julian day number of the start of the DE4xx ephemeris
static double JPL_EphemStart = 0;

//! The Julian day number of the end of the DE4xx ephemeris
static double JPL_EphemEnd = 0;

//! The number of days represented by each data block in DE4xx (32 days)
static double JPL_EphemStep = 0;

//! Length of a data record containing Chebyshev coefficients for all planets for time interval JPL_EphemStep
static int JPL_EphemArrayLen = 0;

//! The 15x3 shape array defined in GROUP 1050 in the header file
const int JPL_ShapeData_width = 15;
const int JPL_ShapeData_height = 3;
static int *JPL_ShapeData = NULL;

//! The metadata variables about the ephemeris, defined in GROUP 1040/1041
static dict *JPL_EphemVars = NULL;

//! The number of blocks needed to go from EphemStart to EphemEnd at step size EphemStep
static int JPL_EphemArrayRecords = 0;

//! The offset of the start of the ephemeris binary data from the start of the binary file
static int JPL_EphemData_offset = -1;

//! File pointer used to read binary data from DE4xx (we don't read whole binary ephemeris into memory)
static FILE *JPL_EphemFile = NULL;

//! File name of binary ephemeris file
static char jpl_ephem_filename[FNAME_LENGTH];

//! Buffer to hold the ephemeris data, as we load it
static double *JPL_EphemData = NULL;

//! Record of which ephemeris data records we have loaded
static unsigned char *JPL_EphemData_items_loaded = NULL;

//! Astronomical unit, measured in km
static double JPL_AU = 0.0;

//! jpl_setEphemerisNumber - Select which NASA JPL DE4xx ephemeris we are to use.
//! @param de_number - The number of the DE4xx ephemeris we are to read (e.g. 440 for DE440).

void jpl_setEphemerisNumber(int de_number) {
    switch (de_number) {
        case 405:
            JPL_EphemNumber = 405;
            JPL_ASCII_first = 1600;
            JPL_ASCII_last = 2200;
            JPL_ASCII_number_width = 4;
            JPL_ASCII_step = 20;
            break;
        case 430:
            JPL_EphemNumber = 430;
            JPL_ASCII_first = 1550;
            JPL_ASCII_last = 2550;
            JPL_ASCII_number_width = 4;
            JPL_ASCII_step = 100;
            break;
        case 431:
            JPL_EphemNumber = 431;
            JPL_ASCII_first = 1000;
            JPL_ASCII_last = 16000;
            JPL_ASCII_number_width = 5;
            JPL_ASCII_step = 1000;
            break;
        case 440:
            JPL_EphemNumber = 440;
            JPL_ASCII_first = 1550;
            JPL_ASCII_last = 2550;
            JPL_ASCII_number_width = 5;
            JPL_ASCII_step = 100;
            break;
        case 441:
            JPL_EphemNumber = 441;
            JPL_ASCII_first = 1000;
            JPL_ASCII_last = 16000;
            JPL_ASCII_number_width = 5;
            JPL_ASCII_step = 1000;
            break;
        default: {
            char msg[LSTR_LENGTH];
            snprintf(msg, FNAME_LENGTH, "Unsupported ephemeris: DE%d", de_number);
            ephem_fatal(__FILE__, __LINE__, msg);
            exit(1);
        }
    }

    // Ensure ephemeris is reloaded
    jpl_shutdown();

    // Clear out data read from DE4xx ephemeris
    JPL_EphemStart = 0;
    JPL_EphemEnd = 0;
    JPL_EphemStep = 0;
    JPL_EphemArrayLen = 0;
    JPL_ShapeData = NULL;
    JPL_EphemVars = NULL;
    JPL_EphemArrayRecords = 0;
    JPL_EphemData_offset = -1;
    JPL_EphemFile = NULL;
    JPL_EphemData = NULL;
    JPL_EphemData_items_loaded = NULL;
    JPL_AU = 0.0;
}

//! jpl_readBinaryData - Restore DE4xx from a binary dump of the data in <data/binary_DE4xx.bin>, to save parsing the
//! original ASCII files every time we are run.

int jpl_readBinaryData() {
    char fname[FNAME_LENGTH];

    // Work out the filename of the binary file that we are to open
    snprintf(fname, FNAME_LENGTH, "%s/../data/binary_de%d.bin", SRCDIR, JPL_EphemNumber);
    snprintf(jpl_ephem_filename, FNAME_LENGTH, "%s", fname);
    if (DEBUG) {
        char msg[LSTR_LENGTH];
        snprintf(msg, FNAME_LENGTH, "Fetching binary data from file <%s>.", fname);
        ephem_log(msg);
    }

    // Open binary data
    JPL_EphemFile = fopen(fname, "rb");
    if (JPL_EphemFile == NULL) return 1; // Failed to open binary file

    // Read headers to binary file
    dcf_fread((void *) &JPL_EphemStart, sizeof(double), 1, JPL_EphemFile, fname, __FILE__, __LINE__);
    if (DEBUG) {
        char msg[LSTR_LENGTH];
        snprintf(msg, FNAME_LENGTH, "JPL_EphemStart        = %10f", JPL_EphemStart);
        ephem_log(msg);
    }
    dcf_fread((void *) &JPL_EphemEnd, sizeof(double), 1, JPL_EphemFile, fname, __FILE__, __LINE__);
    if (DEBUG) {
        char msg[LSTR_LENGTH];
        snprintf(msg, FNAME_LENGTH, "JPL_EphemEnd          = %10f", JPL_EphemEnd);
        ephem_log(msg);
    }
    dcf_fread((void *) &JPL_EphemStep, sizeof(double), 1, JPL_EphemFile, fname, __FILE__, __LINE__);
    if (DEBUG) {
        char msg[LSTR_LENGTH];
        snprintf(msg, FNAME_LENGTH, "JPL_EphemStep         = %10f", JPL_EphemStep);
        ephem_log(msg);
    }
    dcf_fread((void *) &JPL_AU, sizeof(double), 1, JPL_EphemFile, fname, __FILE__, __LINE__);
    if (DEBUG) {
        char msg[LSTR_LENGTH];
        snprintf(msg, FNAME_LENGTH, "JPL_AU                = %10f", JPL_AU);
        ephem_log(msg);
    }
    dcf_fread((void *) &JPL_EphemArrayLen, sizeof(int), 1, JPL_EphemFile, fname, __FILE__, __LINE__);
    if (DEBUG) {
        char msg[LSTR_LENGTH];
        snprintf(msg, FNAME_LENGTH, "JPL_EphemArrayLen     = %10d", JPL_EphemArrayLen);
        ephem_log(msg);
    }
    dcf_fread((void *) &JPL_EphemArrayRecords, sizeof(int), 1, JPL_EphemFile, fname, __FILE__, __LINE__);
    if (DEBUG) {
        char msg[LSTR_LENGTH];
        snprintf(msg, FNAME_LENGTH, "JPL_EphemArrayRecords = %10d", JPL_EphemArrayRecords);
        ephem_log(msg);
    }

    JPL_ShapeData = (int *) malloc(JPL_ShapeData_width * JPL_ShapeData_height * sizeof(int));
    if (JPL_ShapeData == NULL) {
        ephem_fatal(__FILE__, __LINE__, "Malloc fail.");
        exit(1);
    }

    // Read shape data array
    dcf_fread((void *) JPL_ShapeData, sizeof(int), JPL_ShapeData_width * JPL_ShapeData_height,
              JPL_EphemFile, fname, __FILE__, __LINE__);

    // We have now reached the actual ephemeris data. We don't load this into RAM since it is large and this would
    // take time. Instead, store a pointer to the offset of the start of the ephemeris from the beginning of file.
    JPL_EphemData_offset = (int) ftell(JPL_EphemFile);

    // Allocate memory to use to store ephemeris, as we load it
    JPL_EphemData = (double *) malloc(JPL_EphemArrayLen * JPL_EphemArrayRecords * sizeof(double));

    // Allocate array to record which blocks we have already loaded from disk
    JPL_EphemData_items_loaded = (unsigned char *) malloc(JPL_EphemArrayRecords * sizeof(unsigned char));
    memset(JPL_EphemData_items_loaded, 0, JPL_EphemArrayRecords);

    if (DEBUG) {
        char msg[LSTR_LENGTH];
        snprintf(msg, FNAME_LENGTH, "Data file successfully opened.");
        ephem_log(msg);
    }

    // Success
    return 0;
}

//! jpl_dumpBinaryData - Dump the contents of DE4xx to a binary dump in <data/binary_DE4xx.bin>, to save parsing the
//! original ASCII files every time we are run.

void jpl_dumpBinaryData() {
    FILE *output;
    char fname[FNAME_LENGTH];

    snprintf(fname, FNAME_LENGTH, "%s/../data/binary_de%d.bin", SRCDIR, JPL_EphemNumber);
    if (DEBUG) {
        char msg[LSTR_LENGTH];
        sprintf(msg, "Dumping binary data to file <%s>.", fname);
        ephem_log(msg);
    }
    output = fopen(fname, "w");
    if (output == NULL) return; // FAIL
    fwrite((void *) &JPL_EphemStart, sizeof(double), 1, output);
    fwrite((void *) &JPL_EphemEnd, sizeof(double), 1, output);
    fwrite((void *) &JPL_EphemStep, sizeof(double), 1, output);
    fwrite((void *) &JPL_AU, sizeof(double), 1, output);
    fwrite((void *) &JPL_EphemArrayLen, sizeof(int), 1, output);
    fwrite((void *) &JPL_EphemArrayRecords, sizeof(int), 1, output);
    fwrite((void *) JPL_ShapeData, sizeof(int), JPL_ShapeData_width * JPL_ShapeData_height, output);
    fwrite((void *) JPL_EphemData, sizeof(double), JPL_EphemArrayLen * JPL_EphemArrayRecords, output);
    fclose(output);
    if (DEBUG) {
        char msg[LSTR_LENGTH];
        snprintf(msg, FNAME_LENGTH, "Data successfully dumped.");
        ephem_log(msg);
    }
}

//! jpl_readData - Read the data contained in the original DE4xx ASCII data files

void jpl_readAsciiData() {
    char fname[FNAME_LENGTH], line[FNAME_LENGTH], key[FNAME_LENGTH];
    const char *line_ptr;

    FILE *input = NULL; // The ASCII file we are reading the ephemeris from
    int year = -1; // The year number in the filename of the ephemeris file we are reading (advances in N-year steps)
    int state = -1; // The last GROUP number header we passed; different blocks of data have different GROUP numbers
    int var_dict_len = -1; // The number of metadata variables set in GROUP 1040, in the header of the ephemeris
    int first = 0; // Boolean flag indicating whether this is the first line of the current GROUP
    long malloced_data_len = -1; // The number of bytes allocated to hold the ephemeris in <JPL_EphemData>
    int pos_1040 = 0;
    int pos_1041 = 0;
    int pos_1050 = 0;
    long pos_1070 = 0; // The current write position in the array <JPL_EphemData>
    int count_ephem_data_this_block = 0; // Count the floating point numbers we've read in the current ephemeris block
    double *var_val = NULL; // Array of doubles for holding the values of the metadata variables in GROUP 1040/1041
    double jd_expected_next = 0; // The Julian Day number expected at the start of the next ephemeris data block

    // Try and read the ephemeris from binary files. Only proceed with parsing the original files if binary files
    // don't exist.
    if (jpl_readBinaryData() == 0) return;

    // Logging message to report that we are parsing the DE4xx files
    if (DEBUG) {
        char msg[LSTR_LENGTH];
        snprintf(msg, FNAME_LENGTH, "Beginning to read JPL ephemeris DE%d.", JPL_EphemNumber);
        ephem_log(msg);
    }

    // The header file, <data/header.430>, contains global information about the ephemeris
    snprintf(fname, FNAME_LENGTH, "%s/../data/de%d/header.%d", SRCDIR, JPL_EphemNumber, JPL_EphemNumber);

    while (1) {
        // If we don't currently have an ephemeris file with readable data, we need to open one
        if ((input == NULL) || (feof(input)) || (ferror(input))) {
            // If we already have an ephemeris file open, close it
            if (input != NULL) fclose(input);

            // If we've reached the end of the time span of DE4xx, we're finished
            if (year >= JPL_ASCII_last) break;

            // If we've not started reading yet, start at the beginning, otherwise advance N years
            if (year == -1) year = JPL_ASCII_first;
            else year += JPL_ASCII_step;

            // Log message that we are opening a new ephemeris file
            if (DEBUG) {
                char msg[LSTR_LENGTH];
                snprintf(msg, FNAME_LENGTH, "Opening file <%s>", fname);
                ephem_log(msg);
                if (JPL_EphemArrayLen > 0) {
                    snprintf(msg, FNAME_LENGTH, "Currently reading JD %.1f; block number %d.",
                             jd_expected_next, (int) round(pos_1070 / JPL_EphemArrayLen));
                    ephem_log(msg);
                }
            }

            // Open the file -- first time around the header files; subsequently, the ephemeris itself
            input = fopen(fname, "rt");
            if (input == NULL) {
                char msg[LSTR_LENGTH];
                snprintf(msg, FNAME_LENGTH, "Could not open ephemeris file <%s>", fname);
                ephem_fatal(__FILE__, __LINE__, msg);
                exit(1);
            }

            // Populate <fname> with the filename of the next ASCII ephemeris data file to read
            // The ephemeris data is contained in files <data/de4xx/ascp????.4xx>, where ???? is the start year
            snprintf(fname, FNAME_LENGTH, "%s/../data/de%d/ascp%0*d.%d", SRCDIR,
                     JPL_EphemNumber, JPL_ASCII_number_width, year, JPL_EphemNumber);
        }

        // Read a line of data from the ephemeris file
        file_readline(input, line, sizeof line);
        str_strip(line, line);

        // Ignore blank lines
        if (line[0] == '\0') continue;

        // The first line of the header file contains the length of the records in the ephemeris files (NCOEFF=...)
        if (strncmp(line, "KSIZE=", 5) == 0) {
            //KSIZE =
            line_ptr = next_word(line);
            line_ptr = next_word(line_ptr);

            //NCOEFF =
            line_ptr = next_word(line_ptr);
            JPL_EphemArrayLen = (int) get_float(line_ptr, NULL); // Number of items in each GROUP 1070 data block
            if (DEBUG) {
                char msg[LSTR_LENGTH];
                snprintf(msg, FNAME_LENGTH, "Each record of length %d floats.", JPL_EphemArrayLen);
                ephem_log(msg);
            }
            continue;
        }

        // The header file is divided into parts, each with a GROUP number. We set <state> to the current group number
        if (strncmp(line, "GROUP", 5) == 0) {
            first = 1; // This is the first line of this group
            line_ptr = next_word(line);
            state = (int) get_float(line_ptr, NULL); // Set state to the new GROUP number
            if (DEBUG) {
                char msg[LSTR_LENGTH];
                snprintf(msg, FNAME_LENGTH, "Entering GROUP %d.", state);
                ephem_log(msg);
            }

            if (state == 1040) {
                // Entering group 1040, which defines a number of variables
                // Before we start, we create an associative array to put them in
                JPL_EphemVars = dictInit(HASHSIZE_SMALL);
                if (JPL_EphemVars == NULL) {
                    ephem_fatal(__FILE__, __LINE__, "Malloc fail.");
                    exit(1);
                }
            } else if (state == 1050) {
                // Entering group 1050, which defines the shape array
                // Before we start we need to allocate storage for the 15x3 shape array
                JPL_ShapeData = (int *) malloc(JPL_ShapeData_width * JPL_ShapeData_height * sizeof(int));
                if (JPL_ShapeData == NULL) {
                    ephem_fatal(__FILE__, __LINE__, "Malloc fail.");
                    exit(1);
                }
                for (int shape_i = 0; shape_i < JPL_ShapeData_width; shape_i++) {
                    for (int shape_j = 0; shape_j < JPL_ShapeData_height; shape_j++) {
                        JPL_ShapeData[shape_j + shape_i * JPL_ShapeData_height] = 0;
                    }
                }
            } else if (state == 1070) {
                // Entering group 1070, which defines the actual ephemeris data
                // Before we start we need to allocate storage for the data

                // Transfer the value of the variable "AU" from the quantities defined in GROUP 1041, to <JPL_AU>
                double *dptr;
                dictLookup(JPL_EphemVars, "AU", NULL, (void *) &dptr); // astronomical unit, measured in km
                JPL_AU = *dptr;
                if (DEBUG) {
                    char msg[LSTR_LENGTH];
                    snprintf(msg, FNAME_LENGTH, "Astronomical unit = %.1f km.", JPL_AU);
                    ephem_log(msg);
                }

                // Work out the number of records that the ephemeris will contain
                JPL_EphemArrayRecords = (int) ceil((JPL_EphemEnd - JPL_EphemStart) / JPL_EphemStep);

                // Work out how many bytes of storage we need
                malloced_data_len = JPL_EphemArrayLen * JPL_EphemArrayRecords;

                // Allocate storage for the ephemeris data
                JPL_EphemData = (double *) malloc(malloced_data_len * sizeof(double));
                if (JPL_EphemData == NULL) {
                    ephem_fatal(__FILE__, __LINE__, "Malloc fail.");
                    exit(1);
                }

                // Initialise storage to zero
                for (int i = 0; i < malloced_data_len; i++) {
                    JPL_EphemData[i] = 0;
                }
            }
            continue;
        }

        if (state == 1010) {
            // Group 1010 defines the name of the ephemeris, and the start and end dates. We have already hard-coded these
            continue;
        } else if (state == 1030) {
            // Group 1030 has three numbers: the start and end Julian day numbers, and the step size (32 days)
            JPL_EphemStart = get_float(line, NULL);
            line_ptr = next_word(line);
            JPL_EphemEnd = get_float(line_ptr, NULL);
            line_ptr = next_word(line_ptr);
            JPL_EphemStep = get_float(line_ptr, NULL);

            // Expected JD at the start of the ephemeris data blocks in GROUP 1070
            jd_expected_next = JPL_EphemStart;

            if (DEBUG) {
                char msg[LSTR_LENGTH];
                snprintf(msg, FNAME_LENGTH, "Ephemeris spans from %.1f to %.1f; step size %.1f days.",
                         JPL_EphemStart, JPL_EphemEnd, JPL_EphemStep);
                ephem_log(msg);
            }
        } else if (state == 1040) {
            // Group 1040 has a list of variables which are set within the header

            // The first line of GROUP 1040 is the number of items
            if (first) {
                first = 0;
                var_dict_len = (int) get_float(line, NULL);
                var_val = (double *) lt_malloc(var_dict_len * sizeof(double));
                if (var_val == NULL) {
                    ephem_fatal(__FILE__, __LINE__, "Malloc fail.");
                    exit(1);
                }
                if (DEBUG) {
                    char msg[LSTR_LENGTH];
                    sprintf(msg, "Variable dictionary has %d entries.", var_dict_len);
                    ephem_log(msg);
                }
                continue;
            }

            // Subsequent lines list the names of the variables in turn
            line_ptr = line;
            while (line_ptr[0] != '\0') {
                int i = 0;
                if (pos_1040 >= var_dict_len) {
                    ephem_fatal(__FILE__, __LINE__, "Variable dictionary overflow.");
                    exit(1);
                }
                for (i = 0; line_ptr[i] > ' '; i++) key[i] = line_ptr[i];
                key[i] = '\0';
                dictAppendPtr(JPL_EphemVars, key, var_val + (pos_1040++), 0, 0, DATATYPE_VOID);
                line_ptr = next_word(line_ptr);
            }
        } else if (state == 1041) {
            // Group 1041 has a list of the values of the variables which are set within the header

            // The first line of GROUP 1041 is the number of items. This should match GROUP 1040's length!
            if (first) {
                double tmp = (int) get_float(line, NULL);
                first = 0;
                if (tmp != var_dict_len) {
                    ephem_fatal(__FILE__, __LINE__, "Inconsistent lengths stated for variable dictionary.");
                    exit(1);
                }
                continue;
            }

            // Each subsequent line defines the whitespace separated values of some variables
            line_ptr = line;
            while (line_ptr[0] != '\0') {
                if (pos_1041 >= var_dict_len) {
                    // Skip final terminating zero
                    double value = get_float(line_ptr, NULL);
                    if (value == 0) {
                        line_ptr = next_word(line_ptr);
                        continue;
                    }

                    // Otherwise throw error if we have too many values
                    ephem_fatal(__FILE__, __LINE__, "Variable dictionary overflow.");
                    exit(1);
                }
                var_val[pos_1041++] = get_float(line_ptr, NULL);
                line_ptr = next_word(line_ptr);
            }
        } else if (state == 1050) {
            // GROUP 1050 defines a 15x3 shape array, which we store in <JPL_ShapeData>
            line_ptr = line;
            for (int i = 0; line_ptr[0] != '\0'; i++) {
                if (i >= JPL_ShapeData_width) {
                    ephem_fatal(__FILE__, __LINE__, "Shape array horizontal overflow.");
                    exit(1);
                }
                if (pos_1050 >= JPL_ShapeData_height) {
                    ephem_fatal(__FILE__, __LINE__, "Shape array vertical overflow.");
                    exit(1);
                }
                JPL_ShapeData[pos_1050 + JPL_ShapeData_height * i] = (int) get_float(line_ptr, NULL);
                line_ptr = next_word(line_ptr);
            }
            pos_1050++;
        } else if (state == 1070) {
            // GROUP 1070 is the actual ephemeris data itself

            // A line " 1 1018 " at the start of each block, which gives the length of each record, marks a new block
            if (strlen(line) < 20) {
                if ((count_ephem_data_this_block > 0) && (count_ephem_data_this_block < JPL_EphemArrayLen)) {
                    if (DEBUG) {
                        char msg[LSTR_LENGTH];
                        snprintf(msg, FNAME_LENGTH, "Data block truncated at %d items; expected %d items.",
                                 count_ephem_data_this_block, JPL_EphemArrayLen);
                        ephem_log(msg);
                    }

                    // Pad truncated data blocks with zeros
                    for (int i = count_ephem_data_this_block; i < JPL_EphemArrayLen; i++) {
                        JPL_EphemData[pos_1070++] = 0;
                    }
                }
                count_ephem_data_this_block = 0;
                continue;
            }

            // The first two items within each block are jd_min and jd_max. After we have read them, check them.

            // Loop over the words on each line of the ephemeris
            line_ptr = line;
            while (line_ptr[0] != '\0') {
                if (count_ephem_data_this_block == 2) {
                    // We have just started a new block. Check that jd_min has the value we expect
                    const double new_block_jd_min = JPL_EphemData[pos_1070 - 2];
                    const double new_block_jd_max = JPL_EphemData[pos_1070 - 1];

                    const double record_number_f = (new_block_jd_min - JPL_EphemStart) / JPL_EphemStep;
                    const int record_number_i = (int) round(record_number_f);
                    const double rounding_error = fabs(record_number_f - record_number_i);

                    int unexpected_jd = fabs(new_block_jd_min - jd_expected_next) > 0.1;

                    // Logging message stating the time range of the data block we've just started reading
                    if (DEBUG) {
                        char msg[LSTR_LENGTH];
                        // snprintf(msg, FNAME_LENGTH, "Record %5d spans from %.1f to %.1f.",
                        //          record_number_i, new_block_jd_min, new_block_jd_max);
                        // ephem_log(msg);

                        if (rounding_error > 1e-5) {
                            snprintf(msg, FNAME_LENGTH,
                                     "Rounding error of %.6f days in block start time %.1f (expecting %.1f).",
                                     rounding_error, new_block_jd_min,
                                     JPL_EphemStart + JPL_EphemStep * record_number_i);
                            ephem_log(msg);
                        }

                        if (unexpected_jd) {
                            snprintf(msg, FNAME_LENGTH,
                                     "Out-of-sequence record detected at %.1f (expecting %.1f).",
                                     new_block_jd_min, jd_expected_next);
                            ephem_log(msg);
                        }
                    }

                    // Ensure we are reading data into the correct place in JPL_EphemData
                    pos_1070 = record_number_i * JPL_EphemArrayLen + 2;

                    // Make sure that we've not read more data than the ephemeris header told us to allocate space for
                    if ((pos_1070 < 0) || (pos_1070 >= malloced_data_len)) {
                        ephem_fatal(__FILE__, __LINE__, "Data array overflow.");
                        exit(1);
                    }

                    // Copy start/end JDs for this block to the new write location
                    JPL_EphemData[pos_1070 - 2] = new_block_jd_min;
                    JPL_EphemData[pos_1070 - 1] = new_block_jd_max;

                    // Update <jd_expected_next> to reflect the end of the current data block
                    jd_expected_next = new_block_jd_max;
                }

                const double next_value = get_float(line_ptr, NULL);

                if (count_ephem_data_this_block >= JPL_EphemArrayLen) {
                    // Ignore trailing zeros, which older DE4xx use to pad the end of data blocks
                    if (next_value != 0) {
                        ephem_fatal(__FILE__, __LINE__, "Too many values listed in data block.");
                        exit(1);
                    }
                } else {
                    // Make sure that we've not read more data than the ephemeris header told us to allocate space for
                    if (pos_1070 >= malloced_data_len) {
                        ephem_fatal(__FILE__, __LINE__, "Data array overflow.");
                        exit(1);
                    }

                    // Read the floating point numbers from the text file into a big array
                    JPL_EphemData[pos_1070++] = next_value;
                }

                // Proceed to the next word
                line_ptr = next_word(line_ptr);

                // Count the number of words we have read
                count_ephem_data_this_block++;
            }
        }
    }

    if (DEBUG) {
        char msg[LSTR_LENGTH];
        snprintf(msg, FNAME_LENGTH, "Read ephemeris into a data array of length %ld bytes; %d blocks.",
                 malloced_data_len, (int) (malloced_data_len / JPL_EphemArrayLen));
        ephem_log(msg);
        snprintf(msg, FNAME_LENGTH, "Final write position was JD %.1f; block number %d.",
                 jd_expected_next, (int) round(pos_1070 / JPL_EphemArrayLen));
        ephem_log(msg);
        snprintf(msg, FNAME_LENGTH, "Finished reading JPL ephemeris DE%d.", JPL_EphemNumber);
        ephem_log(msg);
    }

    // Now that we've parsed the text-based DE4xx files that we downloaded, we dump the data in binary format
    jpl_dumpBinaryData();

    // Make table indicating that we have loaded all the ephemeris data
    JPL_EphemData_items_loaded = (unsigned char *) malloc(JPL_EphemArrayRecords * sizeof(unsigned char));
    memset(JPL_EphemData_items_loaded, 1, JPL_EphemArrayRecords);

    // Free storage for local copy
    free(JPL_EphemData);

    // Open file pointer to version on disk
    jpl_readBinaryData();
}

//! chebyshev - Evaluate a Chebyshev polynomial
//! \param coeffs - The coefficients of the Chebyshev polynomial
//! \param Ncoeff - The number of coefficients
//! \param x - The point at which to evaluate the Chebyshev polynomial
//! \return The value of the Chebyshev polynomial

double chebyshev(double *coeffs, int Ncoeff, double x) {
    double x2 = 2 * x;
    double d = 0, dd = 0, ddd = 0;
    int k = Ncoeff - 1;

    while (k > 0) {
        ddd = dd;
        dd = d;
        d = x2 * dd - ddd + coeffs[k];
        k--;
    }
    return x * d - dd + coeffs[0];
}

//! jpl_computeXYZ - Evaluate the 3D position of a solar system body at Julian date JD (in ICRF v2 as used by DE4xx)
//! \param [in] body_id - The body's index within DE4xx (0 Sun - 12 Pluto)
//! \param [in] jd - Julian day number; TT
//! \param [out] x - Cartesian position of body (AU). This axis points away from RA=0.
//! \param [out] y - Cartesian position of body (AU).
//! \param [out] z - Cartesian position of body (AU). This axis points towards J2000.0 north celestial pole

void jpl_computeXYZ(int body_id, double jd, double *x, double *y, double *z) {
    int record_index, i;
    double dt, tc;

#pragma omp critical (jpl_init)
    {
        // If we haven't already loaded DE4xx data, make sure we have done so now
        if (JPL_EphemFile == NULL) jpl_readAsciiData();
    }

    // If this query falls outside the time span of DE4xx, then reject the query
    if ((JPL_EphemFile == NULL) || (jd < JPL_EphemStart) || (jd > JPL_EphemEnd)) {
        *x = *y = *z = GSL_NAN;
        return;
    }

    // Work out which block within DE4xx this query falls within
    record_index = floor((jd - JPL_EphemStart) / JPL_EphemStep);

    // Clip block number within allowed range
    if (record_index < 0) record_index = 0;
    if (record_index >= JPL_EphemArrayRecords) record_index = JPL_EphemArrayRecords - 1;

    if (!JPL_EphemData_items_loaded[record_index]) {
        // Create a pointer to the block that we need to query
        long data_position_needed = JPL_EphemData_offset + record_index * JPL_EphemArrayLen * sizeof(double);

#pragma omp critical (jpl_fetch)
        {
            fseek(JPL_EphemFile, data_position_needed, SEEK_SET);
            dcf_fread((void *) &JPL_EphemData[record_index * JPL_EphemArrayLen],
                      sizeof(double), JPL_EphemArrayLen, JPL_EphemFile,
                      jpl_ephem_filename, __FILE__, __LINE__);
            JPL_EphemData_items_loaded[record_index] = 1;
        }
    }
    double *data = &JPL_EphemData[record_index * JPL_EphemArrayLen];

    double t0 = data[0]; // First JD of time step
    //double t1 = data[1]; // Last JD of time step

    // Read the shape array data about this body, which gives us 3 numbers...

    // Offset of start of Chebyshev coefficient list (FORTRAN numbering starts at 1!)
    int c = JPL_ShapeData[body_id * JPL_ShapeData_height + 0];

    // Number of Chebyshev coefficients
    int n = JPL_ShapeData[body_id * JPL_ShapeData_height + 1];

    // Number of sub-steps within time step
    int g = JPL_ShapeData[body_id * JPL_ShapeData_height + 2];

    if (g == 1) {
        // If the time step is not subdivided, then life is very easy...
        dt = JPL_EphemStep; // size of whole time step
        tc = 2 * (jd - t0) / dt - 1; // time position within this step, scaled to range -1 to 1.
    } else {
        // Work out which subdivision we fall within...
        dt = JPL_EphemStep / g; // size of each subdivision

        // Work out which subdivision we fall into, and clamp it within sensible range
        i = (int) floor((jd - t0) / dt);
        if (i >= g) i = g - 1;
        if (i < 0) i = 0;

        // Update the offset of start of Chebyshev coefficient list for this subdivision
        c += i * 3 * n;

        // time position within this step, scaled to range -1 to 1.
        tc = 2 * ((jd - t0) - i * dt) / dt - 1;
        if (tc < -1) tc = -1;
        if (tc > 1) tc = 1;
    }

    // Offset within block of coefficients uses FORTRAN numbering
    double *data_scan = data + (c - 1);

    // Evaluate the Chebyshev polynomial
    *x = chebyshev(data_scan, n, tc) / JPL_AU;
    *y = chebyshev(data_scan + 1 * n, n, tc) / JPL_AU;
    *z = chebyshev(data_scan + 2 * n, n, tc) / JPL_AU;

    // For diagnostics, it may be useful to print internal state
    // if (DEBUG) {
    //   snprintf(msg, FNAME_LENGTH,
    //            "t0 = %.1f ; t1 = %.1f ; JD = %.1f ; C = %d ; N = %d ; G = %d ; dt = %.1f ; Tc = %.1f ; "
    //            "x = %.1f ; y = %.1f ; z = %.1f", t0,t1,JD,C,N,G,dt,Tc,*x,*y,*z);
    //   ephem_log(msg);
    // }
}

//! jpl_computeEphemeris - Main entry point for estimating the position, brightness, etc of an object at a particular
//! time, using data from the DE4xx ephemeris.
//! \param [in] bodyId - The object ID number we want to query. 0=Mercury. 2=Earth/Moon barycentre. 9=Pluto. 10=Sun, etc
//! \param [in] jd - The Julian date to query; TT
//! \param [out] x - x,y,z position of body, in ICRF v2, in AU, relative to solar system barycentre.
//! \param [out] y - x points to RA=0. y points to RA=6h.
//! \param [out] z - z points to celestial north pole (i.e. J2000.0).
//! \param [out] ra - Right ascension of the object (J2000.0, radians, relative to geocentre)
//! \param [out] dec - Declination of the object (J2000.0, radians, relative to geocentre)
//! \param [out] mag - Estimated V-band magnitude of the object
//! \param [out] phase - Phase of the object (0-1)
//! \param [out] angSize - Angular size of the object (diameter; arcseconds)
//! \param [out] phySize - Physical size of the object (diameter; metres)
//! \param [out] albedo - Albedo of the object (0-1)
//! \param [out] sunDist - Distance of the object from the Sun (AU)
//! \param [out] earthDist - Distance of the object from the Earth (AU)
//! \param [out] sunAngDist - Angular distance of the object from the Sun, as seen from the Earth (radians)
//! \param [out] theta_ESO - Angular distance of the object from the Earth, as seen from the Sun (radians)
//! \param [out] eclipticLongitude - The ecliptic longitude of the object (J2000.0 radians)
//! \param [out] eclipticLatitude - The ecliptic latitude of the object (J2000.0 radians)
//! \param [out] eclipticDistance - The separation of the object from the Sun, in ecliptic longitude (radians)
//! \param [in] ra_dec_epoch - The epoch of the RA/Dec coordinates to output. Supply 2451545.0 for J2000.0.
//! \param [in] do_topocentric_correction - Boolean indicating whether to apply topocentric correction to (ra, dec)
//! \param [in] topocentric_latitude - Latitude (deg) of observer on Earth, if topocentric correction is applied.
//! \param [in] topocentric_longitude - Longitude (deg) of observer on Earth, if topocentric correction is applied.

void jpl_computeEphemeris(int bodyId, const double jd, double *x, double *y, double *z, double *ra, double *dec,
                          double *mag, double *phase, double *angSize, double *phySize, double *albedo, double *sunDist,
                          double *earthDist, double *sunAngDist, double *theta_ESO, double *eclipticLongitude,
                          double *eclipticLatitude, double *eclipticDistance, const double ra_dec_epoch,
                          const int do_topocentric_correction,
                          const double topocentric_latitude, const double topocentric_longitude) {
    // Position of the Sun relative to the solar system barycentre, J2000.0 equatorial coordinates, AU
    double sun_pos_x, sun_pos_y, sun_pos_z;

    // Position of the Earth-Moon barycentre, relative to the solar system barycentre, AU
    double EMX, EMY, EMZ;

    // Moon's position relative to the Earth-Moon barycentre, AU
    double moon_pos_x, moon_pos_y, moon_pos_z;

    // Earth's position relative to the solar system barycentre, J2000.0 equatorial coordinates, AU
    double earth_pos_x, earth_pos_y, earth_pos_z;

    // Boolean flags indicating whether this is the Earth, Sun or Moon (which need special treatment)
    int is_moon = 0, is_earth = 0, is_sun = 0;

    double EMX_future, EMY_future, EMZ_future; // Position of the Earth-Moon centre of mass
    double moon_pos_x_future, moon_pos_y_future, moon_pos_z_future;
    double earth_pos_x_future, earth_pos_y_future, earth_pos_z_future;

    // Body 19 is the Earth.
    // DE4xx gives us the Earth/Moon barycentre (body 2), from which we subtract a small fraction of the Moon's
    // offset (body 9) to get the Earth's centre of mass
    if (bodyId == 19) {
        bodyId = 2;
        is_earth = 1;
    }

    // Body 9 is the Moon.
    // DE4xx gives us a position relative to the Earth/Moon barycentre (body 2), so we add body 2's position
    if (bodyId == 9) {
        is_moon = 1;
    }

    // Body 10 is the Sun
    if (bodyId == 10) {
        is_sun = 1;
    }

    // We give asteroids body numbers which start at 1e7 + 1 (Ceres). These aren't in DE4xx, so use orbital elements.
    if (bodyId > ASTEROIDS_OFFSET) {
        orbitalElements_computeEphemeris(bodyId, jd, x, y, z, ra, dec, mag, phase, angSize, phySize, albedo, sunDist,
                                         earthDist, sunAngDist, theta_ESO, eclipticLongitude, eclipticLatitude,
                                         eclipticDistance, ra_dec_epoch,
                                         do_topocentric_correction, topocentric_latitude, topocentric_longitude);
        return;
    }

    // If we've got a query for a body which isn't in DE4xx, then we can't proceed
    if ((bodyId < 0) || (bodyId > 10)) {
        *x = *y = *z = *ra = *dec = GSL_NAN;
        return;
    }

    // DE4xx gives us XYZ coordinates relative to the solar system's centre of mass
    // We need to know the Earth's position, in order to convert this to RA and Dec
    // Below are values of GM3 and GMM from DE405. See
    // <https://web.archive.org/web/20120220062549/http://iau-comm4.jpl.nasa.gov/de405iom/de405iom.pdf>
    const double earth_mass = 0.8887692390113509e-9;
    const double moon_mass = 0.1093189565989898e-10;
    const double moon_earth_mass_ratio = moon_mass / (moon_mass + earth_mass);

    // Look up the Earth-Moon centre of mass position
    jpl_computeXYZ(2, jd, &EMX, &EMY, &EMZ);

    // Look up the Moon's position relative to the E-M centre of mass
    jpl_computeXYZ(9, jd, &moon_pos_x, &moon_pos_y, &moon_pos_z);

    // Calculate the position of the Earth's centre of mass
    earth_pos_x = EMX - moon_earth_mass_ratio * moon_pos_x;
    earth_pos_y = EMY - moon_earth_mass_ratio * moon_pos_y;
    earth_pos_z = EMZ - moon_earth_mass_ratio * moon_pos_z;

    // Look up the Sun's position, taking light travel time into account
    {
        jpl_computeXYZ(10, jd, &sun_pos_x, &sun_pos_y, &sun_pos_z);

        // Calculate light travel time
        const double distance = gsl_hypot3(sun_pos_x - earth_pos_x,
                                           sun_pos_y - earth_pos_y,
                                           sun_pos_z - earth_pos_z); // AU
        const double light_travel_time = distance * GSL_CONST_MKSA_ASTRONOMICAL_UNIT / GSL_CONST_MKSA_SPEED_OF_LIGHT;

        // Look up position of requested object at the time the light left the object
        jpl_computeXYZ(10, jd - light_travel_time / 86400, &sun_pos_x, &sun_pos_y, &sun_pos_z);
    }

    // If the user's query was about the Earth, we already know its position
    if (is_earth) {
        *x = earth_pos_x;
        *y = earth_pos_y;
        *z = earth_pos_z;
    }

    // If the user's query was about the Sun, we already know that position too
    else if (is_sun) {
        *x = sun_pos_x;
        *y = sun_pos_y;
        *z = sun_pos_z;
    }

    // If the user's query was about the Moon, we already know that position too
    else if (is_moon) {
        *x = moon_pos_x + earth_pos_x;
        *y = moon_pos_y + earth_pos_y;
        *z = moon_pos_z + earth_pos_z;
    }

    // Otherwise we need to query DE4xx for the particular object the user was looking for,
    // taking light travel time into account
    else {
        // Calculate position of requested object at specified time
        jpl_computeXYZ(bodyId, jd, x, y, z);

        // Calculate light travel time
        const double distance = gsl_hypot3(*x - earth_pos_x, *y - earth_pos_y, *z - earth_pos_z); // AU
        const double light_travel_time = distance * GSL_CONST_MKSA_ASTRONOMICAL_UNIT / GSL_CONST_MKSA_SPEED_OF_LIGHT;

        // Look up position of requested object at the time the light left the object
        jpl_computeXYZ(bodyId, jd - light_travel_time / 86400, x, y, z);
    }

    // Look up the Earth-Moon centre of mass position, a short time in the future
    // We use this to calculate the Earth's velocity vector, which is needed to correct for aberration
    // (see eqn 7.119 of the Explanatory Supplement)
    const double eb_dot_timestep = 1e-6; // days
    const double eb_dot_timestep_sec = eb_dot_timestep * 86400;
    jpl_computeXYZ(2, jd + eb_dot_timestep, &EMX_future, &EMY_future, &EMZ_future);
    jpl_computeXYZ(9, jd + eb_dot_timestep, &moon_pos_x_future, &moon_pos_y_future, &moon_pos_z_future);
    earth_pos_x_future = EMX_future - moon_earth_mass_ratio * moon_pos_x_future;
    earth_pos_y_future = EMY_future - moon_earth_mass_ratio * moon_pos_y_future;
    earth_pos_z_future = EMZ_future - moon_earth_mass_ratio * moon_pos_z_future;

    // Equation (7.118) of the Explanatory Supplement - correct for aberration
    if (!is_earth) {
        const double u1[3] = {
            *x - earth_pos_x,
            *y - earth_pos_y,
            *z - earth_pos_z
        };
        const double u1_mag = gsl_hypot3(u1[0], u1[1], u1[2]);
        const double u[3] = {u1[0] / u1_mag, u1[1] / u1_mag, u1[2] / u1_mag};
        const double eb_dot[3] = {
            earth_pos_x_future - earth_pos_x,
            earth_pos_y_future - earth_pos_y,
            earth_pos_z_future - earth_pos_z
        };

        // Speed of light in AU per time step
        const double c = GSL_CONST_MKSA_SPEED_OF_LIGHT / GSL_CONST_MKSA_ASTRONOMICAL_UNIT * eb_dot_timestep_sec;
        const double V[3] = {eb_dot[0] / c, eb_dot[1] / c, eb_dot[2] / c};
        const double V_mag = gsl_hypot3(V[0], V[1], V[2]);
        const double beta = sqrt(1 - gsl_pow_2(V_mag));
        const double f1 = u[0] * V[0] + u[1] * V[1] + u[2] * V[2];
        const double f2 = 1 + f1 / (1 + beta);

        // Correct for aberration
        *x = earth_pos_x + (beta * u1[0] + f2 * u1_mag * V[0]) / (1 + f1);
        *y = earth_pos_y + (beta * u1[1] + f2 * u1_mag * V[1]) / (1 + f1);
        *z = earth_pos_z + (beta * u1[2] + f2 * u1_mag * V[2]) / (1 + f1);
    }

    // Populate other quantities, like the brightness, RA and Dec of the object, based on its XYZ position
    magnitudeEstimate(bodyId, *x, *y, *z, earth_pos_x, earth_pos_y, earth_pos_z, sun_pos_x, sun_pos_y, sun_pos_z, ra,
                      dec, mag, phase, angSize, phySize,
                      albedo, sunDist, earthDist, sunAngDist, theta_ESO, eclipticLongitude, eclipticLatitude,
                      eclipticDistance, ra_dec_epoch, jd,
                      do_topocentric_correction, topocentric_latitude, topocentric_longitude);
}

//! jpl_shutdown - Free malloced memory within this module

void jpl_shutdown() {
    if (JPL_ShapeData != NULL) free(JPL_ShapeData);
    if (JPL_EphemData != NULL) free(JPL_EphemData);
    if (JPL_EphemData_items_loaded != NULL) free(JPL_EphemData_items_loaded);
}
