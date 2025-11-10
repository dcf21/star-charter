// orbitalElements.h
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

#ifndef ORBITALELEMENTS_H
#define ORBITALELEMENTS_H 1

#define OBJECT_NAME_LENGTH 24

// Offsets applied to <bodyId> UIDs for objects in the asteroid and comet catalogues
#define MAX_ALLOWED_OBJECTS  9000000
#define ASTEROIDS_OFFSET    10000000
#define COMETS_OFFSET       20000000

typedef struct {
    int number, last_seen_file_index;
    char name[OBJECT_NAME_LENGTH], name2[OBJECT_NAME_LENGTH];
} objectNames;

typedef struct {
    int number; // bodyId for planets; bodyId-10000000 for asteroids; bodyId-20000000 for comets
    int secureOrbit; // boolean flag indicating whether orbit is deemed secure
    double epochOsculation; // Julian date
    double epochPerihelion; // Julian date
    double absoluteMag; // Absolute magnitude H
    double meanAnomaly; // mean anomaly at epoch of osculation; radians
    double argumentPerihelion; // argument of perihelion at epoch of osculation; radians; J2000.0
    double argumentPerihelion_dot; // rate of change; radians per day
    double longAscNode; // longitude of ascending node; radians; J2000.0
    double longAscNode_dot; // rate of change; radians per day
    double inclination; // radians; J2000.0
    double inclination_dot; // rate of change; radians per day
    double eccentricity;
    double eccentricity_dot; // rate of change; per day
    double semiMajorAxis; // AU
    double semiMajorAxis_dot; // rate of change; AU per day
    double slopeParam_n, slopeParam_G;
} orbitalElements;

typedef struct orbitalElementSet {
    // Details of the object type
    const char *object_type;
    int match_by_name;

    // List of names, used for searching for objects by name
    objectNames *object_names;

    // Details of the binary data file
    FILE *binary_data_file; // The file handle of the open binary file
    const char *binary_filename; // The filename of the binary file
    const char *ascii_filename_catalogue; // The filename of a file listing the text files containing orbital elements

    // Cache binary data in memory
    int cache_in_memory; // Boolean flag indicating whether to load binary file into memory; this is RAM intensive
    void *binary_file_cache; // Pointer to an in-memory copy of the binary file

    // Counters of the number of objects within the data file
    // These are all stored in order at the beginning of the binary file
    int ready; // Flag which is set to 1 when file is fully written (to stop other processes reading file prematurely)
    double file_creation_epoch; // Unix time when the file was created
    char software_version[24]; // The software version that created these elements
    char file_creation_hostname[64]; // Hostname of the machine on which the file was created
    int max_objects; // Size of orbital elements table
    int object_count; // Number of objects actually read into the orbital elements table
    int object_secure_count; // Number of objects with secure orbits
    int epoch_max_count; // Number of epochs allowed in the orbital elements table

    // Positions of tables within the binary file
    long offset_table_names; // Table of the names of the objects
    long offset_table_secure; // Table of ints, one per object, indicating whether it ever had a secure orbit
    long offset_table_epoch_counts; // Table of ints, one per object, indicating the number of available epochs
    long offset_table_orbital_elements; // Table of orbitalElements structures, of length epoch_max_count * max_objects

    // Function pointer, pointing to utility function to turn a line of ASCII into an orbitalElements structure
    int (*ascii_reader)(const char *, struct orbitalElementSet *, orbitalElements *, char *, char *);
} orbitalElementSet;

orbitalElements orbitalElements_nullOrbitalElements();

int orbitalElements_binary_getSecureFlag(orbitalElementSet *set, int object_index);

void orbitalElements_asteroids_init(int load_names);

int orbitalElements_fetch(int set_index, int object_index, double epoch_requested,
                          orbitalElements *output_1, double *weight_1,
                          orbitalElements *output_2, double *weight_2);

int orbitalElements_searchBodyIdByObjectName(const char *name_in);

void orbitalElements_computeXYZ(int body_id, double jd, double *x, double *y, double *z);

void orbitalElements_computeEphemeris(int bodyId, double jd, double *x, double *y, double *z, double *ra,
                                      double *dec, double *mag, double *phase, double *angSize, double *phySize,
                                      double *albedo, double *sunDist, double *earthDist, double *sunAngDist,
                                      double *theta_eso, double *eclipticLongitude, double *eclipticLatitude,
                                      double *eclipticDistance, double ra_dec_epoch,
                                      int do_topocentric_correction,
                                      double topocentric_latitude, double topocentric_longitude);

void orbitalElements_shutdown();

#endif
