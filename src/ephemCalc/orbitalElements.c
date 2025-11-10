// orbitalElements.c
// 
// -------------------------------------------------
// Copyright 2015-2025 Dominic Ford
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
#include <ctype.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <gsl/gsl_math.h>

#include "coreUtils/asciiDouble.h"
#include "coreUtils/errorReport.h"
#include "coreUtils/strConstants.h"

#include "mathsTools/julianDate.h"

#include "alias.h"
#include "jpl.h"
#include "orbitalElements.h"
#include "magnitudeEstimate.h"

// Maximum size of each catalogue of solar system objects
#define MAX_ASTEROIDS 1500000
#define MAX_COMETS     200000
#define MAX_PLANETS        50

#define DATA_IN_PATH SRCDIR "/../data/"
#define DATA_OUT_PATH SRCDIR "/../data/"

// Numerical constants
const static double ORBIT_CONST_SPEED_OF_LIGHT = 299792458.; // m/s
const static double ORBIT_CONST_ASTRONOMICAL_UNIT = 149597870700.; // m
const static double ORBIT_CONST_GM_SOLAR = 1.32712440041279419e20; // m^3 s^-2

// Utility functions for reading orbital elements from ASCII lines
int orbitalElements_readAsciiLine_planets(const char *line, orbitalElementSet *set, orbitalElements *output,
                                          char *name, char *name2);

int orbitalElements_readAsciiLine_asteroids(const char *line, orbitalElementSet *set, orbitalElements *output,
                                            char *name, char *name2);

int orbitalElements_readAsciiLine_comets(const char *line, orbitalElementSet *set, orbitalElements *output,
                                         char *name, char *name2);

// Binary files containing the orbital elements of solar system objects
orbitalElementSet planet_database = {
        "planet", 1, NULL,
        NULL,
        DATA_OUT_PATH "/binary_planets.bin",
        DATA_IN_PATH "/list_planet_files.txt",
        0, NULL,
        0, 0, DCFVERSION, "",
        MAX_PLANETS, -1, -1,
        -1, 0, 0, 0, 0,
        orbitalElements_readAsciiLine_planets
};

orbitalElementSet asteroid_database = {
        "asteroid", 0, NULL,
        NULL,
        DATA_OUT_PATH "/binary_asteroids.bin",
        DATA_IN_PATH "/list_astorb_files.txt",
        0, NULL,
        0, 0, DCFVERSION, "",
        MAX_ASTEROIDS, -1, -1,
        -1, 0, 0, 0, 0,
        orbitalElements_readAsciiLine_asteroids
};

orbitalElementSet comet_database = {
        "comet", 1, NULL,
        NULL,
        DATA_OUT_PATH "/binary_comet.bin",
        DATA_IN_PATH "/list_comet_files.txt",
        0, NULL,
        0, 0, DCFVERSION, "",
        MAX_COMETS, -1, -1,
        -1, 0, 0, 0, 0,
        orbitalElements_readAsciiLine_comets
};

//! orbitalElements_nullOrbitalElements - Create a null set of orbital elements, with fields set to NaN.
//! @return - Null set of orbital elements

orbitalElements orbitalElements_nullOrbitalElements() {
    const orbitalElements null_elements = {
            -1, 0,
            GSL_NAN, GSL_NAN,
            GSL_NAN, GSL_NAN,
            GSL_NAN, 0,
            GSL_NAN, 0,
            GSL_NAN, 0,
            GSL_NAN, 0,
            GSL_NAN, 0,
            2, -999
    };

    return null_elements;
}

//! orbitalElements_nullObjectNames - Create an object name structure with null contents.
//! @return - Null object name structure

objectNames orbitalElements_nullObjectNames() {
    const objectNames null_name = {
            -1, -1, "", ""
    };

    return null_name;
}

//! orbitalElements_writeHeaders - Dump the headers to an orbital element set to a binary file.
//!
//! \param [in] f - File handle for the binary file
//! \param [in] set - Descriptor for the set of orbital elements to be written.
//! \return - The file position of the end of the headers

long orbitalElements_writeHeaders(const orbitalElementSet *set) {
    FILE *f = set->binary_data_file;
    long pos = -1;

    // Check we don't have a local cache
    if (set->binary_file_cache != NULL) {
        ephem_fatal(__FILE__, __LINE__, "Attempting to update binary file after it has been cached");
        exit(1);
    }

#pragma omp critical (binary_file_access)
    {
        // Rewind to the beginning of the file
        fseek(f, 0, SEEK_SET);

        // Write the headers into the binary data file
        fwrite(&set->ready, sizeof(set->ready), 1, f);
        fwrite(&set->file_creation_epoch, sizeof(set->file_creation_epoch), 1, f);
        fwrite(&set->software_version, sizeof(set->software_version), 1, f);
        fwrite(&set->file_creation_hostname, sizeof(set->file_creation_hostname), 1, f);
        fwrite(&set->max_objects, sizeof(set->max_objects), 1, f);
        fwrite(&set->object_count, sizeof(set->object_count), 1, f);
        fwrite(&set->object_secure_count, sizeof(set->object_secure_count), 1, f);
        fwrite(&set->epoch_max_count, sizeof(set->epoch_max_count), 1, f);

        // Check the position of the end of the headers
        pos = ftell(f);
    }

    return pos;
}

//! orbitalElements_startWritingBinaryData - Dump orbital element set to a binary file, to save parsing original text
//! files every time we are run.
//!
//! \param [in|out] set - Descriptor for the set of orbital elements to be written.
//! \return - Zero on success

int orbitalElements_startWritingBinaryData(orbitalElementSet *set) {
    // Work out the full path of the binary data file we are to read
    if (DEBUG) {
        char msg[LSTR_LENGTH];
        sprintf(msg, "%ss: Starting to write binary data to file <%s>.",
                set->object_type, set->binary_filename);
        ephem_log(msg);
    }

    // Open binary data file
    set->binary_data_file = fopen(set->binary_filename, "w+b");
    if (set->binary_data_file == NULL) {
        return 1;
    }

    // Abbreviations
    const int n = set->max_objects;
    FILE *f = set->binary_data_file;

    // Write the headers into the binary data file
    set->offset_table_names = orbitalElements_writeHeaders(set);

    // We have now reached the table of booleans indicating which objects have secure orbits.
    // Store their offset from the start of the file.
    set->offset_table_secure = set->offset_table_names + n * sizeof(objectNames);

    // We have now reached the table of epochs per object. Store their offset from the start of the file.
    set->offset_table_epoch_counts = set->offset_table_secure + n * sizeof(int);

    // We have now reached the orbital element set. Store their offset from the start of the file.
    set->offset_table_orbital_elements = set->offset_table_epoch_counts + n * sizeof(int);

    // Expected position of the end of the file
    const long expected_file_end = (set->offset_table_orbital_elements +
                                    set->max_objects * set->epoch_max_count * sizeof(orbitalElements));

    if (DEBUG) {
        char buffer[LSTR_LENGTH];
        sprintf(buffer, "Expected file length %ld bytes.\n\
Max objects = %d\n\
Max epochs = %d\n\
Offset of object names table = %ld.\n\
Offset of secure orbits table = %ld.\n\
Offset of epoch counts table = %ld.\n\
Offset of orbital elements table = %ld.\n\
", expected_file_end, set->max_objects, set->epoch_max_count, set->offset_table_names, set->offset_table_secure,
                set->offset_table_epoch_counts, set->offset_table_orbital_elements);
        ephem_log(buffer);

    }

    // Create a null set of orbital elements
    const objectNames null_name = orbitalElements_nullObjectNames();
    const orbitalElements null_elements = orbitalElements_nullOrbitalElements();

#pragma omp critical (binary_file_access)
    {
        // Position file after headers
        fseek(f, set->offset_table_names, SEEK_SET);

        // Write a table of object names
        for (int i = 0; i < set->max_objects; i++) {
            fwrite(&null_name, sizeof(null_name), 1, f);
        }

        // Write a table of which objects have secure orbits
        {
            const long pos = ftell(f);
            if (pos != set->offset_table_secure) {
                char buffer[LSTR_LENGTH];
                sprintf(buffer, "Unexpected file position (%ld) when writing secure orbits table. Expected %ld.",
                        pos, set->offset_table_secure);
                ephem_warning(buffer);
            }
        }
        for (int i = 0; i < set->max_objects; i++) {
            const int zero = 0;
            fwrite(&zero, sizeof(zero), 1, f);
        }

        // Write a table of the number of epochs for each object
        {
            const long pos = ftell(f);
            if (pos != set->offset_table_epoch_counts) {
                char buffer[LSTR_LENGTH];
                sprintf(buffer,
                        "Unexpected file position (%ld) when writing epoch counts table. Expected %ld.",
                        pos, set->offset_table_epoch_counts);
                ephem_warning(buffer);
            }
        }
        for (int i = 0; i < set->max_objects; i++) {
            const int zero = 0;
            fwrite(&zero, sizeof(zero), 1, f);
        }

        // Write a table of NaN-filled orbital elements
        {
            const long pos = ftell(f);
            if (pos != set->offset_table_orbital_elements) {
                char buffer[LSTR_LENGTH];
                sprintf(buffer,
                        "Unexpected file position (%ld) when writing orbital elements table. Expected %ld.",
                        pos, set->offset_table_orbital_elements);
                ephem_warning(buffer);
            }
        }
        for (int i = 0; i < set->max_objects; i++) {
            for (int j = 0; j < set->epoch_max_count; j++) {
                fwrite(&null_elements, sizeof(orbitalElements), 1, f);
            }
        }

        // Check that the file has the expected length
        {
            const long pos = ftell(f);
            if (pos != expected_file_end) {
                char buffer[LSTR_LENGTH];
                sprintf(buffer,
                        "Unexpected file position (%ld) at the end of the file. Expected %ld.",
                        pos, expected_file_end);
                ephem_warning(buffer);
            }
        }

        // Finished
        if (DEBUG) {
            char msg[LSTR_LENGTH];
            sprintf(msg, "Binary file successfully initialised.");
            ephem_log(msg);
        }
    }

    return 0;
}

//! orbitalElements_binary_setObjectName - Set the object name for a specified object index.
//! \param [in] set - Descriptor for the set of orbital elements to be updated.
//! \param [in] object_index - Index of the object within the set of orbital elements.
//! \param [in] name - The name(s) for this object.

void orbitalElements_binary_setObjectName(orbitalElementSet *set, int object_index, objectNames *name) {
    // Check <object_index> is within range
    if ((object_index < 0) || (object_index >= set->max_objects)) {
        char buffer[LSTR_LENGTH];
        sprintf(buffer, "%ss: Illegal object index <%d>", set->object_type, object_index);
        ephem_fatal(__FILE__, __LINE__, buffer);
        exit(1);
    }

    // Check we don't have a local cache
    if (set->binary_file_cache != NULL) {
        ephem_fatal(__FILE__, __LINE__, "Attempting to update binary file after it has been cached");
        exit(1);
    }

    // Calculate position within the file to write to
    const long pos = set->offset_table_names + object_index * sizeof(objectNames);

#pragma omp critical (binary_file_access)
    {
        // Write to disk
        fseek(set->binary_data_file, pos, SEEK_SET);

        // Write data structure with this object's name
        fwrite(name, sizeof(*name), 1, set->binary_data_file);
    }
}

//! orbitalElements_binary_getSecureFlag - Fetch the flag indicating whether the specified object index has a secure
//! orbit.
//! \param [in] set - Descriptor for the set of orbital elements to be queried.
//! \param [in] object_index - Index of the object within the set of orbital elements.

int orbitalElements_binary_getSecureFlag(orbitalElementSet *set, int object_index) {
    int output = -123456;

    // Check <object_index> is within range
    if ((object_index < 0) || (object_index >= set->max_objects)) {
        char buffer[LSTR_LENGTH];
        sprintf(buffer, "%ss: Illegal object index <%d>", set->object_type, object_index);
        ephem_fatal(__FILE__, __LINE__, buffer);
        exit(1);
    }

    // Calculate position within the file to read from
    const long pos = set->offset_table_secure + object_index * sizeof(int);

    // Fetch value
    if (set->binary_file_cache != NULL) {
        // Fetch from in-memory copy
        memcpy(&output, set->binary_file_cache + pos, sizeof(output));
    } else {
#pragma omp critical (binary_file_access)
        {
            // Fetch from disk
            fseek(set->binary_data_file, pos, SEEK_SET);

            // Read secure flag
            dcf_fread(&output, sizeof(output), 1, set->binary_data_file,
                      set->binary_filename, __FILE__, __LINE__);
        }
    }

    // Sanity check value
    if ((output != 0) && (output != 1)) {
        char buffer[LSTR_LENGTH];
        sprintf(buffer, "Illegal value (%d) of secure orbit flag read for object %d from position %ld.",
                output, object_index, pos);
        ephem_warning(buffer);
    }

    // Return value
    return output;
}

//! orbitalElements_binary_setSecureFlag - Set the flag indicating whether the specified object index has a secure
//! orbit.
//! \param [in] set - Descriptor for the set of orbital elements to be updated.
//! \param [in] object_index - Index of the object within the set of orbital elements.
//! \param [in] secure_flag - Boolean flag indicating whether this object has a secure orbit.

void orbitalElements_binary_setSecureFlag(orbitalElementSet *set, int object_index, int secure_flag) {
    // Check <object_index> is within range
    if ((object_index < 0) || (object_index >= set->max_objects)) {
        char buffer[LSTR_LENGTH];
        sprintf(buffer, "%ss: Illegal object index <%d>", set->object_type, object_index);
        ephem_fatal(__FILE__, __LINE__, buffer);
        exit(1);
    }

    // Check we don't have a local cache
    if (set->binary_file_cache != NULL) {
        ephem_fatal(__FILE__, __LINE__, "Attempting to update binary file after it has been cached");
        exit(1);
    }

    // Calculate position within the file to write to
    const long pos = set->offset_table_secure + object_index * sizeof(int);

    // Sanity check value
    if ((secure_flag != 0) && (secure_flag != 1)) {
        char buffer[LSTR_LENGTH];
        sprintf(buffer,
                "Illegal value (%d) of secure orbit flag being written for object %d to position %ld.",
                secure_flag, object_index, pos);
        ephem_warning(buffer);
    }

#pragma omp critical (binary_file_access)
    {
        // Write to disk
        fseek(set->binary_data_file, pos, SEEK_SET);

        // Update secure flag
        fwrite(&secure_flag, sizeof(secure_flag), 1, set->binary_data_file);
    }
}

//! orbitalElements_binary_getEpochCount - Fetch the count of epochs at which orbital elements are available for an
//! object.
//! \param [in] set - Descriptor for the set of orbital elements to be queried.
//! \param [in] object_index - Index of the object within the set of orbital elements.

int orbitalElements_binary_getEpochCount(orbitalElementSet *set, int object_index) {
    int output = -123456;

    // Check <object_index> is within range
    if ((object_index < 0) || (object_index >= set->max_objects)) {
        char buffer[LSTR_LENGTH];
        sprintf(buffer, "%ss: Illegal object index <%d>", set->object_type, object_index);
        ephem_fatal(__FILE__, __LINE__, buffer);
        exit(1);
    }

    // Calculate position within the file to read from
    const long pos = set->offset_table_epoch_counts + object_index * sizeof(int);

    // Fetch value
    if (set->binary_file_cache != NULL) {
        // Fetch from in-memory copy
        memcpy(&output, set->binary_file_cache + pos, sizeof(output));
    } else {
#pragma omp critical (binary_file_access)
        {
            // Fetch from disk
            fseek(set->binary_data_file, pos, SEEK_SET);

            // Fetch epoch count
            dcf_fread(&output, sizeof(output), 1, set->binary_data_file,
                      set->binary_filename, __FILE__, __LINE__);
        }
    }

    // Sanity check value
    if ((output < 0) || (output > set->epoch_max_count)) {
        char buffer[LSTR_LENGTH];
        sprintf(buffer, "Illegal value (%d) of epoch count read for object %d from position %ld.",
                output, object_index, pos);
        ephem_warning(buffer);
    }

    // Return value
    return output;
}

//! orbitalElements_binary_setEpochCount - Set the count of epochs at which orbital elements are available for an
//! object.
//! \param [in] set - Descriptor for the set of orbital elements to be updated.
//! \param [in] object_index - Index of the object within the set of orbital elements.
//! \param [in] epoch_count - Count of epochs.

void orbitalElements_binary_setEpochCount(orbitalElementSet *set, int object_index, int epoch_count) {
    // Check <object_index> is within range
    if ((object_index < 0) || (object_index >= set->max_objects)) {
        char buffer[LSTR_LENGTH];
        sprintf(buffer, "%ss: Illegal object index <%d>", set->object_type, object_index);
        ephem_fatal(__FILE__, __LINE__, buffer);
        exit(1);
    }

    // Check we don't have a local cache
    if (set->binary_file_cache != NULL) {
        ephem_fatal(__FILE__, __LINE__, "Attempting to update binary file after it has been cached");
        exit(1);
    }

    // Calculate position within the file to write to
    const long pos = set->offset_table_epoch_counts + object_index * sizeof(int);

    // Sanity check value
    if ((epoch_count < 0) || (epoch_count > set->epoch_max_count)) {
        char buffer[LSTR_LENGTH];
        sprintf(buffer, "Illegal value (%d) of epoch count being written for object %d to position %ld.",
                epoch_count, object_index, pos);
        ephem_warning(buffer);
    }

#pragma omp critical (binary_file_access)
    {
        // Write to disk
        fseek(set->binary_data_file, pos, SEEK_SET);

        // Update epoch count
        fwrite(&epoch_count, sizeof(epoch_count), 1, set->binary_data_file);
    }
}

//! orbitalElements_binary_getElements - Fetch an array of orbital elements for an object at various epochs
//! \param [in] set - Descriptor for the set of orbital elements to be queried.
//! \param [in] object_index - Index of the object within the set of orbital elements.
//! \param [out] output - Storage space into which to write the output <orbitalElements> structures.
//! \param [in] buffer_size - The maximum number of <orbitalElements> structures which can be written to <output>.
//! \return - The number of <orbitalElements> structures returned.

int orbitalElements_binary_getElements(orbitalElementSet *set, int object_index,
                                       orbitalElements *output, int buffer_size) {
    // Check <object_index> is within range
    if ((object_index < 0) || (object_index >= set->max_objects)) {
        char buffer[LSTR_LENGTH];
        sprintf(buffer, "%ss: Illegal object index <%d>", set->object_type, object_index);
        ephem_fatal(__FILE__, __LINE__, buffer);
        exit(1);
    }

    // Look up how many sets of orbital elements we already have for this object
    const int epoch_count = orbitalElements_binary_getEpochCount(set, object_index);

    // Work out the number of elements to read
    const int elements_to_read = (int) gsl_min(buffer_size, epoch_count);

    // Calculate position within the binary file to read elements from
    const long stride = set->epoch_max_count * sizeof(orbitalElements);
    const long pos = set->offset_table_orbital_elements + object_index * stride;

    // Fetch value
    if (set->binary_file_cache != NULL) {
        // Fetch from in-memory copy
        memcpy(output, set->binary_file_cache + pos, sizeof(orbitalElements) * elements_to_read);
    } else {
#pragma omp critical (binary_file_access)
        {
            // Fetch from disk
            fseek(set->binary_data_file, pos, SEEK_SET);

            // Read orbital elements
            dcf_fread(output, sizeof(orbitalElements), elements_to_read, set->binary_data_file,
                      set->binary_filename, __FILE__, __LINE__);
        }
    }

    // Return the number of orbital elements structures we read
    return elements_to_read;
}

//! orbitalElements_binary_writeElements - Write an array of <orbitalElements> structures for an object.
//! \param [in] set - Descriptor for the set of orbital elements to be updated.
//! \param [in] object_index - Index of the object within the set of orbital elements.
//! \param [in] output - Storage space from which to read the <orbitalElements> structures to write to disk.
//! \param [in] element_count - The number of <orbitalElements> structures in <output> (at various epochs).

void orbitalElements_binary_writeElements(orbitalElementSet *set, int object_index,
                                          orbitalElements *output, int element_count) {
    // Check <object_index> is within range
    if ((object_index < 0) || (object_index >= set->max_objects)) {
        char buffer[LSTR_LENGTH];
        sprintf(buffer, "%ss: Illegal object index <%d>", set->object_type, object_index);
        ephem_fatal(__FILE__, __LINE__, buffer);
        exit(1);
    }

    // Check we don't have a local cache
    if (set->binary_file_cache != NULL) {
        ephem_fatal(__FILE__, __LINE__, "Attempting to update binary file after it has been cached");
        exit(1);
    }

    // Calculate position within the binary file to read elements from
    const long stride = set->epoch_max_count * sizeof(orbitalElements);
    const long pos = set->offset_table_orbital_elements + object_index * stride;

#pragma omp critical (binary_file_access)
    {
        // Write to disk
        fseek(set->binary_data_file, pos, SEEK_SET);

        // Write orbital elements
        fwrite(output, sizeof(orbitalElements), element_count, set->binary_data_file);
    }

    // Update epoch count
    orbitalElements_binary_setEpochCount(set, object_index, element_count);
}

//! orbitalElements_binary_appendElements - Append a set of orbital elements to the array of <orbitalElements>
//! structures available for an object.
//! \param [in] set - Descriptor for the set of orbital elements to be queried.
//! \param [in] object_index - Index of the object within the set of orbital elements.
//! \param [in] elements - The <orbitalElements> structure to append to the array of epochs available for this object.

void orbitalElements_binary_appendElements(orbitalElementSet *set, int object_index, orbitalElements *elements) {
    // Look up how many sets of orbital elements we already have for this object
    const int epoch_count = orbitalElements_binary_getEpochCount(set, object_index);

    // Check <object_index> is within range
    if ((object_index < 0) || (object_index >= set->max_objects)) {
        char buffer[LSTR_LENGTH];
        sprintf(buffer, "%ss: Illegal object index <%d>", set->object_type, object_index);
        ephem_fatal(__FILE__, __LINE__, buffer);
        exit(1);
    }

    // Check <epoch_count> is within range
    if ((epoch_count < 0) || (epoch_count >= set->epoch_max_count)) {
        char buffer[LSTR_LENGTH];
        sprintf(buffer, "%ss: Illegal epoch index <%d>", set->object_type, epoch_count);
        ephem_fatal(__FILE__, __LINE__, buffer);
        exit(1);
    }

    // Check we don't have a local cache
    if (set->binary_file_cache != NULL) {
        ephem_fatal(__FILE__, __LINE__, "Attempting to update binary file after it has been cached");
        exit(1);
    }

    // Calculate position within the binary file to write elements to
    const long stride = set->epoch_max_count * sizeof(orbitalElements);
    const long pos = (set->offset_table_orbital_elements
                      + object_index * stride
                      + epoch_count * sizeof(orbitalElements)
    );

#pragma omp critical (binary_file_access)
    {
        // Write to disk
        fseek(set->binary_data_file, pos, SEEK_SET);

        // Add orbital elements to file
        fwrite(elements, sizeof(*elements), 1, set->binary_data_file);
    }

    // Increment epoch count
    orbitalElements_binary_setEpochCount(set, object_index, epoch_count + 1);
}

//! orbitalElements_binary_epochExists - Test whether a specified epoch of osculation already exists in the orbital
//! elements for a specific object.
//! \param [in] set - The descriptor for the set of orbital elements we are reading
//! \param [in] object_index - Index of the object within the set of orbital elements.
//! \param [in] epoch - The epoch to search for within the existing orbital elements.
//! \return - Boolean flag indicating whether this epoch is already present.

int orbitalElements_binary_epochExists(orbitalElementSet *set, int object_index, double epoch) {
    // Create a temporary workspace for fetching a single object's elements
    const int buffer_size = set->epoch_max_count;
    orbitalElements *buffer = (orbitalElements *) malloc(buffer_size * sizeof(orbitalElements));
    if (buffer == NULL) {
        ephem_fatal(__FILE__, __LINE__, "Malloc fail.");
        exit(1);
    }

    // Fetch all the existing elements available for this object
    const int n = orbitalElements_binary_getElements(set, object_index, buffer, buffer_size);

    // Check whether any of the existing epochs match
    int got_match = 0;
    for (int i = 0; i < n; i++) {
        if (buffer[i].epochOsculation == epoch) {
            got_match = 1;
            break;
        }
    }

    // Free temporary workspace
    free(buffer);

    // Return the boolean flag
    return got_match;
}

//! orbitalElements_searchByObjectName - Search for an existing object with matching name
//! @param [in] set - The descriptor for the set of orbital elements we are reading
//! @param [in] name - The name of the object to search for
//! @param [out] last_seen_file_index - The file index at which this name was last seen
//! @return - The number of the matching object, or -1 for no match

int orbitalElements_searchByObjectName(const orbitalElementSet *set, const char *name, int *last_seen_file_index) {
    if (last_seen_file_index != NULL) {
        *last_seen_file_index = -1;
    }

    // If we don't have a table of object names, give up immediately
    if (!set->match_by_name) return -1;

    // Search for objects whose <name> or <name2> fields match the supplied name
    for (int object_index = 0; object_index < set->object_count; object_index++) {
        if ((str_cmp_no_case(name, set->object_names[object_index].name) == 0) ||
            (str_cmp_no_case(name, set->object_names[object_index].name2) == 0)) {
            if (last_seen_file_index != NULL) {
                *last_seen_file_index = set->object_names[object_index].last_seen_file_index;
            }
            return object_index;
        }
    }

    // No match was found
    return -1;
}
//! orbitalElements_maximumEpochCount - Count how many ASCII input files we will process, to determine the maximum
//! number of epochs of data an object might have.
//! @param [in] set - The descriptor for the set of orbital elements we are reading
//! @return - The number of ASCII input files we will process

int orbitalElements_maximumEpochCount(const orbitalElementSet *set) {
    int output = 0;

    if (DEBUG) {
        char msg[LSTR_LENGTH];
        sprintf(msg, "%ss: Opening file ASCII list of orbital elements files <%s>",
                set->object_type, set->ascii_filename_catalogue);
        ephem_log(msg);
    }

    // Open catalogue of files containing orbital elements
    FILE *input = fopen(set->ascii_filename_catalogue, "rt");
    if (input == NULL) {
        char buffer[LSTR_LENGTH];
        sprintf(buffer, "%ss: Could not open input ASCII data file <%s>",
                set->object_type, set->ascii_filename_catalogue);
        ephem_fatal(__FILE__, __LINE__, buffer);
        exit(1);
    }

    // Read through the input ASCII file, line by line
    while ((!feof(input)) && (!ferror(input))) {
        char line[LSTR_LENGTH];

        // Read a line from the input file
        file_readline(input, line, sizeof line);
        str_strip(line, line);

        // Ignore blank lines and comment lines
        if (line[0] == '\0') continue;
        if (line[0] == '#') continue;

        // Count this file
        output++;
    }

    // Close input ASCII file
    fclose(input);

    // Return the number of files we counted
    return output;
}

//! orbitalElements_sortByEpoch - Objective function for using qsort to sort orbital elements by epoch of osculation
//! @param [in] p - First set of orbital elements
//! @param [in] q - Second set of orbital elements
//! @return - Integer indicating the sorting direction

int orbitalElements_sortByEpoch(const void *p, const void *q) {
    const double x = ((const orbitalElements *) p)->epochOsculation;
    const double y = ((const orbitalElements *) q)->epochOsculation;

    if (x < y) {
        return -1; // Return -1 if you want ascending, 1 if you want descending order.
    }
    if (x > y) {
        return 1; // Return 1 if you want ascending, -1 if you want descending order.
    }
    return 0;
}

//! orbitalElements_readAsciiLine_planets - Read a line of an ASCII file containing orbital elements for planets
//! @param line [in] - The line of text containing the orbital elements
//! @param set [in|out] - The descriptor for the set of orbital elements we are reading
//! @param output [out] - The orbital elements that we read
//! @param name [out] - The name of the object that we read
//! @param name2 [out] - The alternative name of the object that we read
//! @return - The number of objects we read (0 or 1)

int orbitalElements_readAsciiLine_planets(const char *line, orbitalElementSet *set, orbitalElements *output,
                                          char *name, char *name2) {
    int i, j;

    // Initialise name outputs
    name[0] = '\0';
    name2[0] = '\0';

    // Ignore blank lines and comment lines
    if (line[0] == '\0') return 0;
    if (line[0] == '#') return 0;
    if (strlen(line) < 168) return 0;

    // Read body id
    const int body_id = (int) get_float(line, NULL);
    output->number = body_id;

    // Read planet name
    for (i = 166, j = 0; (line[i] > ' '); i++, j++) name[j] = line[i];
    name[j] = '\0';

    // Fill out dummy information
    output->absoluteMag = 999;
    output->slopeParam_G = 2;
    output->secureOrbit = 1;

    // Now start reading orbital elements of the planet

    // Read semi-major axis of orbit -- AU
    for (i = 18; line[i] == ' '; i++);
    output->semiMajorAxis = get_float(line + i, NULL);

    // Read rate of change of semi-major axis of orbit -- convert <AU per century> to <AU per day>
    for (i = 30; line[i] == ' '; i++);
    output->semiMajorAxis_dot = get_float(line + i, NULL) / 36525.;

    // Read eccentricity of orbit -- dimensionless
    for (i = 42; line[i] == ' '; i++);
    output->eccentricity = get_float(line + i, NULL);

    // Read rate of change of eccentricity of orbit -- convert <per century> into <per day>
    for (i = 53; line[i] == ' '; i++);
    output->eccentricity_dot = get_float(line + i, NULL) / 36525.;

    // Read longitude of ascending node -- convert <degrees> to <radians; J2000.0>
    for (i = 65; line[i] == ' '; i++);
    output->longAscNode = get_float(line + i, NULL) * M_PI / 180;

    // Read rate of change of longitude of ascending node -- convert <degrees/century> into <radians/day>
    for (i = 79; line[i] == ' '; i++);
    output->longAscNode_dot = get_float(line + i, NULL) / 36525. * M_PI / 180;

    // Read inclination of orbit -- convert <degrees> to <radians; J2000.0>
    for (i = 91; line[i] == ' '; i++);
    output->inclination = get_float(line + i, NULL) * M_PI / 180;

    // Read rate of change of inclination -- convert <degrees/century> into <radians/day>
    for (i = 104; line[i] == ' '; i++);
    output->inclination_dot = get_float(line + i, NULL) / 36525. * M_PI / 180;

    // Read longitude of perihelion -- convert <degrees> to <radians; J2000.0>
    for (i = 116; line[i] == ' '; i++);
    const double longitude_perihelion = get_float(line + i, NULL) * M_PI / 180;

    // Read rate of change of longitude of perihelion -- convert <degrees/century> into <radians/day>
    for (i = 129; line[i] == ' '; i++);
    const double longitude_perihelion_dot = get_float(line + i, NULL) / 36525. * M_PI / 180;

    // Read mean longitude -- convert <degrees> to <radians; J2000.0>
    for (i = 141; line[i] == ' '; i++);
    const double mean_longitude = get_float(line + i, NULL) * M_PI / 180;

    // Read epoch of osculation -- convert <unix time> to <julian date>
    for (i = 155; line[i] == ' '; i++);
    output->epochOsculation = jd_from_unix(get_float(line + i, NULL));

    // Calculate mean anomaly -- radians; J2000.0
    output->meanAnomaly = mean_longitude - longitude_perihelion;

    // Calculate argument of perihelion -- radians; J2000.0
    output->argumentPerihelion = longitude_perihelion - output->longAscNode;

    // Calculate the rate of change of argument of perihelion -- radians per day
    output->argumentPerihelion_dot = (longitude_perihelion_dot - output->longAscNode_dot);

    return 1;
}

//! orbitalElements_readAsciiLine_asteroids - Read a line of an ASCII file containing orbital elements for asteroids
//! @param line [in] - The line of text containing the orbital elements
//! @param set [in|out] - The descriptor for the set of orbital elements we are reading
//! @param output [out] - The orbital elements that we read
//! @param name [out] - The name of the object that we read
//! @param name2 [out] - The alternative name of the object that we read
//! @return - The number of objects we read (0 or 1)

int orbitalElements_readAsciiLine_asteroids(const char *line, orbitalElementSet *set, orbitalElements *output,
                                            char *name, char *name2) {
    int i;

    // Initialise name outputs
    name[0] = '\0';
    name2[0] = '\0';

    // Ignore blank lines and comment lines
    if (line[0] == '\0') return 0;
    if (line[0] == '#') return 0;
    if (strlen(line) < 250) return 0;

    // Read asteroid number
    for (i = 0; (line[i] > '\0') && (line[i] <= ' '); i++);

    // Unnumbered asteroid; don't bother adding to catalogue
    if (i >= 6) return 0;

    // Read asteroid number
    const int asteroid_number = (int) get_float(line + i, NULL);

    // asteroid_count should be the highest number asteroid we have encountered
    output->number = asteroid_number;

    // Read asteroid name
    for (i = 25; (i > 7) && (line[i] > '\0') && (line[i] <= ' '); i--);
    strncpy(name, line + 7, i - 6);
    name[i - 6] = '\0';

    // Read absolute magnitude
    for (i = 42; (line[i] > '\0') && (line[i] <= ' '); i++);
    output->absoluteMag = get_float(line + i, NULL);

    // Read slope parameter
    for (i = 48; (line[i] > '\0') && (line[i] <= ' '); i++);
    output->slopeParam_G = get_float(line + i, NULL);

    // Read number of days spanned by data used to derive orbit
    int day_obs_span;
    {
        int j;
        char buffer[8];
        snprintf(buffer, 7, "%s", line + 94);
        for (j = 0; (buffer[j] > '\0') && (buffer[j] <= ' '); j++);
        day_obs_span = (int) get_float(buffer + j, NULL);
    }

    // Read the number of observations used to derive orbit
    for (i = 100; (line[i] > '\0') && (line[i] <= ' '); i++);
    const int obs_count = (int) get_float(line + i, NULL);

    // Orbit deemed secure if more than 10 yrs data
    output->secureOrbit = (day_obs_span > 3650) && (obs_count > 500);

    // Now start reading orbital elements of the asteroid
    {
        for (i = 106; (line[i] > '\0') && (line[i] <= ' '); i++);
        const double tmp = get_float(line + i, NULL);
        // julian date
        output->epochOsculation = julian_day(
                (int) floor(tmp / 10000), ((int) floor(tmp / 100)) % 100,
                ((int) floor(tmp)) % 100, 0, 0, 0, &i, temp_err_string);
    }

    // Read mean anomaly -- radians; J2000.0
    for (i = 115; (line[i] > '\0') && (line[i] <= ' '); i++);
    output->meanAnomaly = get_float(line + i, NULL) * M_PI / 180;

    // Read argument of perihelion -- radians; J2000.0
    for (i = 126; (line[i] > '\0') && (line[i] <= ' '); i++);
    output->argumentPerihelion = get_float(line + i, NULL) * M_PI / 180;

    // Read longitude of ascending node -- radians; J2000.0
    for (i = 137; (line[i] > '\0') && (line[i] <= ' '); i++);
    output->longAscNode = get_float(line + i, NULL) * M_PI / 180;

    // Read inclination of orbit -- radians; J2000.0
    for (i = 147; (line[i] > '\0') && (line[i] <= ' '); i++);
    output->inclination = get_float(line + i, NULL) * M_PI / 180;

    // Read eccentricity of orbit -- dimensionless
    for (i = 157; (line[i] > '\0') && (line[i] <= ' '); i++);
    output->eccentricity = get_float(line + i, NULL);

    // Read semi-major axis of orbit -- AU
    for (i = 168; (line[i] > '\0') && (line[i] <= ' '); i++);
    output->semiMajorAxis = get_float(line + i, NULL);

    return 1;
}

//! orbitalElements_readAsciiLine_comets - Read a line of an ASCII file containing orbital elements for comets
//! @param line [in] - The line of text containing the orbital elements
//! @param set [in|out] - The descriptor for the set of orbital elements we are reading
//! @param output [out] - The orbital elements that we read
//! @param name [out] - The name of the object that we read
//! @param name2 [out] - The alternative name of the object that we read
//! @return - The number of objects we read (0 or 1)

int orbitalElements_readAsciiLine_comets(const char *line, orbitalElementSet *set, orbitalElements *output,
                                         char *name, char *name2) {
    int j, k;

    // Initialise name outputs
    name[0] = '\0';
    name2[0] = '\0';

    // Ignore blank lines and comment lines
    if (line[0] == '\0') return 0;
    if (line[0] == '#') return 0;
    if (strlen(line) < 100) return 0;

    // Read comet name
    for (j = 102, k = 0; (line[j] != '(') && (line[j] != '\0') && (k < 23); j++, k++) {
        name[k] = line[j];
    }
    while ((k > 0) && (name[--k] == ' '));
    name[k + 1] = '\0';

    // Read comet's MPC designation
    for (j = 0, k = 0; (line[j] > '\0') && (line[j] <= ' '); j++);
    while ((line[j] > ' ') && (k < 23)) name2[k++] = line[j++];
    name2[k] = '\0';

    // If comet's MPC designation has a fragment letter, append that now (e.g. 0073P-AA)
    const char fragment_letter_0 = line[10];
    const char fragment_letter_1 = line[11];
    if (isalpha(fragment_letter_1) && (k > 0) && ((name2[k - 1] == 'P') || (name2[k - 1] == 'I'))) {
        name2[k++] = '-';
        if (isalpha(fragment_letter_0)) {
            name2[k++] = (char) toupper(fragment_letter_0);
        }
        name2[k++] = (char) toupper(fragment_letter_1);
        name2[k] = '\0';
    }

    // Read perihelion distance
    const double perihelion_dist = get_float(line + 31, NULL);

    // Read perihelion date
    for (j = 14; (line[j] > '\0') && (line[j] <= ' '); j++);
    const int perihelion_year = (int) get_float(line + j, NULL);
    for (j = 19; (line[j] > '\0') && (line[j] <= ' '); j++);
    const int perihelion_month = (int) get_float(line + j, NULL);
    for (j = 22; (line[j] > '\0') && (line[j] <= ' '); j++);
    const double perihelion_day = get_float(line + j, NULL);

    // julian date
    const double perihelion_date = julian_day(
            perihelion_year, perihelion_month, (int) floor(perihelion_day),
            ((int) floor(perihelion_day * 24)) % 24,
            ((int) floor(perihelion_day * 24 * 60)) % 60,
            ((int) floor(perihelion_day * 24 * 3600)) % 60,
            &j, temp_err_string);

    // Read eccentricity of orbit
    for (j = 41; (line[j] > '\0') && (line[j] <= ' '); j++);
    const double eccentricity = output->eccentricity = get_float(line + j, NULL);

    // Read argument of perihelion, radians, J2000.0
    for (j = 51; (line[j] > '\0') && (line[j] <= ' '); j++);
    output->argumentPerihelion = get_float(line + j, NULL) * M_PI / 180;

    // Read longitude of ascending node, radians, J2000.0
    for (j = 61; (line[j] > '\0') && (line[j] <= ' '); j++);
    output->longAscNode = get_float(line + j, NULL) * M_PI / 180;

    // Read orbital inclination, radians, J2000.0
    for (j = 71; (line[j] > '\0') && (line[j] <= ' '); j++);
    output->inclination = get_float(line + j, NULL) * M_PI / 180;

    // Read epoch of osculation, julian date
    for (j = 81; (line[j] > '\0') && (line[j] <= ' '); j++);
    const double tmp = get_float(line + j, NULL);
    const double epoch = output->epochOsculation = julian_day((int) floor(tmp / 10000),
                                                              ((int) floor(tmp / 100)) % 100,
                                                              ((int) floor(tmp)) % 100, 0, 0, 0,
                                                              &j,
                                                              temp_err_string);

    // Read absolute magnitude
    for (j = 90; (line[j] > '\0') && (line[j] <= ' '); j++);
    if (!valid_float(line + j, NULL)) output->absoluteMag = GSL_NAN;
    else output->absoluteMag = get_float(line + j, NULL);

    // Read slope parameter
    for (j = 96; (line[j] > '\0') && (line[j] <= ' '); j++);
    if (!valid_float(line + j, NULL)) output->slopeParam_n = 2;
    else output->slopeParam_n = get_float(line + j, NULL);

    // Calculate derived quantities
    output->secureOrbit = 1;
    // AU
    const double a = output->semiMajorAxis = perihelion_dist / (1 - eccentricity);
    // radians; J2000.0
    output->meanAnomaly = fmod(
            sqrt(ORBIT_CONST_GM_SOLAR /
                 gsl_pow_3(fabs(a) * ORBIT_CONST_ASTRONOMICAL_UNIT)) * (epoch - perihelion_date) * 24 * 3600 +
            100 * M_PI, 2 * M_PI);
    // julian date
    output->epochPerihelion = perihelion_date;

    return 1;
}

//! orbitalElements_readAsciiDataFile - Read the orbital elements contained in a single ASCII file
//! @param [in] set - The descriptor for the set of orbital elements we are reading
//! @param [in] filename - The filename of the ASCII file we are to read
//! @param [in] file_index - The unique index of the ASCII file we are to read
//! @return - None

void orbitalElements_readAsciiDataFile(orbitalElementSet *set, const char *filename, const int file_index) {
    if (DEBUG) {
        char msg[LSTR_LENGTH];
        sprintf(msg, "Opening file ASCII orbital elements file <%s>", filename);
        ephem_log(msg);
    }

    FILE *input = fopen(filename, "rt");
    if (input == NULL) {
        char msg[LSTR_LENGTH];
        sprintf(msg, "Could not open input ASCII data file <%s>", filename);
        ephem_fatal(__FILE__, __LINE__, msg);
        exit(1);
    }

    // Read through the input ASCII file, line by line
    int lines_read = 0;
    int objects_read = 0;
    int duplicate_epochs = 0;

    while ((!feof(input)) && (!ferror(input))) {
        char line[LSTR_LENGTH];
        orbitalElements elements = orbitalElements_nullOrbitalElements();
        char name[OBJECT_NAME_LENGTH], name2[OBJECT_NAME_LENGTH];

        // Read a line from the input file
        file_readline(input, line, sizeof line);
        lines_read++;

        // Parse line of the ASCII file
        const int status = set->ascii_reader(line, set, &elements, name, name2);
        if (status == 0) continue;
        objects_read++;

        // Work out object number
        int object_index = -1;
        if (elements.number >= 0) {
            object_index = elements.number;
            if (object_index >= set->object_count) {
                set->object_count = object_index + 1;
            }
        } else if (set->match_by_name) {
            int name_last_seen_at_file_index = -1;

            if ((object_index < 0) && (strlen(name) > 0)) {
                object_index = orbitalElements_searchByObjectName(set, name, &name_last_seen_at_file_index);
            }
            if ((object_index < 0) && (strlen(name2) > 0)) {
                object_index = orbitalElements_searchByObjectName(set, name2, &name_last_seen_at_file_index);
            }

            if (name_last_seen_at_file_index == file_index) {
                char msg[LSTR_LENGTH];
                sprintf(msg, "Duplicate orbital elements found for object <%s> / <%s>", name, name2);
                ephem_log(msg);
                continue;
            }
        }
        if (object_index < 0) {
            object_index = set->object_count;
            set->object_count++;
        }

        // Check if these elements are a duplicate of an epoch we have already seen
        if (!orbitalElements_binary_epochExists(set, object_index, elements.epochOsculation)) {
            // Write elements to the binary file
            orbitalElements_binary_appendElements(set, object_index, &elements);
        } else {
            duplicate_epochs++;
        }

        // Write object name to the binary file
        objectNames name_structure;
        name_structure.number = object_index;
        name_structure.last_seen_file_index = file_index;
        snprintf(name_structure.name, OBJECT_NAME_LENGTH, "%s", name);
        snprintf(name_structure.name2, OBJECT_NAME_LENGTH, "%s", name2);
        orbitalElements_binary_setObjectName(set, object_index, &name_structure);
        if (set->object_names != NULL) {
            set->object_names[object_index] = name_structure;
        }

        // Update secure orbit flag
        if (elements.secureOrbit) {
            orbitalElements_binary_setSecureFlag(set, object_index, 1);
        }
    }

    // Close input ASCII file
    fclose(input);

    // Report outcomes
    if (DEBUG) {
        char msg[LSTR_LENGTH];
        sprintf(msg, "Line count       = %7d", lines_read);
        ephem_log(msg);
        sprintf(msg, "Object count     = %7d", objects_read);
        ephem_log(msg);
        sprintf(msg, "Duplicate epochs = %7d", duplicate_epochs);
        ephem_log(msg);
    }
}


//! orbitalElements_readAsciiDataFiles - Read the orbital elements contained in a set of ASCII files
//! @param [in] set - The descriptor for the set of orbital elements we are reading
//! @return - None

void orbitalElements_readAsciiDataFiles(orbitalElementSet *set) {
    // Reset counters of how many objects we have
    set->ready = 0;
    set->object_count = 0;
    set->object_secure_count = 0;
    set->epoch_max_count = orbitalElements_maximumEpochCount(set);
    set->file_creation_epoch = (double) time(NULL);
    snprintf(set->software_version, sizeof(set->software_version), "%s", DCFVERSION);
    gethostname(set->file_creation_hostname, sizeof(set->file_creation_hostname));
    set->file_creation_hostname[sizeof(set->file_creation_hostname) - 1] = '\0';
    if (set->binary_file_cache != NULL) free(set->binary_file_cache);
    set->binary_file_cache = NULL;

    // Start writing the binary output file
    {
        int status = orbitalElements_startWritingBinaryData(set);
        if (status) return;
    }

    // Allocate buffer for storing object names
    if (set->object_names != NULL) free(set->object_names);
    if (set->match_by_name) {
        // Abbreviations
        const int n = set->max_objects;
        FILE *f = set->binary_data_file;
        const char *fn = set->binary_filename;

        set->object_names = (objectNames *) malloc(n * sizeof(objectNames));
        if (set->object_names == NULL) {
            ephem_fatal(__FILE__, __LINE__, "Malloc fail.");
            exit(1);
        }

        // Read table of names into memory
#pragma omp critical (binary_file_access)
        {
            fseek(set->binary_data_file, set->offset_table_names, SEEK_SET);
            dcf_fread(set->object_names, sizeof(objectNames), n, f, fn, __FILE__, __LINE__);
        }
    }

    // Open catalogue of files containing orbital elements
    FILE *input = fopen(set->ascii_filename_catalogue, "rt");
    if (input == NULL) {
        char buffer[LSTR_LENGTH];
        sprintf(buffer, "%ss: Could not open input ASCII data file <%s>",
                set->object_type, set->ascii_filename_catalogue);
        ephem_fatal(__FILE__, __LINE__, buffer);
        exit(1);
    }

    // Read through the input ASCII file, line by line
    int file_counter = 0;
    while ((!feof(input)) && (!ferror(input))) {
        char line[LSTR_LENGTH];

        // Read a line from the input file
        file_readline(input, line, sizeof line);
        str_strip(line, line);

        // Ignore blank lines and comment lines
        if (line[0] == '\0') continue;
        if (line[0] == '#') continue;

        // Process this file
        orbitalElements_readAsciiDataFile(set, line, file_counter);
        file_counter++;
    }

    // Close input ASCII file
    fclose(input);

    // Sort orbital elements into ascending order of epoch of osculation
    {
        if (DEBUG) {
            ephem_log("Sorting orbital elements into order is ascending epoch of osculation.");
        }

        // Create a temporary workspace for sorting a single object's elements
        const int buffer_size = set->epoch_max_count;
        orbitalElements *buffer = (orbitalElements *) malloc(buffer_size * sizeof(orbitalElements));
        if (buffer == NULL) {
            ephem_fatal(__FILE__, __LINE__, "Malloc fail.");
            exit(1);
        }

        // Sort the elements for each object in turn
        for (int object_index = 0; object_index < set->object_count; object_index++) {
            const int n = orbitalElements_binary_getElements(set, object_index, buffer, buffer_size);
            qsort(buffer, n, sizeof(orbitalElements), orbitalElements_sortByEpoch);
            orbitalElements_binary_writeElements(set, object_index, buffer, n);
        }

        // On request, output debugging information about the catalogue
        if (0 && DEBUG) {
            for (int object_index = 0; object_index < set->object_count; object_index++) {
                const int n = orbitalElements_binary_getElements(set, object_index, buffer, buffer_size);
                const char *nameA = (set->match_by_name) ? (set->object_names[object_index].name) : "null";
                const char *nameB = (set->match_by_name) ? (set->object_names[object_index].name2) : "null";
                const double t0 = buffer[0].epochOsculation;
                const double t1 = buffer[1].epochOsculation;
                const double t2 = buffer[2].epochOsculation;

                char msg[LSTR_LENGTH];
                sprintf(msg, "%8d | %4d | %4ld <%s> | %4ld <%s> | %.2f %.2f %.2f", object_index, n,
                        strlen(nameA), nameA, strlen(nameB), nameB, t0, t1, t2);
                ephem_log(msg);
            }
        }

        // Free temporary workspace
        free(buffer);
    }

    // Calculate the number of objects with secure orbits
    for (int object_index = 0; object_index < set->object_count; object_index++) {
        const int secure = orbitalElements_binary_getSecureFlag(set, object_index);
        if (secure) set->object_secure_count++;
    }

    // Write final headers
    set->ready = 1;
    orbitalElements_writeHeaders(set);

    // Close binary file
    fclose(set->binary_data_file);

    // Report outcomes
    if (DEBUG) {
        char msg[LSTR_LENGTH];
        sprintf(msg, "%ss: Object count                = %8d", set->object_type, set->object_count);
        ephem_log(msg);
        sprintf(msg, "%ss: Objects with secure orbits  = %8d", set->object_type, set->object_secure_count);
        ephem_log(msg);
    }
}

//! orbitalElements_readBinaryData - Restore orbital element set from a binary data file which holds the compressed
//! contents of the ASCII input data files. This saves time parsing the original text file every time we are run. For
//! further efficiency, we don't read the orbital element set from disk straight away, until they're actually needed.
//! This massively reduces the start-up time and memory footprint.
//!
//! \param [in|out] set - Descriptor for the set of orbital elements to read.
//! \return - Zero on success

int orbitalElements_readBinaryData(orbitalElementSet *set) {
    long binary_file_size = 0;

    // Work out the full path of the binary data file we are to read
    if (DEBUG) {
        char msg[LSTR_LENGTH];
        sprintf(msg, "Fetching binary data from file <%s>.", set->binary_filename);
        ephem_log(msg);
    }

    // Open binary data file
    set->binary_data_file = fopen(set->binary_filename, "rb");
    if (set->binary_data_file == NULL) {
        // Read from ASCII
        orbitalElements_readAsciiDataFiles(set);

        // Retry opening binary data file
        set->binary_data_file = fopen(set->binary_filename, "rb");
        if (set->binary_data_file == NULL) {
            // Give up
            return 1;
        }
    }

    // Abbreviations
    const int n = set->max_objects;
    FILE *f = set->binary_data_file;
    const char *fn = set->binary_filename;

#pragma omp critical (binary_file_access)
    {
        // Rewind to the beginning of the file
        fseek(f, 0, SEEK_SET);

        // Read the flags from the beginning of the binary file
        dcf_fread(&set->ready, sizeof(set->ready), 1, f, fn, __FILE__, __LINE__);
        dcf_fread(&set->file_creation_epoch, sizeof(set->file_creation_epoch), 1, f, fn, __FILE__, __LINE__);
        dcf_fread(&set->software_version, sizeof(set->software_version), 1, f, fn, __FILE__, __LINE__);
        dcf_fread(&set->file_creation_hostname, sizeof(set->file_creation_hostname), 1, f, fn, __FILE__,
                  __LINE__);
        dcf_fread(&set->max_objects, sizeof(set->max_objects), 1, f, fn, __FILE__, __LINE__);
        dcf_fread(&set->object_count, sizeof(set->object_count), 1, f, fn, __FILE__, __LINE__);
        dcf_fread(&set->object_secure_count, sizeof(set->object_secure_count), 1, f, fn, __FILE__, __LINE__);
        dcf_fread(&set->epoch_max_count, sizeof(set->epoch_max_count), 1, f, fn, __FILE__, __LINE__);

        if (DEBUG) {
            char msg[LSTR_LENGTH], buffer[FNAME_LENGTH];
            time_t t = (time_t) set->file_creation_epoch;

            sprintf(msg, "Elements for <%ss> created on <%s> at <%s> using software version <%s>",
                    set->object_type, set->file_creation_hostname,
                    str_strip(ctime(&t), buffer), set->software_version
            );
            ephem_log(msg);
            sprintf(msg, "%ss: Max objects = %d", set->object_type, set->max_objects);
            ephem_log(msg);
            sprintf(msg, "%ss: Object count = %d", set->object_type, set->object_count);
            ephem_log(msg);
            sprintf(msg, "%ss: Objects with secure orbits = %d", set->object_type, set->object_secure_count);
            ephem_log(msg);
            sprintf(msg, "%ss: Max epochs = %d", set->object_type, set->epoch_max_count);
            ephem_log(msg);
        }

        // We have now reached the table of object names
        set->offset_table_names = ftell(f);

        // We have now reached the table of booleans indicating which objects have secure orbits.
        // Store their offset from the start of the file.
        set->offset_table_secure = set->offset_table_names + n * sizeof(objectNames);

        // We have now reached the table of epochs per object. Store their offset from the start of the file.
        set->offset_table_epoch_counts = set->offset_table_secure + n * sizeof(int);

        // We have now reached the orbital element set. Store their offset from the start of the file.
        set->offset_table_orbital_elements = set->offset_table_epoch_counts + n * sizeof(int);

        // Expected position of the end of the file
        const long expected_file_end = (set->offset_table_orbital_elements +
                                        set->max_objects * set->epoch_max_count * sizeof(orbitalElements));

        // Check that the length of the file matches what we expect
        fseek(set->binary_data_file, 0L, SEEK_END);
        binary_file_size = ftell(set->binary_data_file);

        if (binary_file_size != expected_file_end) {
            char buffer[LSTR_LENGTH];
            sprintf(buffer,
                    "Unexpected binary file length (%ld). Expected %ld.",
                    binary_file_size, expected_file_end);
            ephem_warning(buffer);
        }

        // Allocate buffer for storing object names
        if (set->object_names != NULL) free(set->object_names);
        if (set->match_by_name) {
            set->object_names = (objectNames *) malloc(n * sizeof(objectNames));
            if (set->object_names == NULL) {
                ephem_fatal(__FILE__, __LINE__, "Malloc fail.");
                exit(1);
            }

            // Read table of names into memory
            fseek(set->binary_data_file, set->offset_table_names, SEEK_SET);
            dcf_fread(set->object_names, sizeof(objectNames), n, f, fn, __FILE__, __LINE__);
        }
    }

    // Check that numbers are sensible
    if ((set->object_count < 1) || (set->object_count > MAX_ALLOWED_OBJECTS)) {
        if (DEBUG) { ephem_log("Rejecting this as implausible"); }
        fclose(set->binary_data_file);
        set->binary_data_file = NULL;
        return 1;
    }

    // If requested, cache the entire binary file in memory
    if (set->binary_file_cache != NULL) free(set->binary_file_cache);

    if (set->cache_in_memory) {
        set->binary_file_cache = malloc(binary_file_size);
        if (set->binary_file_cache == NULL) {
            ephem_fatal(__FILE__, __LINE__, "Malloc fail.");
            exit(1);
        }

#pragma omp critical (binary_file_access)
        {
            // Rewind to the beginning of the file
            fseek(f, 0, SEEK_SET);

            // Read table of names into memory
            dcf_fread(set->binary_file_cache, binary_file_size, 1, f, fn, __FILE__, __LINE__);
        }
    }

    // Logging update
    if (DEBUG) {
        char msg[LSTR_LENGTH];
        sprintf(msg, "Data file opened successfully.");
        ephem_log(msg);
    }

    // Success
    return 0;
}

//! orbitalElements_planets_init - Make sure that planet orbital elements are initialised, in thread-safe fashion

void orbitalElements_planets_init() {
#pragma omp critical (planets_init)
    {
        if (planet_database.binary_data_file == NULL) orbitalElements_readBinaryData(&planet_database);
    }
}

//! orbitalElements_asteroids_init - Make sure that asteroid orbital elements are initialised, in thread-safe fashion

void orbitalElements_asteroids_init(const int load_names) {
#pragma omp critical (asteroids_init)
    {
        if (asteroid_database.binary_data_file == NULL) {
            asteroid_database.match_by_name |= load_names;
            orbitalElements_readBinaryData(&asteroid_database);
        }
    }
}

//! orbitalElements_comets_init - Make sure that comet orbital elements are initialised, in thread-safe fashion

void orbitalElements_comets_init() {
#pragma omp critical (comets_init)
    {
        if (comet_database.binary_data_file == NULL) orbitalElements_readBinaryData(&comet_database);
    }
}

//! orbitalElements_fetch - Fetch the orbitalElements record for bodyId <object_index>. If needed, load them from disk.
//! \param [in] set_index - Index of the set of orbital elements to read. 0=planets; 1=asteroids; 2=comets.
//! \param [in] object_index - The index of the object within the set of orbital elements.
//! \param [in] epoch_requested - The JD for which orbital elements are required.
//! \param [out] output_1 - The 1st set of orbital elements to be used in linear interpolation.
//! \param [out] weight_1 - The weighting of the 1st set of orbital elements.
//! \param [out] output_2 - The 2nd set of orbital elements to be used in linear interpolation.
//! \param [out] weight_2 - The weighting of the 2nd set of orbital elements.
//! \return - The number of orbital elements found (0, 1 or 2)

int orbitalElements_fetch(const int set_index, const int object_index, const double epoch_requested,
                          orbitalElements *output_1, double *weight_1,
                          orbitalElements *output_2, double *weight_2
) {
    // Fetch the set of orbital elements to operate on.
    orbitalElementSet *set;
    switch (set_index) {
        case 0:
            orbitalElements_planets_init();
            set = &planet_database;
            break;
        case 1:
            orbitalElements_asteroids_init(0);
            set = &asteroid_database;
            break;
        case 2:
            orbitalElements_comets_init();
            set = &comet_database;
            break;
        default:
            ephem_fatal(__FILE__, __LINE__, "Illegal set_index.");
            exit(1);
    }

    // Check that request is within the allowed range
    if ((object_index < 0) || (object_index >= set->object_count)) return 0;

    // Create a temporary workspace for cycling through a single object's elements
    const int buffer_size = set->epoch_max_count;
    orbitalElements *buffer = (orbitalElements *) malloc(buffer_size * sizeof(orbitalElements));
    if (buffer == NULL) {
        ephem_fatal(__FILE__, __LINE__, "Malloc fail.");
        exit(1);
    }

    // Fetch all the available elements
    const int n = orbitalElements_binary_getElements(set, object_index, buffer, buffer_size);

    // Request falls before first epoch_requested
    if (epoch_requested <= buffer[0].epochOsculation) {
        // Logging update
        if (DEBUG) {
            char msg[LSTR_LENGTH], t_str[FNAME_LENGTH];
            time_t t = (time_t) unix_from_jd(buffer[0].epochOsculation);
            sprintf(msg, "%d epochs available. Using first epoch_requested: <%s>.",
                    n, str_strip(ctime(&t), t_str));
            ephem_log(msg);
        }

        // Output orbital elements
        *output_1 = buffer[0];
        *weight_1 = 1;
        *weight_2 = 0;

        // Free temporary workspace
        free(buffer);
        return 1;
    }

    // Request falls after final epoch_requested
    if (epoch_requested >= buffer[n - 1].epochOsculation) {
        // Logging update
        if (DEBUG) {
            char msg[LSTR_LENGTH], t_str[FNAME_LENGTH];
            const time_t t = (time_t) unix_from_jd(buffer[n - 1].epochOsculation);
            sprintf(msg, "%d epochs available. Using last epoch_requested: <%s>.",
                    n, str_strip(ctime(&t), t_str));
            ephem_log(msg);
        }

        // Output orbital elements
        *output_1 = buffer[n - 1];
        *weight_1 = 1;
        *weight_2 = 0;

        // Free temporary workspace
        free(buffer);
        return 1;
    }

    // Interpolate between two epochs
    int i;
    for (i = 1; i < n; i++) {
        if (buffer[i].epochOsculation > epoch_requested) break;
    }

    // Logging update
    if (DEBUG) {
        char msg[LSTR_LENGTH], t0_str[FNAME_LENGTH], t1_str[FNAME_LENGTH];
        const time_t t0 = (time_t) unix_from_jd(buffer[i - 1].epochOsculation);
        const time_t t1 = (time_t) unix_from_jd(buffer[i].epochOsculation);
        sprintf(msg, "%d epochs available. Interpolating between %d and %d. Epochs are: <%s> and <%s>.",
                n, i - 1, i, str_strip(ctime(&t0), t0_str), str_strip(ctime(&t1), t1_str));
        ephem_log(msg);
    }

    // Set weighting parameters for linear interpolation
    const double t0 = buffer[i - 1].epochOsculation;
    const double t1 = buffer[i].epochOsculation;
    const double gap = gsl_max(1e-6, t1 - t0);

    // Output orbital elements
    *weight_1 = fabs(t1 - epoch_requested) / gap;
    *output_1 = buffer[i - 1];
    *weight_2 = fabs(t0 - epoch_requested) / gap;
    *output_2 = buffer[i];

    // Free temporary workspace
    free(buffer);
    return 2;
}

//! orbitalElements_computeXYZ_fromElements - Main orbital elements computer - solves Kepler's equation to calculate
//! an object's 3D position using a single set of orbital elements. Return 3D position in ICRF, in AU, relative to the
//! Sun (not the solar system barycentre!!). z-axis points towards the J2000.0 north celestial pole.
//!
//! \param [in] orbital_elements - The set of orbital elements to use for the object
//! \param [in] jd - The Julian day number at which the object's position is wanted; TT
//! \param [out] x - The x position of the object relative to the Sun (in AU; ICRF; points to RA=0)
//! \param [out] y - The y position of the object relative to the Sun (in AU; ICRF; points to RA=6h)
//! \param [out] z - The z position of the object relative to the Sun (in AU; ICRF; points to NCP)

void orbitalElements_computeXYZ_fromElements(const orbitalElements *orbital_elements, const double jd,
                                             double *x, double *y, double *z) {
    double v, r;

    // Extract orbital elements from structure
    const double offset_from_epoch = jd - orbital_elements->epochOsculation;
    const double a = orbital_elements->semiMajorAxis + orbital_elements->semiMajorAxis_dot * offset_from_epoch;
    const double e = orbital_elements->eccentricity + orbital_elements->eccentricity_dot * offset_from_epoch;
    const double N = orbital_elements->longAscNode + orbital_elements->longAscNode_dot * offset_from_epoch;
    const double inc = orbital_elements->inclination + orbital_elements->inclination_dot * offset_from_epoch;
    const double w = orbital_elements->argumentPerihelion + (orbital_elements->argumentPerihelion_dot *
                                                             offset_from_epoch);

    // Mean Anomaly for desired epoch (convert rate of change per second into rate of change per day)
    const double mean_motion = sqrt(ORBIT_CONST_GM_SOLAR /
                                    gsl_pow_3(fabs(a) * ORBIT_CONST_ASTRONOMICAL_UNIT));

    const double M = orbital_elements->meanAnomaly +
                     (jd - orbital_elements->epochOsculation) * mean_motion * 24 * 3600;


    // When debugging, show intermediate calculation
    if (DEBUG) {
        char msg[LSTR_LENGTH];
        sprintf(msg, "Object ID = %d", orbital_elements->number);
        ephem_log(msg);
        sprintf(msg, "JD = %.5f", jd);
        ephem_log(msg);
    }

    if (e > 1.02) {
        // Hyperbolic orbit
        // See <https://stjarnhimlen.se/comp/ppcomp.html> section 20

        const double M_hyperbolic = (jd - orbital_elements->epochPerihelion) * mean_motion * 24 * 3600;

        double F0, F1, ratio = 0.5;

        // Initial guess
        F0 = M_hyperbolic;

        // Iteratively solve Kepler's equation
        for (int j = 0; ((j < 100) && (ratio > 1e-12)); j++) {
            F1 = (M_hyperbolic + e * (F0 * cosh(F0) - sinh(F0))) / (e * cosh(F0) - 1); // Newton's method
            ratio = fabs(F1 / F0);
            if (ratio < 1) ratio = 1 / ratio;
            ratio -= 1;
            F0 = F1;
        }

        v = 2 * atan(sqrt((e + 1) / (e - 1)) * tanh(F0 / 2));
        r = a * (1 - e * e) / (1 + e * cos(v));
    } else if (e < 0.98) {
        int j;
        double E0, E1, delta_E = 1;
        E0 = M + e * sin(M);

        // Iteratively solve inverse Kepler's equation for eccentric anomaly
        for (j = 0; ((j < 100) && (fabs(delta_E) > 1e-12)); j++) {
            // See Explanatory Supplement to the Astronomical Almanac, eq 8.37
            const double delta_M = M - (E0 - e * sin(E0));
            delta_E = delta_M / (1 - e * cos(E0));
            E1 = E0 + delta_E;
            E0 = E1;
        }

        const double xv = a * (cos(E0) - e);
        const double yv = a * (sqrt(1 - gsl_pow_2(e)) * sin(E0));

        v = atan2(yv, xv);
        r = sqrt(gsl_pow_2(xv) + gsl_pow_2(yv));

        // When debugging, show intermediate calculation
        if (DEBUG) {
            char msg[LSTR_LENGTH];
            sprintf(msg, "E0 = %.10f deg", E0 * 180 / M_PI);
            ephem_log(msg);
            sprintf(msg, "delta_E = %.10e deg", delta_E * 180 / M_PI);
            ephem_log(msg);
            sprintf(msg, "xv = %.10f km", xv * ORBIT_CONST_ASTRONOMICAL_UNIT / 1e3);
            ephem_log(msg);
            sprintf(msg, "yv = %.10f km", yv * ORBIT_CONST_ASTRONOMICAL_UNIT / 1e3);
            ephem_log(msg);
            sprintf(msg, "j = %d iterations", j);
            ephem_log(msg);
        }
    } else {
        // Near-parabolic orbit
        // See <https://stjarnhimlen.se/comp/ppcomp.html> section 19

        const double k = 0.01720209895;
        const double q = a * (1 - e);
        const double ddt = (jd - orbital_elements->epochPerihelion);
        const double A = 0.75 * ddt * k * sqrt((1 + e) / (gsl_pow_3(q)));
        const double B = sqrt(1 + A * A);
        const double W = pow(B + A, 0.333333333) - pow(B - A, 0.33333333);
        const double F = (1 - e) / (1 + e);
        const double A1 = (2. / 3) + (2. / 5) * gsl_pow_2(W);
        const double A2 = (7. / 5) + (33. / 35) * gsl_pow_2(W) + (37. / 175) * gsl_pow_4(W);
        const double A3 = gsl_pow_2(W) * ((432. / 175) + (956. / 1125) * gsl_pow_2(W) + (84. / 1575) * gsl_pow_4(W));
        const double C = gsl_pow_2(W) / (1 + gsl_pow_2(W));
        const double G = F * gsl_pow_2(C);
        const double w = W * (1 + F * C * (A1 + A2 * G + A3 * gsl_pow_2(G)));
        v = 2 * atan(w);
        r = q * (1 + gsl_pow_2(w)) / (1 + gsl_pow_2(w) * F);
    }

    // Position of object relative to the Sun, in ecliptic coordinates (Eq 8.34)
    const double xh_j2000 = r * (cos(N) * cos(v + w) - sin(N) * sin(v + w) * cos(inc));
    const double yh_j2000 = r * (sin(N) * cos(v + w) + cos(N) * sin(v + w) * cos(inc));
    const double zh_j2000 = r * (sin(v + w) * sin(inc));

    // Inclination of the ecliptic at J2000.0 epoch
    const double epsilon = 23.4392794444 * M_PI / 180;

    // Transfer ecliptic coordinates (relative to Sun) into J2000.0 coordinates (i.e. ICRF)
    *x = xh_j2000;
    *y = yh_j2000 * cos(epsilon) - zh_j2000 * sin(epsilon);
    *z = yh_j2000 * sin(epsilon) + zh_j2000 * cos(epsilon);

    // When debugging, show intermediate calculation
    if (DEBUG) {
        char msg[LSTR_LENGTH];
        sprintf(msg, "a = %.10e km", a * ORBIT_CONST_ASTRONOMICAL_UNIT / 1e3);
        ephem_log(msg);
        sprintf(msg, "e = %.10f", e);
        ephem_log(msg);
        sprintf(msg, "N = %.10f deg", N * 180 / M_PI);
        ephem_log(msg);
        sprintf(msg, "inc = %.10f deg", inc * 180 / M_PI);
        ephem_log(msg);
        sprintf(msg, "w = %.10f deg", w * 180 / M_PI);
        ephem_log(msg);
        sprintf(msg, "mean_motion = %.10e deg/sec", mean_motion * 180 / M_PI);
        ephem_log(msg);
        sprintf(msg, "M = %.10f deg (mean anomaly)", M * 180 / M_PI);
        ephem_log(msg);
        sprintf(msg, "v = %.10f deg", v * 180 / M_PI);
        ephem_log(msg);
        sprintf(msg, "r = %.10f km", r * ORBIT_CONST_ASTRONOMICAL_UNIT / 1e3);
        ephem_log(msg);
    }
}

//! orbitalElements_computeXYZ - Main orbital elements computer. Return 3D position in ICRF, in AU, relative to the
//! Sun (not the solar system barycentre!!). z-axis points towards the J2000.0 north celestial pole.
//! \param [in] body_id - The id number of the object whose position is being queried
//! \param [in] jd - The Julian day number at which the object's position is wanted; TT
//! \param [out] x - The x position of the object relative to the Sun (in AU; ICRF; points to RA=0)
//! \param [out] y - The y position of the object relative to the Sun (in AU; ICRF; points to RA=6h)
//! \param [out] z - The z position of the object relative to the Sun (in AU; ICRF; points to NCP)

void orbitalElements_computeXYZ(int body_id, double jd, double *x, double *y, double *z) {
    orbitalElements orbital_elements_0 = orbitalElements_nullOrbitalElements();
    orbitalElements orbital_elements_1 = orbitalElements_nullOrbitalElements();
    double weight_0 = 0;
    double weight_1 = 0;
    int element_count = 0;

    // Fetch the orbital elements we are to use
    if (body_id < ASTEROIDS_OFFSET) {
        // Case 1: Object is a planet
        // Planets occupy body numbers 1-19
        const int index = body_id;

        orbitalElements_planets_init();

        // Return NaN if the object is not in the database
        if ((planet_database.binary_data_file == NULL) || (index >= planet_database.object_count)) {
            if (DEBUG) {
                char buffer[LSTR_LENGTH];
                sprintf(buffer, "Unrecognised planet index <%d>", index);
                ephem_log(buffer);
            }
            *x = *y = *z = GSL_NAN;
            return;
        }

        // Fetch data from the binary database file
        element_count = orbitalElements_fetch(
                0, index, jd,
                &orbital_elements_0, &weight_0, &orbital_elements_1, &weight_1);
    } else if (body_id < COMETS_OFFSET) {
        // Case 2: Object is an asteroid
        // Asteroids occupy body numbers 1e7 - 2e7
        const int index = body_id - ASTEROIDS_OFFSET;

        orbitalElements_asteroids_init(0);

        // Return NaN if the object is not in the database
        if ((asteroid_database.binary_data_file == NULL) || (index >= asteroid_database.object_count)) {
            if (DEBUG) {
                char buffer[LSTR_LENGTH];
                sprintf(buffer, "Unrecognised asteroid index <%d>", index);
                ephem_log(buffer);
            }
            *x = *y = *z = GSL_NAN;
            return;
        }

        // Fetch data from the binary database file
        element_count = orbitalElements_fetch(
                1, index, jd,
                &orbital_elements_0, &weight_0, &orbital_elements_1, &weight_1);
    } else {
        // Case 3: Object is a comet
        // Comets occupy body numbers 2e7 - 3e7
        const int index = body_id - COMETS_OFFSET;

        orbitalElements_comets_init();

        // Return NaN if the object is not in the database
        if ((comet_database.binary_data_file == NULL) || (index >= comet_database.object_count)) {
            if (DEBUG) {
                char buffer[LSTR_LENGTH];
                sprintf(buffer, "Unrecognised comet index <%d>", index);
                ephem_log(buffer);
            }
            *x = *y = *z = GSL_NAN;
            return;
        }

        // Fetch data from the binary database file
        element_count = orbitalElements_fetch(
                2, index, jd,
                &orbital_elements_0, &weight_0, &orbital_elements_1, &weight_1);
    }

    // If we didn't get any orbital elements, abort now
    if (element_count < 1) {
        if (DEBUG) {
            char buffer[LSTR_LENGTH];
            sprintf(buffer, "No orbital elements returned for object <%d>", body_id);
            ephem_log(buffer);
        }
        *x = *y = *z = GSL_NAN;
        return;
    }

    // If we only have one set of orbital elements, pass these to the Kepler equation solver
    if (element_count == 1) {
        orbitalElements_computeXYZ_fromElements(&orbital_elements_0, jd, x, y, z);
        return;
    }

    // If we are doing linear interpolation, do that now
    double x0, y0, z0;
    double x1, y1, z1;
    orbitalElements_computeXYZ_fromElements(&orbital_elements_0, jd, &x0, &y0, &z0);
    orbitalElements_computeXYZ_fromElements(&orbital_elements_1, jd, &x1, &y1, &z1);

    *x = weight_0 * x0 + weight_1 * x1;
    *y = weight_0 * y0 + weight_1 * y1;
    *z = weight_0 * z0 + weight_1 * z1;
}

//! orbitalElements_computeEphemeris - Main entry point for estimating the position, brightness, etc of an object at
//! a particular time, using orbital elements.
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

void orbitalElements_computeEphemeris(int bodyId, const double jd, double *x, double *y, double *z, double *ra,
                                      double *dec, double *mag, double *phase, double *angSize, double *phySize,
                                      double *albedo, double *sunDist, double *earthDist, double *sunAngDist,
                                      double *theta_eso, double *eclipticLongitude, double *eclipticLatitude,
                                      double *eclipticDistance, const double ra_dec_epoch,
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

    // Earth: Need to convert from Earth/Moon barycentre to geocentre
    if (bodyId == 19) {
        bodyId = 2;
        is_earth = 1;
    }

    // Moon:  Position returned relative to geocentre, not solar system barycentre
    if (bodyId == 9) {
        is_moon = 1;
    }

    // Sun
    if (bodyId == 10) {
        is_sun = 1;
    }

    // Look up position of the Earth at this JD, so that we can convert XYZ coordinates relative to Sun into
    // RA and Dec as observed from the Earth.
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
        const double light_travel_time = distance * ORBIT_CONST_ASTRONOMICAL_UNIT / ORBIT_CONST_SPEED_OF_LIGHT;

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

        // Otherwise we need to use the orbital elements for the particular object the user was looking for,
        // taking light travel time into account
    else {
        double x_from_sun, y_from_sun, z_from_sun;

        // Calculate position of requested object at specified time (relative to Sun)
        orbitalElements_computeXYZ(bodyId, jd, &x_from_sun, &y_from_sun, &z_from_sun);

        // Convert to barycentric coordinates (to match DE430's coordinate system)
        const double x_barycentric_0 = x_from_sun + sun_pos_x;
        const double y_barycentric_0 = y_from_sun + sun_pos_y;
        const double z_barycentric_0 = z_from_sun + sun_pos_z;

        // Calculate light travel time
        const double distance = gsl_hypot3(x_barycentric_0 - earth_pos_x,
                                           y_barycentric_0 - earth_pos_y,
                                           z_barycentric_0 - earth_pos_z); // AU
        const double light_travel_time = distance * ORBIT_CONST_ASTRONOMICAL_UNIT / ORBIT_CONST_SPEED_OF_LIGHT;

        // Look up position of requested object at the time the light left the object
        orbitalElements_computeXYZ(bodyId, jd - light_travel_time / 86400,
                                   &x_from_sun, &y_from_sun, &z_from_sun);
        const double x_barycentric_1 = x_from_sun + sun_pos_x;
        const double y_barycentric_1 = y_from_sun + sun_pos_y;
        const double z_barycentric_1 = z_from_sun + sun_pos_z;

        // Store result
        *x = x_barycentric_1;
        *y = y_barycentric_1;
        *z = z_barycentric_1;
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
        const double c = ORBIT_CONST_SPEED_OF_LIGHT / ORBIT_CONST_ASTRONOMICAL_UNIT * eb_dot_timestep_sec;
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
                      albedo, sunDist, earthDist, sunAngDist, theta_eso, eclipticLongitude, eclipticLatitude,
                      eclipticDistance, ra_dec_epoch, jd,
                      do_topocentric_correction, topocentric_latitude, topocentric_longitude);
}

void orbitalElements_shutdown() {
    if (planet_database.object_names != NULL) free(planet_database.object_names);
    if (asteroid_database.object_names != NULL) free(asteroid_database.object_names);
    if (comet_database.object_names != NULL) free(comet_database.object_names);

    if (planet_database.binary_file_cache != NULL) free(planet_database.binary_file_cache);
    if (asteroid_database.binary_file_cache != NULL) free(asteroid_database.binary_file_cache);
    if (comet_database.binary_file_cache != NULL) free(comet_database.binary_file_cache);

    if (planet_database.binary_data_file != NULL) fclose(planet_database.binary_data_file);
    if (asteroid_database.binary_data_file != NULL) fclose(asteroid_database.binary_data_file);
    if (comet_database.binary_data_file != NULL) fclose(comet_database.binary_data_file);

    planet_database.object_names = NULL;
    asteroid_database.object_names = NULL;
    comet_database.object_names = NULL;

    planet_database.binary_file_cache = NULL;
    asteroid_database.binary_file_cache = NULL;
    comet_database.binary_file_cache = NULL;

    planet_database.binary_data_file = NULL;
    asteroid_database.binary_data_file = NULL;
    comet_database.binary_data_file = NULL;

    planet_database.ready = 0;
    asteroid_database.ready = 0;
    comet_database.ready = 0;
}

//! orbitalElements_searchByEphemerisName - Search for an object's integer bodyId from its name
//! @param name [in] - The name of the object to search for
//! @return - The bodyId of the matching object, or -1 for no match

int orbitalElements_searchBodyIdByObjectName(const char *name_in) {
    char name[FNAME_LENGTH];

    // Convert the name into stripped lowercase
    strncpy(name, name_in, FNAME_LENGTH);
    name[FNAME_LENGTH - 1] = '\0';
    str_strip(name, name);
    str_lower(name, name);

    // Match planets
    if ((strcmp(name, "mercury") == 0) || (strcmp(name, "pmercury") == 0) || (strcmp(name, "p1") == 0))
        return 0;
    if ((strcmp(name, "venus") == 0) || (strcmp(name, "pvenus") == 0) || (strcmp(name, "p2") == 0))
        return 1;
    if ((strcmp(name, "earth") == 0) || (strcmp(name, "pearth") == 0) || (strcmp(name, "p3") == 0))
        return 19;
    if ((strcmp(name, "mars") == 0) || (strcmp(name, "pmars") == 0) || (strcmp(name, "p4") == 0))
        return 3;
    if ((strcmp(name, "jupiter") == 0) || (strcmp(name, "pjupiter") == 0) || (strcmp(name, "p5") == 0))
        return 4;
    if ((strcmp(name, "saturn") == 0) || (strcmp(name, "psaturn") == 0) || (strcmp(name, "p6") == 0))
        return 5;
    if ((strcmp(name, "uranus") == 0) || (strcmp(name, "puranus") == 0) || (strcmp(name, "p7") == 0))
        return 6;
    if ((strcmp(name, "neptune") == 0) || (strcmp(name, "pneptune") == 0) || (strcmp(name, "p8") == 0))
        return 7;
    if ((strcmp(name, "pluto") == 0) || (strcmp(name, "ppluto") == 0) || (strcmp(name, "p9") == 0))
        return 8;
    if ((strcmp(name, "moon") == 0) || (strcmp(name, "pmoon") == 0) || (strcmp(name, "p301") == 0))
        return 9;
    if (strcmp(name, "sun") == 0)
        return 10;
    if (((name[0] == 'a') || (name[0] == 'A')) && valid_float(name + 1, NULL)) {
        // Asteroid, e.g. A1
        return ASTEROIDS_OFFSET + (int) get_float(name + 1, NULL);
    }
    if (((name[0] == 'c') || (name[0] == 'C')) && valid_float(name + 1, NULL)) {
        // Comet, e.g. C1 (first in datafile)
        return COMETS_OFFSET + (int) get_float(name + 1, NULL);
    }

    // Search for comets with matching names

    // Open comet database
    orbitalElements_comets_init();
    if (!comet_database.match_by_name) return -1;

    // Loop over comets seeing if names match
    for (int object_index = 0; object_index < comet_database.object_count; object_index++) {
        if ((str_cmp_no_case(name, comet_database.object_names[object_index].name) == 0) ||
            (str_cmp_no_case(name, comet_database.object_names[object_index].name2) == 0)) {
            return COMETS_OFFSET + object_index;
        }
    }

    // No match was found
    return -1;
}
