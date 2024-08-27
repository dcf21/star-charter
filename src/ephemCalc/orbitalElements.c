// orbitalElements.c
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

#define ORBITALELEMENTS_C 1

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include <gsl/gsl_const_mksa.h>
#include <gsl/gsl_math.h>

#include "coreUtils/asciiDouble.h"
#include "coreUtils/errorReport.h"
#include "coreUtils/strConstants.h"

#include "listTools/ltMemory.h"

#include "mathsTools/julianDate.h"

#include "alias.h"
#include "jpl.h"
#include "orbitalElements.h"
#include "magnitudeEstimate.h"

// Binary files containing the orbital elements of solar system objects
FILE *planet_database_file = NULL;
FILE *asteroid_database_file = NULL;
FILE *comet_database_file = NULL;

// Filenames of binary files
char planet_database_filename[FNAME_LENGTH];
char asteroid_database_filename[FNAME_LENGTH];
char comet_database_filename[FNAME_LENGTH];

// Blocks of memory used to hold the orbital elements
orbitalElements *planet_database = NULL;
orbitalElements *asteroid_database = NULL;
orbitalElements *comet_database = NULL;

// Record of which entries we have loaded from each database
unsigned char *planet_database_items_loaded = NULL;
unsigned char *asteroid_database_items_loaded = NULL;
unsigned char *comet_database_items_loaded = NULL;

// Pointers to the positions in the files where the orbitalElement structures begin
int planet_database_offset = -1;
int asteroid_database_offset = -1;
int comet_database_offset = -1;

// Number of objects in each list
int planet_count = 0;
int asteroid_count = 0;
int comet_count = 0;

// Number of objects with securely determined orbits
int planet_secure_count = 0;
int asteroid_secure_count = 0;
int comet_secure_count = 0;

//! OrbitalElements_ReadBinaryData - restore orbital elements from a binary dump of the data in a file such as
//! <data/dcfbinary.ast>. This saves time parsing original text file every time we are run. For further efficiency,
//! we don't actually read the orbital elements from disk straight away, until they're actually needed. We merely
//! malloc a buffer to hold them. This massively reduces the start-up time.
//!
//! \param [in] filename - The filename of the binary data dump
//! \param [out] file_pointer - Return a file handle for the binary data dump
//! \param [out] elements_offset  - Return the offset of the start of the table of <orbitalElements> structures from the
//! beginning of the file.
//! \param [out] data_buffer - Return a malloced buffer which is big enough to contain the table of <orbitalElements>
//! structures.
//! \param [out] data_buffer_items_loaded - Return an array which we use to keep track of which orbital elements we have
//! already loaded.
//! \param [out] item_count - Return the number of orbital elements in this binary file.
//! \param [out] item_secure_count - Return the number of securely determined orbital elements in this binary file.
//! \return - Zero on success

int OrbitalElements_ReadBinaryData(const char *filename, FILE **file_pointer, int *elements_offset,
                                   orbitalElements **data_buffer, unsigned char **data_buffer_items_loaded,
                                   int *item_count, int *item_secure_count) {
    char filename_with_path[FNAME_LENGTH];

    // Work out the full path of the binary data file we are to read
    sprintf(filename_with_path, "%s/../data/%s", SRCDIR, filename);
    if (DEBUG) {
        sprintf(temp_err_string, "Trying to fetch binary data from file <%s>.", filename_with_path);
        ephem_log(temp_err_string);
    }

    // Open binary data file
    *file_pointer = fopen(filename_with_path, "rb");
    if (*file_pointer == NULL) return 1; // FAIL

    // Read the number of objects with orbital elements in this file
    dcf_fread((void *) item_count, sizeof(int), 1, *file_pointer, filename_with_path, __FILE__, __LINE__);
    if (DEBUG) {
        sprintf(temp_err_string, "Object count = %d", *item_count);
        ephem_log(temp_err_string);
    }

    // Read the number of secure orbits described in this file
    dcf_fread((void *) item_secure_count, sizeof(int), 1, *file_pointer, filename_with_path, __FILE__, __LINE__);
    if (DEBUG) {
        sprintf(temp_err_string, "Objects with secure orbits = %d", *item_secure_count);
        ephem_log(temp_err_string);
    }

    // Check that numbers are sensible
    if ((*item_count < 1) || (*item_count > 1e6)) {
        if (DEBUG) { ephem_log("Rejecting this as implausible"); }
        fclose(*file_pointer);
        *file_pointer = NULL;
        return 1;
    }

    // We have now reached the orbital elements. Store their offset from the start of the file.
    *elements_offset = ftell(*file_pointer);

    // Allocate memory to store records as we load them
    *data_buffer = (orbitalElements *) lt_malloc((*item_count) * sizeof(orbitalElements));
    *data_buffer_items_loaded = (unsigned char *) lt_malloc((*item_count) * sizeof(unsigned char));

    // Zero array telling us which records we have read
    memset(*data_buffer_items_loaded, 0, *item_count);

    if (DEBUG) {
        sprintf(temp_err_string, "Data file opened successfully.");
        ephem_log(temp_err_string);
    }

    // Success
    return 0;
}

//! OrbitalElements_DumpBinaryData - dump orbital elements to a binary dump such as <data/dcfbinary.ast>,
//! to save parsing original text file every time we are run.
//!
//! \param [in] filename - The filename of the binary dump we are to produce
//! \param [in] data - The table of orbitalElements structures to write
//! \param [out] elements_offset  - Return the offset of the start of the table of <orbitalElements> structures from the
//! beginning of the file.
//! \param [in] item_count - The number of orbital elements structures to write
//! \param [in] item_secure_count - The number of objects in this table which have secure orbits

void OrbitalElements_DumpBinaryData(const char *filename, const orbitalElements *data, int *elements_offset,
                                    const int item_count, const int item_secure_count) {
    FILE *output;
    char filename_with_path[FNAME_LENGTH];

    // Work out the full path of the binary data file we are to write
    sprintf(filename_with_path, "%s/../data/%s", SRCDIR, filename);
    if (DEBUG) {
        sprintf(temp_err_string, "Dumping binary data to file <%s>.", filename_with_path);
        ephem_log(temp_err_string);
    }

    // Open binary data file
    output = fopen(filename_with_path, "wb");
    if (output == NULL) return; // FAIL

    // Write the number of objects with orbital elements in this file
    fwrite((void *) &item_count, sizeof(int), 1, output);
    fwrite((void *) &item_secure_count, sizeof(int), 1, output);

    // We have now reached the orbital elements. Store their offset from the start of the file.
    *elements_offset = ftell(output);

    // Write the orbital elements themselves
    fwrite((void *) data, sizeof(orbitalElements), item_count, output);

    // Close output file
    fclose(output);

    // Finished
    if (DEBUG) {
        sprintf(temp_err_string, "Data successfully dumped.");
        ephem_log(temp_err_string);
    }
}

//! orbitalElements_planets_readAsciiData - Read the asteroid orbital elements contained in the file <data/planets.dat>

void orbitalElements_planets_readAsciiData() {
    int i, j;
    char fname[FNAME_LENGTH];
    FILE *input = NULL;

    // Try and read data from binary dump. Only proceed with parsing the text files if binary dump doesn't exist.
    int status = OrbitalElements_ReadBinaryData("dcfbinary.plt", &planet_database_file, &planet_database_offset,
                                                &planet_database, &planet_database_items_loaded,
                                                &planet_count, &planet_secure_count);

    // If successful, return
    if (status == 0) return;

    // Allocate memory to store asteroid orbital elements, and reset counters of how many objects we have
    planet_count = 0;
    planet_secure_count = 0;
    planet_database = (orbitalElements *) lt_malloc(MAX_PLANETS * sizeof(orbitalElements));
    if (planet_database == NULL) {
        ephem_fatal(__FILE__, __LINE__, "Malloc fail.");
        exit(1);
    }

    // Pre-fill columns with NANs, which is the best value for data we don't populate later
    for (i = 0; i < MAX_PLANETS; i++) {
        memset(&planet_database[i], 0, sizeof(orbitalElements));
        planet_database[i].absoluteMag = GSL_NAN;
        planet_database[i].meanAnomaly = GSL_NAN;
        planet_database[i].argumentPerihelion = GSL_NAN;
        planet_database[i].longAscNode = GSL_NAN;
        planet_database[i].inclination = GSL_NAN;
        planet_database[i].eccentricity = GSL_NAN;
        planet_database[i].semiMajorAxis = GSL_NAN;
        planet_database[i].epochPerihelion = GSL_NAN;
        planet_database[i].epochOsculation = GSL_NAN;
        planet_database[i].slopeParam_n = 2;
        planet_database[i].slopeParam_G = -999;
        planet_database[i].number = -1;
        planet_database[i].secureOrbit = 0;
        planet_database[i].argumentPerihelion_dot = 0;
        planet_database[i].longAscNode_dot = 0;
        planet_database[i].inclination_dot = 0;
        planet_database[i].eccentricity_dot = 0;
        planet_database[i].semiMajorAxis_dot = 0;
        strcpy(planet_database[i].name, "Undefined");
        strcpy(planet_database[i].name2, "Undefined");
    }

    if (DEBUG) {
        sprintf(temp_err_string, "Beginning to read ASCII planet list.");
        ephem_log(temp_err_string);
    }
    sprintf(fname, "%s/../data/planets.dat", SRCDIR);
    if (DEBUG) {
        sprintf(temp_err_string, "Opening file <%s>", fname);
        ephem_log(temp_err_string);
    }
    input = fopen(fname, "rt");
    if (input == NULL) {
        ephem_fatal(__FILE__, __LINE__, "Could not open planet data file.");
        exit(1);
    }

    // Read through planet.dat, line by line
    while ((!feof(input)) && (!ferror(input))) {
        char line[FNAME_LENGTH];
        int body_id;

        // Read a line from the input file
        file_readline(input, line);

        // Ignore blank lines and comment lines
        if (line[0] == '\0') continue;
        if (line[0] == '#') continue;
        if (strlen(line) < 100) continue;

        // Read body id
        body_id = (int) get_float(line, NULL);
        if (planet_count <= body_id) planet_count = body_id + 1;
        planet_database[body_id].number = body_id;
        planet_secure_count++;

        // Read planet name
        for (i = 141, j = 0; (line[i] > ' '); i++, j++) planet_database[body_id].name[j] = line[i];
        planet_database[body_id].name[j] = '\0';

        // Fill out dummy information
        planet_database[body_id].absoluteMag = 999;
        planet_database[body_id].slopeParam_G = 2;
        planet_database[body_id].secureOrbit = 1;

        // Now start reading orbital elements of object
        for (i = 18; line[i] == ' '; i++);
        // AU
        planet_database[body_id].semiMajorAxis = get_float(line + i, NULL);
        for (i = 30; line[i] == ' '; i++);
        // convert <AU per century> to <AU per day>
        planet_database[body_id].semiMajorAxis_dot = get_float(line + i, NULL) / 36525.;
        for (i = 42; line[i] == ' '; i++);
        planet_database[body_id].eccentricity = get_float(line + i, NULL);
        for (i = 53; line[i] == ' '; i++);
        // convert <per century> into <per day>
        planet_database[body_id].eccentricity_dot = get_float(line + i, NULL) / 36525.;
        for (i = 65; line[i] == ' '; i++);
        // radians; J2000.0
        planet_database[body_id].longAscNode = get_float(line + i, NULL) * M_PI / 180;
        for (i = 75; line[i] == ' '; i++);
        // Convert <arcsec/century> into <radians/day>
        planet_database[body_id].longAscNode_dot = get_float(line + i, NULL) / 36525. / 3600 * M_PI / 180;
        for (i = 85; line[i] == ' '; i++);
        // radians; J2000.0
        planet_database[body_id].inclination = get_float(line + i, NULL) * M_PI / 180;
        for (i = 94; line[i] == ' '; i++);
        // Convert <arcsec/century> into <radians/day>
        planet_database[body_id].inclination_dot = get_float(line + i, NULL) / 36525. / 3600 * M_PI / 180;
        for (i = 101; line[i] == ' '; i++);
        // radians; J2000.0
        const double longitude_perihelion = get_float(line + i, NULL) * M_PI / 180;
        for (i = 111; line[i] == ' '; i++);
        // Convert <arcsec/century> into <radians/day>
        const double longitude_perihelion_dot = get_float(line + i, NULL) / 36525. / 3600 * M_PI / 180;
        for (i = 120; line[i] == ' '; i++);
        // radians; J2000.0
        const double mean_longitude = get_float(line + i, NULL) * M_PI / 180;
        for (i = 130; line[i] == ' '; i++);
        // julian date
        planet_database[body_id].epochOsculation = jd_from_unix(get_float(line + i, NULL));

        // radians; J2000.0
        planet_database[body_id].meanAnomaly = mean_longitude - longitude_perihelion;

        // radians; J2000.0
        planet_database[body_id].argumentPerihelion = longitude_perihelion - planet_database[body_id].longAscNode;

        // radians per day
        planet_database[body_id].argumentPerihelion_dot = (longitude_perihelion_dot -
                                                           planet_database[body_id].longAscNode_dot);
    }
    fclose(input);

    if (DEBUG) {
        sprintf(temp_err_string, "Planet count                 = %7d", planet_count);
        ephem_log(temp_err_string);
    }
    if (DEBUG) {
        sprintf(temp_err_string, "Planets with secure orbits   = %7d", planet_secure_count);
        ephem_log(temp_err_string);
    }

    // Now that we've parsed the text-based version of this data, dump a binary version to make loading faster next time
    OrbitalElements_DumpBinaryData("dcfbinary.plt", planet_database, &planet_database_offset,
                                   planet_count, planet_secure_count);

    // Make table indicating that we have loaded all the orbital elements in this table
    planet_database_items_loaded = (unsigned char *) lt_malloc(planet_count * sizeof(unsigned char));
    memset(planet_database_items_loaded, 1, planet_count);

    // Open a file pointer to the file
    char filename_with_path[FNAME_LENGTH];
    snprintf(filename_with_path, FNAME_LENGTH, "%s/../data/%s", SRCDIR, "dcfbinary.plt");
    planet_database_file = fopen(filename_with_path, "rb");
    snprintf(planet_database_filename, FNAME_LENGTH, "%s", filename_with_path);
}

//! orbitalElements_asteroids_readAsciiData - Read the asteroid orbital elements contained in the original astorb.dat
//! file downloaded from Ted Bowell's website

void orbitalElements_asteroids_readAsciiData() {
    int i;
    char fname[FNAME_LENGTH];
    FILE *input = NULL;

    // Try and read data from binary dump. Only proceed with parsing the text files if binary dump doesn't exist.
    int status = OrbitalElements_ReadBinaryData("dcfbinary.ast", &asteroid_database_file, &asteroid_database_offset,
                                                &asteroid_database, &asteroid_database_items_loaded,
                                                &asteroid_count, &asteroid_secure_count);

    // If successful, return
    if (status == 0) return;

    // Allocate memory to store asteroid orbital elements, and reset counters of how many objects we have
    asteroid_count = 0;
    asteroid_secure_count = 0;
    asteroid_database = (orbitalElements *) lt_malloc(MAX_ASTEROIDS * sizeof(orbitalElements));
    if (asteroid_database == NULL) {
        ephem_fatal(__FILE__, __LINE__, "Malloc fail.");
        exit(1);
    }

    // Pre-fill columns with NANs, which is the best value for data we don't populate later
    for (i = 0; i < MAX_ASTEROIDS; i++) {
        memset(&asteroid_database[i], 0, sizeof(orbitalElements));
        asteroid_database[i].absoluteMag = GSL_NAN;
        asteroid_database[i].meanAnomaly = GSL_NAN;
        asteroid_database[i].argumentPerihelion = GSL_NAN;
        asteroid_database[i].longAscNode = GSL_NAN;
        asteroid_database[i].inclination = GSL_NAN;
        asteroid_database[i].eccentricity = GSL_NAN;
        asteroid_database[i].semiMajorAxis = GSL_NAN;
        asteroid_database[i].epochPerihelion = GSL_NAN;
        asteroid_database[i].epochOsculation = GSL_NAN;
        asteroid_database[i].slopeParam_n = 2;
        asteroid_database[i].slopeParam_G = -999;
        asteroid_database[i].number = -1;
        asteroid_database[i].secureOrbit = 0;
        asteroid_database[i].argumentPerihelion_dot = 0;
        asteroid_database[i].longAscNode_dot = 0;
        asteroid_database[i].inclination_dot = 0;
        asteroid_database[i].eccentricity_dot = 0;
        asteroid_database[i].semiMajorAxis_dot = 0;
        strcpy(asteroid_database[i].name, "Undefined");
        strcpy(asteroid_database[i].name2, "Undefined");
    }

    if (DEBUG) {
        sprintf(temp_err_string, "Beginning to read ASCII asteroid list.");
        ephem_log(temp_err_string);
    }
    sprintf(fname, "%s/../data/astorb.dat", SRCDIR);
    if (DEBUG) {
        sprintf(temp_err_string, "Opening file <%s>", fname);
        ephem_log(temp_err_string);
    }
    input = fopen(fname, "rt");
    if (input == NULL) {
        ephem_fatal(__FILE__, __LINE__, "Could not open asteroid data file.");
        exit(1);
    }

    // Read through astorb.dat, line by line
    while ((!feof(input)) && (!ferror(input))) {
        char line[FNAME_LENGTH];

        // Read a line from the input file
        file_readline(input, line);

        // Ignore blank lines and comment lines
        if (line[0] == '\0') continue;
        if (line[0] == '#') continue;
        if (strlen(line) < 250) continue;

        // Read asteroid number
        for (i = 0; (line[i] > '\0') && (line[i] <= ' '); i++);

        // Unnumbered asteroid; don't bother adding to catalogue
        if (i >= 6) continue;

        const int n = (int) get_float(line + i, NULL);

        // asteroid_count should be the highest number asteroid we have encountered
        if (asteroid_count <= n) asteroid_count = n + 1;
        asteroid_database[n].number = n;

        // Read asteroid name
        for (i = 25; (i > 7) && (line[i] > '\0') && (line[i] <= ' '); i--);
        strncpy(asteroid_database[n].name, line + 7, i - 6);
        asteroid_database[n].name[i - 6] = '\0';

        // Read absolute magnitude
        for (i = 42; (line[i] > '\0') && (line[i] <= ' '); i++);
        asteroid_database[n].absoluteMag = get_float(line + i, NULL);

        // Read slope parameter
        for (i = 48; (line[i] > '\0') && (line[i] <= ' '); i++);
        asteroid_database[n].slopeParam_G = get_float(line + i, NULL);

        // Read number of days spanned by data used to derive orbit
        int day_obs_span;
        {
            int j;
            char buffer[8];
            snprintf(buffer, 7, "%s", line + 94);
            for (j = 0; (buffer[j] > '\0') && (buffer[j] <= ' '); j++);
            day_obs_span = (int) get_float(buffer + j, NULL);
        }

        // Read number of observations used to derive orbit
        for (i = 100; (line[i] > '\0') && (line[i] <= ' '); i++);
        const int obs_count = (int) get_float(line + i, NULL);

        // Orbit deemed secure if more than 10 yrs data
        asteroid_database[n].secureOrbit = (day_obs_span > 3650) && (obs_count > 500);

        // Count how many objects we've seen with secure orbits
        if (asteroid_database[n].secureOrbit) asteroid_secure_count++;

        // Now start reading orbital elements of object
        {
            for (i = 106; (line[i] > '\0') && (line[i] <= ' '); i++);
            const double tmp = get_float(line + i, NULL);
            // julian date
            asteroid_database[n].epochOsculation = julian_day((int) floor(tmp / 10000), ((int) floor(tmp / 100)) % 100,
                                                              ((int) floor(tmp)) % 100, 0, 0, 0, &i, temp_err_string);
        }

        // Read mean anomaly -- radians; J2000.0
        for (i = 115; (line[i] > '\0') && (line[i] <= ' '); i++);
        asteroid_database[n].meanAnomaly = get_float(line + i, NULL) * M_PI / 180;

        // Read argument of perihelion -- radians; J2000.0
        for (i = 126; (line[i] > '\0') && (line[i] <= ' '); i++);
        asteroid_database[n].argumentPerihelion = get_float(line + i, NULL) * M_PI / 180;

        // Read longitude of ascending node -- radians; J2000.0
        for (i = 137; (line[i] > '\0') && (line[i] <= ' '); i++);
        asteroid_database[n].longAscNode = get_float(line + i, NULL) * M_PI / 180;

        // Read inclination of orbit -- radians; J2000.0
        for (i = 147; (line[i] > '\0') && (line[i] <= ' '); i++);
        asteroid_database[n].inclination = get_float(line + i, NULL) * M_PI / 180;

        // Read eccentricity of orbit -- dimensionless
        for (i = 157; (line[i] > '\0') && (line[i] <= ' '); i++);
        asteroid_database[n].eccentricity = get_float(line + i, NULL);

        // Read semi-major axis of orbit -- AU
        for (i = 168; (line[i] > '\0') && (line[i] <= ' '); i++);
        asteroid_database[n].semiMajorAxis = get_float(line + i, NULL);
    }
    fclose(input);

    if (DEBUG) {
        sprintf(temp_err_string, "Asteroid count               = %7d", asteroid_count);
        ephem_log(temp_err_string);
    }
    if (DEBUG) {
        sprintf(temp_err_string, "Asteroids with secure orbits = %7d", asteroid_secure_count);
        ephem_log(temp_err_string);
    }

    // Now that we've parsed the text-based version of this data, dump a binary version to make loading faster next time
    OrbitalElements_DumpBinaryData("dcfbinary.ast", asteroid_database, &asteroid_database_offset,
                                   asteroid_count, asteroid_secure_count);

    // Make table indicating that we have loaded all the orbital elements in this table
    asteroid_database_items_loaded = (unsigned char *) lt_malloc(asteroid_count * sizeof(unsigned char));
    memset(asteroid_database_items_loaded, 1, asteroid_count);

    // Open a file pointer to the file
    char filename_with_path[FNAME_LENGTH];
    snprintf(filename_with_path, FNAME_LENGTH, "%s/../data/%s", SRCDIR, "dcfbinary.ast");
    asteroid_database_file = fopen(filename_with_path, "rb");
    snprintf(asteroid_database_filename, FNAME_LENGTH, "%s", filename_with_path);
}


//! orbitalElements_comets_readAsciiData - Read the comet orbital elements contained in the ASCII file downloaded
//! from the Minor Planet Center's website

void orbitalElements_comets_readAsciiData() {
    int i;
    char fname[FNAME_LENGTH];
    FILE *input = NULL;

    // Try and read data from binary dump. Only proceed with parsing the text files if binary dump doesn't exist.
    int status = OrbitalElements_ReadBinaryData("dcfbinary.cmt", &comet_database_file, &comet_database_offset,
                                                &comet_database, &comet_database_items_loaded,
                                                &comet_count, &comet_secure_count);

    // If successful, return
    if (status == 0) return;

    // Allocate memory to store asteroid orbital elements, and reset counters of how many objects we have
    comet_count = 0;
    comet_secure_count = 0;
    comet_database = (orbitalElements *) lt_malloc(MAX_COMETS * sizeof(orbitalElements));
    if (comet_database == NULL) {
        ephem_fatal(__FILE__, __LINE__, "Malloc fail.");
        exit(1);
    }

    // Pre-fill columns with NANs, which is the best value for data we don't populate later
    for (i = 0; i < MAX_COMETS; i++) {
        memset(&comet_database[i], 0, sizeof(orbitalElements));
        comet_database[i].absoluteMag = GSL_NAN;
        comet_database[i].meanAnomaly = GSL_NAN;
        comet_database[i].argumentPerihelion = GSL_NAN;
        comet_database[i].longAscNode = GSL_NAN;
        comet_database[i].inclination = GSL_NAN;
        comet_database[i].eccentricity = GSL_NAN;
        comet_database[i].semiMajorAxis = GSL_NAN;
        comet_database[i].epochPerihelion = GSL_NAN;
        comet_database[i].epochOsculation = GSL_NAN;
        comet_database[i].slopeParam_n = 2;
        comet_database[i].slopeParam_G = -999;
        comet_database[i].number = -1;
        comet_database[i].secureOrbit = 0;
        comet_database[i].argumentPerihelion_dot = 0;
        comet_database[i].longAscNode_dot = 0;
        comet_database[i].inclination_dot = 0;
        comet_database[i].eccentricity_dot = 0;
        comet_database[i].semiMajorAxis_dot = 0;
        strcpy(comet_database[i].name, "Undefined");
        strcpy(comet_database[i].name2, "Undefined");
    }

    // Now start reading the orbital elements of comets from Soft00Cmt.txt

    if (DEBUG) {
        sprintf(temp_err_string, "Beginning to read ASCII comet list.");
        ephem_log(temp_err_string);
    }
    sprintf(fname, "%s/../data/Soft00Cmt.txt", SRCDIR);
    if (DEBUG) {
        sprintf(temp_err_string, "Opening file <%s>", fname);
        ephem_log(temp_err_string);
    }
    input = fopen(fname, "rt");
    if (input == NULL) {
        ephem_fatal(__FILE__, __LINE__, "Could not open comet data file.");
        exit(1);
    }

    // Iterate over Soft00Cmt.txt, line by line
    while ((!feof(input)) && (!ferror(input))) {
        char line[FNAME_LENGTH];
        int j, k;
        double tmp, perihelion_dist, eccentricity, perihelion_date, epoch, a;

        // Read a line from the input file
        file_readline(input, line);

        // Ignore blank lines and comment lines
        if (line[0] == '\0') continue;
        if (line[0] == '#') continue;
        if (strlen(line) < 100) continue;

        // Read comet name
        for (j = 102, k = 0; (line[j] != '(') && (line[j] != '\0') && (k < 23); j++, k++) {
            comet_database[comet_count].name[k] = line[j];
        }
        while ((k > 0) && (comet_database[comet_count].name[--k] == ' '));
        comet_database[comet_count].name[k + 1] = '\0';

        // Read comet's MPC designation
        for (j = 0, k = 0; (line[j] > '\0') && (line[j] <= ' '); j++);
        while ((line[j] > ' ') && (k < 23)) comet_database[comet_count].name2[k++] = line[j++];
        comet_database[comet_count].name2[k] = '\0';

        // Read perihelion distance
        perihelion_dist = get_float(line + 31, NULL);

        // Read perihelion date
        for (j = 14; (line[j] > '\0') && (line[j] <= ' '); j++);
        const int perihelion_year = (int) get_float(line + j, NULL);
        for (j = 19; (line[j] > '\0') && (line[j] <= ' '); j++);
        const int perihelion_month = (int) get_float(line + j, NULL);
        for (j = 22; (line[j] > '\0') && (line[j] <= ' '); j++);
        const double perihelion_day = get_float(line + j, NULL);

        // julian date
        perihelion_date = julian_day(perihelion_year, perihelion_month, (int) floor(perihelion_day),
                                     ((int) floor(perihelion_day * 24)) % 24,
                                     ((int) floor(perihelion_day * 24 * 60)) % 60,
                                     ((int) floor(perihelion_day * 24 * 3600)) % 60,
                                     &j, temp_err_string);

        // Read eccentricity of orbit
        for (j = 41; (line[j] > '\0') && (line[j] <= ' '); j++);
        comet_database[comet_count].eccentricity = eccentricity = get_float(line + j, NULL);

        // Read argument of perihelion, radians, J2000.0
        for (j = 51; (line[j] > '\0') && (line[j] <= ' '); j++);
        comet_database[comet_count].argumentPerihelion = get_float(line + j, NULL) * M_PI / 180;

        // Read longitude of ascending node, radians, J2000.0
        for (j = 61; (line[j] > '\0') && (line[j] <= ' '); j++);
        comet_database[comet_count].longAscNode = get_float(line + j, NULL) * M_PI / 180;

        // Read orbital inclination, radians, J2000.0
        for (j = 71; (line[j] > '\0') && (line[j] <= ' '); j++);
        comet_database[comet_count].inclination = get_float(line + j, NULL) * M_PI / 180;

        // Read epoch of osculation, julian date
        for (j = 81; (line[j] > '\0') && (line[j] <= ' '); j++);
        tmp = get_float(line + j, NULL);
        comet_database[comet_count].epochOsculation = epoch = julian_day((int) floor(tmp / 10000),
                                                                         ((int) floor(tmp / 100)) % 100,
                                                                         ((int) floor(tmp)) % 100, 0, 0, 0,
                                                                         &j,
                                                                         temp_err_string);

        // Read absolute magnitude
        for (j = 90; (line[j] > '\0') && (line[j] <= ' '); j++);
        if (!valid_float(line + j, NULL)) comet_database[comet_count].absoluteMag = GSL_NAN;
        else comet_database[comet_count].absoluteMag = get_float(line + j, NULL);

        // Read slope parameter
        for (j = 96; (line[j] > '\0') && (line[j] <= ' '); j++);
        if (!valid_float(line + j, NULL)) comet_database[comet_count].slopeParam_n = 2;
        else comet_database[comet_count].slopeParam_n = get_float(line + j, NULL);

        // Calculate derived quantities
        comet_database[comet_count].secureOrbit = 1;
        // AU
        comet_database[comet_count].semiMajorAxis = a = perihelion_dist / (1 - eccentricity);
        // radians; J2000.0
        comet_database[comet_count].meanAnomaly = fmod(
                sqrt(GSL_CONST_MKSA_GRAVITATIONAL_CONSTANT * GSL_CONST_MKSA_SOLAR_MASS /
                     gsl_pow_3(fabs(a) * GSL_CONST_MKSA_ASTRONOMICAL_UNIT)) * (epoch - perihelion_date) * 24 * 3600 +
                100 * M_PI, 2 * M_PI);
        // julian date
        comet_database[comet_count].epochPerihelion = perihelion_date;

        // Increment the comet counter
        comet_count++;
        comet_secure_count++;
    }
    fclose(input);

    if (DEBUG) {
        sprintf(temp_err_string, "Comet count                  = %7d", comet_count);
        ephem_log(temp_err_string);
    }
    if (DEBUG) {
        sprintf(temp_err_string, "Comets with secure orbits    = %7d", comet_secure_count);
        ephem_log(temp_err_string);
    }

    // Now that we've parsed the text-based version of this data, dump a binary version to make loading faster next time
    OrbitalElements_DumpBinaryData("dcfbinary.cmt", comet_database, &comet_database_offset,
                                   comet_count, comet_secure_count);

    // Make table indicating that we have loaded all the orbital elements in this table
    comet_database_items_loaded = (unsigned char *) lt_malloc(comet_count * sizeof(unsigned char));
    memset(comet_database_items_loaded, 1, comet_count);

    // Open a file pointer to the file
    char filename_with_path[FNAME_LENGTH];
    snprintf(filename_with_path, FNAME_LENGTH, "%s/../data/%s", SRCDIR, "dcfbinary.cmt");
    comet_database_file = fopen(filename_with_path, "rb");
    snprintf(comet_database_filename, FNAME_LENGTH, "%s", filename_with_path);
}

//! orbitalElements_planets_init - Make sure that planet orbital elements are initialised, in thread-safe fashion

void orbitalElements_planets_init() {
#pragma omp critical (planets_init)
    {
        if (planet_database_file == NULL) orbitalElements_planets_readAsciiData();
    }
}

//! orbitalElements_planets_fetch - Fetch the orbitalElements record for bodyId <index>. If needed, load them from disk.
//! \param index - The bodyId of the object whose orbital elements are to be loaded
//! \return - An orbitalElements structure for bodyId

orbitalElements *orbitalElements_planets_fetch(int index) {
    // Check that request is within allowed range
    if ((index < 0) || (index > planet_count)) return NULL;

    // If we have already loaded these orbital elements, we can return a pointer immediately
    if (planet_database_items_loaded[index]) return &planet_database[index];

#pragma omp critical (planets_fetch)
    {
        // If not, then read them from disk now
        long data_position_needed = planet_database_offset + index * sizeof(orbitalElements);
        fseek(planet_database_file, data_position_needed, SEEK_SET);
        dcf_fread((void *) &planet_database[index], sizeof(orbitalElements), 1, planet_database_file,
                  planet_database_filename, __FILE__, __LINE__);
        planet_database_items_loaded[index] = 1;
    }

    return &planet_database[index];
}

//! orbitalElements_asteroids_init - Make sure that asteroid orbital elements are initialised, in thread-safe fashion

void orbitalElements_asteroids_init() {
#pragma omp critical (asteroids_init)
    {
        if (asteroid_database_file == NULL) orbitalElements_asteroids_readAsciiData();
    }
}

//! orbitalElements_asteroids_fetch - Fetch the orbitalElements record for bodyId (10000000 + index). If needed, load
//! them from disk.
//! \param index - The index of the object whose orbital elements are to be loaded (bodyId = 10000000 + index)
//! \return - An orbitalElements structure for bodyId

orbitalElements *orbitalElements_asteroids_fetch(int index) {
    // Check that request is within allowed range
    if ((index < 0) || (index > asteroid_count)) return NULL;

    // If we have already loaded these orbital elements, we can return a pointer immediately
    if (asteroid_database_items_loaded[index]) return &asteroid_database[index];

#pragma omp critical (asteroids_fetch)
    {
        // If not, then read them from disk now
        long data_position_needed = asteroid_database_offset + index * sizeof(orbitalElements);
        fseek(asteroid_database_file, data_position_needed, SEEK_SET);
        dcf_fread((void *) &asteroid_database[index], sizeof(orbitalElements), 1, asteroid_database_file,
                  asteroid_database_filename, __FILE__, __LINE__);
        asteroid_database_items_loaded[index] = 1;
    }

    return &asteroid_database[index];
}

//! orbitalElements_comets_init - Make sure that comet orbital elements are initialised, in thread-safe fashion

void orbitalElements_comets_init() {
#pragma omp critical (comets_init)
    {
        if (comet_database_file == NULL) orbitalElements_comets_readAsciiData();
    }
}

//! orbitalElements_comets_fetch - Fetch the orbitalElements record for bodyId (20000000 + index). If needed, load
//! them from disk.
//! \param index - The index of the object whose orbital elements are to be loaded (bodyId = 20000000 + index)
//! \return - An orbitalElements structure for bodyId

orbitalElements *orbitalElements_comets_fetch(int index) {
    // Check that request is within allowed range
    if ((index < 0) || (index > comet_count)) return NULL;

    // If we have already loaded these orbital elements, we can return a pointer immediately
    if (comet_database_items_loaded[index]) return &comet_database[index];

#pragma omp critical (comets_fetch)
    {
        // If not, then read them from disk now
        long data_position_needed = comet_database_offset + index * sizeof(orbitalElements);
        fseek(comet_database_file, data_position_needed, SEEK_SET);
        dcf_fread((void *) &comet_database[index], sizeof(orbitalElements), 1, comet_database_file,
                  comet_database_filename, __FILE__, __LINE__);
        comet_database_items_loaded[index] = 1;
    }

    return &comet_database[index];
}

//! orbitalElements_computeXYZ - Main orbital elements computer. Return 3D position in ICRF, in AU, relative to the
//! Sun (not the solar system barycentre!!). z-axis points towards the J2000.0 north celestial pole.
//! \param [in] body_id - The id number of the object whose position is being queried
//! \param [in] jd - The Julian day number at which the object's position is wanted; TT
//! \param [out] x - The x position of the object relative to the Sun (in AU; ICRF; points to RA=0)
//! \param [out] y - The y position of the object relative to the Sun (in AU; ICRF; points to RA=6h)
//! \param [out] z - The z position of the object relative to the Sun (in AU; ICRF; points to NCP)

void orbitalElements_computeXYZ(int body_id, double jd, double *x, double *y, double *z) {
    orbitalElements *orbital_elements;

    double v, r;

    // const double epsilon = (23.4393 - 3.563E-7 * (jd - 2451544.5)) * M_PI / 180;

    // Case 1: Object is a planet
    if (body_id < 10000000) {
        // Planets occupy body numbers 1-19
        const int index = body_id;

        orbitalElements_planets_init();

        // Return NaN if object is not in database
        if ((planet_database_file == NULL) || (index >= planet_count)) {
            *x = *y = *z = GSL_NAN;
            return;
        }

        // Fetch data from the binary database file
        orbital_elements = orbitalElements_planets_fetch(index);
    }

        // Case 2: Object is an asteroid
    else if (body_id < 20000000) {
        // Asteroids occupy body numbers 1e7 - 2e7
        const int index = body_id - 10000000;

        orbitalElements_asteroids_init();

        // Return NaN if object is not in database
        if ((asteroid_database_file == NULL) || (index >= asteroid_count)) {
            *x = *y = *z = GSL_NAN;
            return;
        }

        // Fetch data from the binary database file
        orbital_elements = orbitalElements_asteroids_fetch(index);
    }

        // Case 3: Object is a comet
    else {
        // Comets occupy body numbers 2e7 - 3e7
        const int index = body_id - 20000000;

        orbitalElements_comets_init();

        // Return NaN if object is not in database
        if ((comet_database_file == NULL) || (index >= comet_count)) {
            *x = *y = *z = GSL_NAN;
            return;
        }

        // Fetch data from the binary database file
        orbital_elements = orbitalElements_comets_fetch(index);
    }

    // Extract orbital elements from structure
    const double offset_from_epoch = jd - orbital_elements->epochOsculation;
    const double a = orbital_elements->semiMajorAxis + orbital_elements->semiMajorAxis_dot * offset_from_epoch;
    const double e = orbital_elements->eccentricity + orbital_elements->eccentricity_dot * offset_from_epoch;
    const double N = orbital_elements->longAscNode + orbital_elements->longAscNode_dot * offset_from_epoch;
    const double inc = orbital_elements->inclination + orbital_elements->inclination_dot * offset_from_epoch;
    const double w = orbital_elements->argumentPerihelion + (orbital_elements->argumentPerihelion_dot *
                                                             offset_from_epoch);

    // Mean Anomaly for desired epoch (convert rate of change per second into rate of change per day)
    const double mean_motion = sqrt(GSL_CONST_MKSA_GRAVITATIONAL_CONSTANT * GSL_CONST_MKSA_SOLAR_MASS /
                                    gsl_pow_3(fabs(a) * GSL_CONST_MKSA_ASTRONOMICAL_UNIT));

    const double M = orbital_elements->meanAnomaly +
                     (jd - orbital_elements->epochOsculation) * mean_motion * 24 * 3600;


    // When debugging, show intermediate calculation
    if (DEBUG) {
        sprintf(temp_err_string, "Object ID = %d", body_id);
        ephem_log(temp_err_string);
        sprintf(temp_err_string, "JD = %.5f", jd);
        ephem_log(temp_err_string);
    }

    if (e > 1.02) {
        // Hyperbolic orbit
        // See <https://stjarnhimlen.se/comp/ppcomp.html> section 20

        const double M_hyperbolic = (jd - orbital_elements->epochPerihelion) * mean_motion * 24 * 3600;

        double F0, F1, ratio = 0.5;
        F0 = M_hyperbolic;
        for (int j = 0; ((j < 50) && (ratio > 1e-8)); j++) {
            F1 = (M_hyperbolic + e * (F0 * cosh(F0) - sinh(F0))) / (e * cosh(F0) - 1); // Newton's method
            ratio = fabs(F1 / F0);
            if (ratio < 1) ratio = 1 / ratio;
            ratio -= 1;
            F0 = F1;
        }

        v = 2 * atan(sqrt((e + 1) / (e - 1))) * tanh(F0 / 2);
        r = a * (1 - e * e) / (1 + e * cos(v));
    } else if (e < 0.98) {
        int j;
        double E0, E1, delta_E = 1;
        E0 = M + e * sin(M);

        // Iteratively solve inverse Kepler's equation for eccentric anomaly
        for (j = 0; ((j < 50) && (fabs(delta_E) > 1e-12)); j++) {
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
            sprintf(temp_err_string, "E0 = %.10f deg", E0 * 180 / M_PI);
            ephem_log(temp_err_string);
            sprintf(temp_err_string, "delta_E = %.10e deg", delta_E * 180 / M_PI);
            ephem_log(temp_err_string);
            sprintf(temp_err_string, "xv = %.10f km", xv * GSL_CONST_MKSA_ASTRONOMICAL_UNIT / 1e3);
            ephem_log(temp_err_string);
            sprintf(temp_err_string, "yv = %.10f km", yv * GSL_CONST_MKSA_ASTRONOMICAL_UNIT / 1e3);
            ephem_log(temp_err_string);
            sprintf(temp_err_string, "j = %d iterations", j);
            ephem_log(temp_err_string);
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
        sprintf(temp_err_string, "a = %.10e km", a * GSL_CONST_MKSA_ASTRONOMICAL_UNIT / 1e3);
        ephem_log(temp_err_string);
        sprintf(temp_err_string, "e = %.10f", e);
        ephem_log(temp_err_string);
        sprintf(temp_err_string, "N = %.10f deg", N * 180 / M_PI);
        ephem_log(temp_err_string);
        sprintf(temp_err_string, "inc = %.10f deg", inc * 180 / M_PI);
        ephem_log(temp_err_string);
        sprintf(temp_err_string, "w = %.10f deg", w * 180 / M_PI);
        ephem_log(temp_err_string);
        sprintf(temp_err_string, "mean_motion = %.10e deg/sec", mean_motion * 180 / M_PI);
        ephem_log(temp_err_string);
        sprintf(temp_err_string, "M = %.10f deg (mean anomaly)", M * 180 / M_PI);
        ephem_log(temp_err_string);
        sprintf(temp_err_string, "v = %.10f deg", v * 180 / M_PI);
        ephem_log(temp_err_string);
        sprintf(temp_err_string, "r = %.10f km", r * GSL_CONST_MKSA_ASTRONOMICAL_UNIT / 1e3);
        ephem_log(temp_err_string);
    }
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
//! \param [out] angSize - Angular size of the object (arcseconds)
//! \param [out] phySize - Physical size of the object (metres)
//! \param [out] albedo - Albedo of the object
//! \param [out] sunDist - Distance of the object from the Sun (AU)
//! \param [out] earthDist - Distance of the object from the Earth (AU)
//! \param [out] sunAngDist - Angular distance of the object from the Sun, as seen from the Earth (radians)
//! \param [out] theta_ESO - Angular distance of the object from the Earth, as seen from the Sun (radians)
//! \param [out] eclipticLongitude - The ecliptic longitude of the object (J2000.0 radians)
//! \param [out] eclipticLatitude - The ecliptic latitude of the object (J2000.0 radians)
//! \param [out] eclipticDistance - The separation of the object from the Sun, in ecliptic longitude (radians)

void orbitalElements_computeEphemeris(int bodyId, double jd, double *x, double *y, double *z, double *ra,
                                      double *dec, double *mag, double *phase, double *angSize, double *phySize,
                                      double *albedo, double *sunDist, double *earthDist, double *sunAngDist,
                                      double *theta_eso, double *eclipticLongitude, double *eclipticLatitude,
                                      double *eclipticDistance) {
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
                                           sun_pos_z - earth_pos_z);  // AU
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

        // Otherwise we need to use the orbital elements for the particular object the user was looking for,
        // taking light travel time into account
    else {
        // Calculate position of requested object at specified time
        orbitalElements_computeXYZ(bodyId, jd, x, y, z);

        // Calculate light travel time
        const double distance = gsl_hypot3(*x - earth_pos_x, *y - earth_pos_y, *z - earth_pos_z);  // AU
        const double light_travel_time = distance * GSL_CONST_MKSA_ASTRONOMICAL_UNIT / GSL_CONST_MKSA_SPEED_OF_LIGHT;

        // Look up position of requested object at the time the light left the object
        orbitalElements_computeXYZ(bodyId, jd - light_travel_time / 86400, x, y, z);
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
                      albedo, sunDist, earthDist, sunAngDist, theta_eso, eclipticLongitude, eclipticLatitude,
                      eclipticDistance);
}
