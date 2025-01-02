#!/usr/bin/python3
# -*- coding: utf-8 -*-
# merge.py
#
# -------------------------------------------------
# Copyright 2015-2025 Dominic Ford
#
# This file is part of StarCharter.
#
# StarCharter is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# StarCharter is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with StarCharter.  If not, see <http://www.gnu.org/licenses/>.
# -------------------------------------------------

import copy
import json
import logging
import operator
import os
import re
import sys

from math import pi, cos, sin, sqrt, asin
from typing import Any, Dict, Final, List, Optional, Sequence

# Conversion of object types used in the Messier catalogue data file into the form we use in final file
object_type_conversion_messier: Final[Dict[str, str]] = {
    "GC": "Gb",  # globular clusters
    "PN": "Pl",  # planetary nebulae
    "BN": "Nb",  # bright nebulae
    "GX": "Gx",  # galaxies
    "*2": "D*",  # double stars
    "**": "As*",  # asterisms
    "*C": "As*"  # asterisms
}

# Conversion of object types used in the NGC2000 catalogue data file into the form we use in final file
object_type_conversion_ngc2000: Final[Dict[str, str]] = {
    "": "-",  # no type info
    "Ast": "As*",  # asterisms
    "C+N": "Nb",  # clusters with nebulosity
    "D*?": "D*",  # double stars
    "Kt": "Nb",  # knot
    "*": "star",  # stars
    "***": "As*",  # asterisms
    "*?": "star",  # stars
    "?": "Nb"  # whatever
}

# Conversion of object types used in the OpenNGC catalogue data file into the form we use in final file
object_type_conversion_open_ngc: Final[Dict[str, str]] = {
    "*": "star",
    "**": "D*",
    "*Ass": "As*",
    "OCl": "OC",
    "GCl": "Gb",
    "Cl+N": "Nb",
    "G": "Gx",
    "GPair": "Gx2",
    "GTrpl": "Gx3",
    "GGroup": "GxC",
    "PN": "Pl",
    "HII": "HII",
    "DrkN": "Dk",
    "EmN": "EmN",
    "Neb": "Nb",
    "RfN": "Ref",
    "SNR": "SNR",
    "Nova": "star",
    "NonEx": "none",
    "Dup": "dup",
    "Other": "-",
}


def angular_distance(ra1: float, dec1: float, ra2: float, dec2: float) -> float:
    """
    Calculate the angular distance between two points on the sky (used for cross-matching objects)

    :param ra1:
        Right ascension of point 1, in hours
    :param dec1:
        Declination of point 1, in degrees
    :param ra2:
        Right ascension of point 2, in hours
    :param dec2:
        Declination of point 2, in degrees
    :return:
        Angular distance, in arcminutes
    """

    # Convert inputs to radians
    ra1 *= pi / 12
    ra2 *= pi / 12
    dec1 *= pi / 180
    dec2 *= pi / 180

    # Project two points onto a unit sphere in Cartesian coordinates
    x1: float = cos(ra1) * cos(dec1)
    y1: float = sin(ra1) * cos(dec1)
    z1: float = sin(dec1)
    x2: float = cos(ra2) * cos(dec2)
    y2: float = sin(ra2) * cos(dec2)
    z2: float = sin(dec2)

    # Work out the linear distance between the points in Cartesian space
    distance: float = sqrt((x2 - x1) ** 2 + (y2 - y1) ** 2 + (z2 - z1) ** 2)

    # Convert this distance into an angle
    return 2 * asin(distance / 2) * 180 / pi * 60


def read_english_names(ic_names: Dict[int, List[str]], ic_to_messier: Dict[int, int],
                       ngc_to_messier: Dict[int, int], ngc_names: Dict[int, List[str]]) -> None:
    """
    Read the data file <ngc/names.dat> which lists popular names for NGC and IC objects, together with their identities
    in the Messier catalogue.

    :param ic_names:
        For each IC object, a list of popular names.
    :param ngc_names:
        For each NGC object, a list of popular names.
    :param ic_to_messier:
        For each IC object, its Messier number.
    :param ngc_to_messier:
        For each NGC object, its Messier number.
    :return:
        None
    """
    # Open the list of English popular names for NGC and IC objects
    with open("../ngc/names.dat") as f:
        for line in f:
            # Ignore blank lines
            if line.strip() == '':
                continue

            # Fetch the English names for this object
            name: str = line[:36].strip()

            # Fetch the catalogue name for this object, e.g. M82, NGC1234 or IC123
            words: str = line[37:].strip()

            # Ignore objects with no catalogue name
            if words == "":
                continue

            # NGC objects are listed simply as numbers, IC objects are prefixed 'I'
            words: Sequence[str] = words.split()
            is_ic: bool = (line[36] == 'I')  # Boolean flag indicating if this object is an IC object
            try:
                cat_num: int = int(words[0])  # The catalogue number of this object
            except ValueError:
                logging.info("Could not read catalogue names for <{}>".format(name))
                continue

            # English name is in fact a Messier object number. Populate arrays of the Messier numbers of NGC/IC objects
            if name.split()[0] == 'M':
                if is_ic:
                    if cat_num not in ic_to_messier:
                        ic_to_messier[cat_num] = int(name.split()[1])
                else:
                    if cat_num not in ngc_to_messier:
                        ngc_to_messier[cat_num] = int(name.split()[1])

            # Name is not a Messier object number - put this into array of English names for objects
            else:
                # Target array to insert this name into
                target: Dict[int, List[str]] = ic_names if is_ic else ngc_names

                # If we don't yet have any names for this object, create an array for them
                if cat_num not in target:
                    target[cat_num] = []

                # Check whether this is a name that we've already seen for this object
                already_got_name: bool = False
                for item in target[cat_num]:
                    if item.lower() == name.lower():
                        already_got_name = True

                # If this is a new name that we've not seen before, insert it into the list
                if not already_got_name:
                    target[cat_num].append(name)


def populate_stub_catalogue_entries(obj_list: List[Dict[str, Any]],
                                    ic_ptr: Dict[int, int], ic_names: Dict[int, List[str]],
                                    ic_to_messier: Dict[int, int], messier_ptr: Dict[int, int], ngc_ptr: Dict[int, int],
                                    ngc_to_messier: Dict[int, int], ngc_names: Dict[int, List[str]]) -> Dict[str, Any]:
    """
    Create a catalogue of stub data files for all the entries in the Messier, NGC and IC catalogues.

    :param obj_list:
        A list of dictionaries describing objects.
    :param ic_ptr:
        Dictionary of pointers to objects in <obj_list>, indexed by IC number.
    :param ic_names:
        Dictionary of lists of popular names, indexed by IC number.
    :param ic_to_messier:
        Dictionary of Messier object numbers, indexed by IC number.
    :param messier_ptr:
        Dictionary of pointers to objects in <obj_list>, indexed by Messier object number.
    :param ngc_ptr:
        Dictionary of pointers to objects in <obj_list>, indexed by NGC object number.
    :param ngc_to_messier:
        Dictionary of Messier object numbers, indexed by NGC number.
    :param ngc_names:
        Dictionary of lists of popular names, indexed by NGC number.
    :return:
        Dictionary describing a stub object entry
    """
    # Size of Messier, NGC and IC catalogues
    catalogue_m_size: Final[int] = 110
    catalogue_ngc_size: Final[int] = 7840
    catalogue_ic_size: Final[int] = 5386

    # Stub entry with all the fields that we populate for each object that we see
    object_stub_entry: Dict[str, Any] = {
        "m": 0,  # Messier number (0 is null)
        "ngc": 0,  # NGC number
        "ic": 0,  # IC number
        "source": None,  # Source for position
        "source_dist": None,  # Source for distance
        "ra": None,  # RA
        "dec": None,  # Declination
        "mag": {},  # Magnitudes in various bands
        "b_v": None,  # B-V colour
        "axis_major": None,  # Major axis size, diameter in arcminutes
        "axis_minor": None,  # Minor axis size, diameter in arcminutes
        "axis_pa": None,  # Position angle of major axis (north-eastwards)
        "type": None,  # Type identifier string
        "hubble_type": None,  # Hubble type (galaxies only)
        "names": [],  # Popular names for object
        "designations": []  # Other designations for this object
    }

    # Populate stub entries for Messier objects
    for cat_num in range(1, catalogue_m_size + 1):
        messier_ptr[cat_num] = len(obj_list)
        obj_list.append(copy.deepcopy(object_stub_entry))
        obj_list[-1]["m"] = cat_num

    # Populate stub entries for NGC objects
    for cat_num in range(1, catalogue_ngc_size + 1):
        # Test whether this object already exists as a Messier object
        # (but reject if another NGC object is already paired with that Messier object)
        if (cat_num in ngc_to_messier) and (obj_list[messier_ptr[ngc_to_messier[cat_num]]]["ngc"] == 0):
            obj_ptr: int = messier_ptr[ngc_to_messier[cat_num]]
            ngc_ptr[cat_num] = obj_ptr
            obj_list[obj_ptr]["ngc"] = cat_num
        # Create a new object entry
        else:
            ngc_ptr[cat_num] = len(obj_list)
            obj_list.append(copy.deepcopy(object_stub_entry))
            obj_list[-1]["ngc"] = cat_num
        # Add English names for this object if they exist
        if cat_num in ngc_names:
            obj_list[ngc_ptr[cat_num]]["names"] = ngc_names[cat_num]

    # Populate stub entries for IC objects
    for cat_num in range(1, catalogue_ic_size + 1):
        # Test whether this object already exists as a Messier object
        if (cat_num in ic_to_messier) and (obj_list[messier_ptr[ic_to_messier[cat_num]]]["ic"] == 0):
            obj_ptr = messier_ptr[ic_to_messier[cat_num]]
            ic_ptr[cat_num] = obj_ptr
            obj_list[obj_ptr]["ic"] = cat_num
        # Create a new object entry
        else:
            ic_ptr[cat_num] = len(obj_list)
            obj_list.append(copy.deepcopy(object_stub_entry))
            obj_list[-1]["ic"] = cat_num
        # Add English names for this object if they exist
        if cat_num in ic_names:
            obj_list[ic_ptr[cat_num]]["names"] = ic_names[cat_num]
    return object_stub_entry


def read_messier_catalogue(obj_list: List[Dict[str, Any]], messier_ptr: Dict[int, int]) -> None:
    """
    Read the data file <messier/messier.dat> which contains the Messier catalogue.

    :param obj_list:
        A list of dictionaries describing objects.
    :param messier_ptr:
        Dictionary of pointers to objects in <obj_list>, indexed by Messier object number.
    :return:
        None
    """
    # Import V-band magnitudes from Messier catalogue
    cat_num: int = 0
    with open("../messier/messier.dat") as f:
        for line in f:
            # Ignore blank lines
            if line.strip() == '':
                continue
            # Get Messier catalogue number from line number
            cat_num += 1

            # Index in catalogue of objects
            obj_ptr: int = messier_ptr[cat_num]

            # Extract RA and Dec (J2000) from ASCII file
            ra_hours: float = float(line[32:34])
            ra_minutes: float = float(line[34:38])
            dec_deg: float = float(line[40:42])
            dec_minutes: float = float(line[42:46])
            dec_sign: str = line[39]
            distance_str: str = line[48:61]

            # Some objects have multiple magnitudes
            mags: Sequence[str] = line[18:31].split(",")
            mag_list: Dict[str, float] = {}
            for mag in mags:
                mag: str = mag.strip()
                band: str = ""

                # If magnitude has non-numeric characters at the end, these name a band, e.g. V or B
                while mag[-1] not in '0123456789':
                    band = mag[-1] + band
                    mag = mag[:-1]
                band = band.upper()

                # Add the magnitude we have just extracted from the catalogue
                if band in ['V', 'B']:
                    mag_list[band] = float(mag)

            # Convert RA and Dec from hours and minutes into decimal hours and degrees
            ra: float = (ra_hours + ra_minutes / 60)
            decl: float = (dec_deg + dec_minutes / 60)
            if dec_sign == '-':
                decl *= -1

            # Messier catalogue uses different object type names from the NGC catalogue
            # Convert Messier catalogue type names into NGC types
            obj_type_messier: str = line[9:11]

            if obj_type_messier in object_type_conversion_messier:
                obj_type: str = object_type_conversion_messier[obj_type_messier]
            else:
                obj_type = obj_type_messier

            # Fetch distance, expressed as value and unit
            dist: Sequence[str] = line[48:61].split(" ")
            try:
                dist_val: float = float(dist[0])

                # Objects have distances various in ly, kly or Mly
                if dist[1] == 'kly':
                    dist_val *= 3.06595098e-04
                elif dist[1] == 'Mly':
                    dist_val *= 0.306595098
                elif dist[1] == 'ly':
                    dist_val *= 3.06595098e-07
                else:
                    logging.info("Unknown distance unit <{}>".format(dist[1]))
                    raise ValueError

                # Insert distance into object catalogue
                obj_list[obj_ptr]['dist'] = dist_val  # in Mpc
                obj_list[obj_ptr]['source_dist'] = 'messier'
            except (ValueError, IndexError):
                pass

            # Insert object's position into object catalogue
            obj_list[obj_ptr]['ra'] = ra
            obj_list[obj_ptr]['dec'] = decl
            obj_list[obj_ptr]['source'] = 'messier'

            # Insert object's magnitude into object catalogue
            for band, mag in list(mag_list.items()):
                obj_list[obj_ptr]['mag'][band] = {'value': mag, 'source': 'messier'}

            # Insert object's type into object catalogue
            obj_list[obj_ptr]['type'] = obj_type

            # Read angular size of object, and if present, insert into object catalogue
            test = re.match(r"([0-9\.]*)'", line[61:].strip())
            if test:
                obj_list[obj_ptr]['axis_major'] = float(test.group(1))  # diameter, arcminutes
                obj_list[obj_ptr]['axis_minor'] = None
                obj_list[obj_ptr]['axis_pa'] = None


def read_redshift_independent_distances(obj_list: List[Dict[str, Any]],
                                        ic_ptr: Dict[int, int], ngc_ptr: Dict[int, int]) -> None:
    """
    Read redshift-independent distance measures for NGC and IC objects, from the data file <output/NED_distances.dat>.

    :param obj_list:
        A list of dictionaries describing objects.
    :param ic_ptr:
        Dictionary of pointers to objects in <obj_list>, indexed by IC number.
    :param ngc_ptr:
        Dictionary of pointers to objects in <obj_list>, indexed by NGC object number.
    :return:
        None
    """
    # Gather redshift-independent distances
    with open("output/NED_distances.dat") as f:
        for line in f:
            # Ignore blank lines and comment lines
            if (len(line) < 8) or (line[0] == '#'):
                continue

            # Flag indicating whether this is an IC object (true), or an NGC object (false)
            is_ic: bool = (line[0] == 'I')

            # The catalogue number of this source (in either NGC or IC catalogue)
            cat_num: int = int(line[4:8])

            # The index of this object within the <obj_list> list
            if is_ic:
                obj_ptr: int = ic_ptr[cat_num]
            else:
                obj_ptr: int = ngc_ptr[cat_num]

            # Look up the distance unit as recorded in the list of distances returned by the NED web interface
            dist_unit: str = line[66:70].strip()

            # Reject the distance if it's not in Mpc
            if not ((len(dist_unit) > 0) and (dist_unit[0] != '-')):
                continue
            if dist_unit != "Mpc":
                logging.info("WARNING Distance unit = {} (should be Mpc)".format(dist_unit))

            # Look up the median distance to this object
            try:
                dist_float: float = float(line[30:37])
                obj_list[obj_ptr]['dist'] = dist_float
                obj_list[obj_ptr]['source_dist'] = 'ned'
            except ValueError:
                pass

            # Look up the redshift of this object
            # ztext contains two white-space separated values: redshift and redshift error
            obj_list[obj_ptr]['ztext'] = line[72:].strip()


def read_ngc_2000(obj_list: List[Dict[str, Any]], ic_ptr: Dict[int, int], ngc_ptr: Dict[int, int]) -> None:
    """
    Read data from the NGC2000 catalogue at <ngc/ngc2000.dat>.

    :param obj_list:
        A list of dictionaries describing objects.
    :param ic_ptr:
        Dictionary of pointers to objects in <obj_list>, indexed by IC number.
    :param ngc_ptr:
        Dictionary of pointers to objects in <obj_list>, indexed by NGC object number.
    :return:
        None
    """
    # Import data from NGC2000 catalogue
    with open("../ngc/ngc2000.dat") as f:
        for line in f:
            # Ignore blank lines and comment lines
            if len(line) < 20:
                continue

            # Flag indicating whether this is an IC object (true), or an NGC object (false)
            is_ic: bool = (line[0] == 'I')

            # The catalogue number of this source (in either NGC or IC catalogue)
            cat_num: int = int(line[1:5])

            # The index of this object within the <obj_list> list
            if is_ic:
                obj_ptr: int = ic_ptr[cat_num]
            else:
                obj_ptr: int = ngc_ptr[cat_num]

            # Look up the reported type of this object
            obj_type_ngc2000: str = line[6:10].strip()

            if obj_type_ngc2000 in object_type_conversion_ngc2000:
                obj_type: str = object_type_conversion_ngc2000[obj_type_ngc2000]
            else:
                obj_type = obj_type_ngc2000

            # Look up the reported angular size of this object (arcminutes)
            size_str: str = line[34:39].strip()
            if size_str != "":
                size: Optional[float] = float(size_str)  # diameter, arcminutes
            else:
                size = None

            # Look up reported brightness and position of this object
            mag: str = line[40:44].strip()
            ra: float = (int(line[10:12])) + 1.0 / 60 * (float(line[13:17]))
            dec: float = (int(line[20:22])) + 1.0 / 60 * (float(line[23:25]))
            if line[19] == '-':
                dec = -dec

            # Insert data into this objects record in the list <obj_list>
            obj_list[obj_ptr]['ra'] = ra
            obj_list[obj_ptr]['dec'] = dec
            obj_list[obj_ptr]['source'] = 'ngc2000'
            if mag and (line[44] != 'p'):
                obj_list[obj_ptr]['mag']['V'] = {'value': mag, 'source': 'ngc2000'}
            obj_list[obj_ptr]['type'] = obj_type
            if size:
                obj_list[obj_ptr]['axis_major'] = size
                obj_list[obj_ptr]['axis_minor'] = None
                obj_list[obj_ptr]['axis_pa'] = None


def read_open_ngc(obj_list: List[Dict[str, Any]], ic_ptr: Dict[int, int], ngc_ptr: Dict[int, int]) -> None:
    """
    Read the data file <ngc/open_ngc.csv> which contains the OpenNGC catalogue.

    :param obj_list:
        A list of dictionaries describing objects.
    :param ic_ptr:
        Dictionary of pointers to objects in <obj_list>, indexed by IC number.
    :param ngc_ptr:
        Dictionary of pointers to objects in <obj_list>, indexed by NGC object number.
    :return:
        None
    """
    # Import data from OpenNGC catalogue
    columns: Optional[List[str]] = None  # Read column headings from first line of CSV file
    with open("../ngc/open_ngc.csv") as f:
        for line in f:
            # First line contains column headings
            if columns is None:
                columns = line.strip().split(";")
                continue

            # Flag indicating whether this is an IC object (true), or an NGC object (false)
            is_ic: bool = line.startswith("IC")
            is_ngc: bool = line.startswith("NGC")

            # Ignore objects which are neither NGC nor IC objects
            if not (is_ic or is_ngc):
                continue

            # Further fields are separated by ;s
            words: List[str] = line.split(";")

            def fetch(column: str) -> str:
                return words[columns.index(column)]

            # Read object number within NGC / IC catalogue
            cat_num: int = int(fetch('Name')[3:7] if is_ngc else fetch('Name')[2:6])

            # The index of this object within the <obj_list> list
            if is_ic:
                obj_ptr: int = ic_ptr[cat_num]
            else:
                obj_ptr: int = ngc_ptr[cat_num]

            # Read magnitudes in various filters
            for band in ('B', 'V', 'J', 'H', 'K'):
                field_name: str = '{}-Mag'.format(band)
                if fetch(field_name):
                    obj_list[obj_ptr]['mag'][band] = {'value': float(fetch(field_name)), 'source': 'open_ngc'}

            # Read B-V colour
            if fetch('B-Mag') and fetch('V-Mag'):
                obj_list[obj_ptr]['b_v'] = float(fetch('B-Mag')) - float(fetch('V-Mag'))

            # Read object type
            obj_type_open_ngc: str = fetch('Type')

            obj_type: str = object_type_conversion_open_ngc[obj_type_open_ngc]
            obj_list[obj_ptr]['type'] = obj_type

            # If this object is a duplicate, what is it a duplicate of?
            if obj_type == 'dup':
                for column_name, from_catalogue in (("M", "Messier"), ("NGC", "NGC"), ("IC", "IC")):
                    datum: str = fetch(column_name)
                    if datum:
                        if not datum.isdigit():
                            if datum[:-1].isdigit():
                                # Truncate NGC1234A to NGC1234
                                datum = datum[:-1]
                            else:
                                # If NGC number isn't recoverable, skip
                                continue
                        obj_list[obj_ptr]['duplicate_of'] = "{} {}".format(from_catalogue, int(datum))

            # Read RA and Dec
            ra: str = fetch('RA')
            dec: str = fetch('Dec')
            if fetch('RA') and fetch('Dec'):
                obj_list[obj_ptr]['ra'] = int(ra[0:2]) + int(ra[3:5]) / 60. + float(ra[6:]) / 3600.
                obj_list[obj_ptr]['dec'] = int(dec[1:3]) + int(dec[4:6]) / 60. + float(dec[7:]) / 3600.
                obj_list[obj_ptr]['source'] = 'open_ngc'
                if dec[0] == '-':
                    obj_list[obj_ptr]['dec'] *= -1

            # Read major axis
            if fetch('MajAx'):
                obj_list[obj_ptr]['axis_major'] = float(fetch('MajAx'))  # diameter, arcminutes
                obj_list[obj_ptr]['axis_minor'] = None
                obj_list[obj_ptr]['axis_pa'] = None

            # Read minor axis
            if fetch('MinAx'):
                obj_list[obj_ptr]['axis_minor'] = float(fetch('MinAx'))  # diameter, arcminutes
                obj_list[obj_ptr]['axis_pa'] = None

            # Read position angle
            if fetch('PosAng'):
                obj_list[obj_ptr]['axis_pa'] = float(fetch('PosAng'))

            # Read Hubble type
            if fetch('Hubble'):
                obj_list[obj_ptr]['hubble_type'] = fetch('Hubble')

            # Read popular name for object
            if fetch('Common names'):
                for name in fetch('Common names').split(","):
                    target: List[str] = obj_list[obj_ptr]['names']
                    already_got_name: bool = False
                    for item in target:
                        if item.lower() == name.lower():
                            already_got_name = True
                    if not already_got_name:
                        target.append(name)

            # Read other designations for object
            if fetch('Identifiers'):
                for name in fetch('Identifiers').split(","):
                    target: List[str] = obj_list[obj_ptr]['designations']
                    already_got_name: bool = False
                    for item in target:
                        if item.lower() == name.lower():
                            already_got_name = True
                    if not already_got_name:
                        target.append(name)


def read_open_cluster_catalogue(obj_list: List[Dict[str, Any]]) -> None:
    """
    Read data from the open cluster catalogue at <openClusters/clusters.txt>.

    :param obj_list:
        A list of dictionaries describing objects.
    :return:
        None
    """
    # Import data from catalogue of open clusters
    with open("../openClusters/clusters.txt") as f:
        for line in f:
            # Ignore blank lines and comment lines
            if len(line.strip()) == 0:
                continue
            name: str = line[0:18].strip()

            # Read position of object
            ra: float = (int(line[18:20])) + 1.0 / 60 * (int(line[21:23])) + 1.0 / 3600 * (int(line[24:26]))
            dec: float = (int(line[29:31])) + 1.0 / 60 * (int(line[32:34])) + 1.0 / 3600 * (int(line[35:37]))
            if line[28] == '-':
                dec *= -1

            # Read distance of object
            dist_str: str = line[55:60].strip()
            if dist_str == "":
                continue
            dist_float: float = (int(dist_str)) / 1.0e6  # Convert pc into Mpc

            # Search existing objects for matching open clusters within 10 arcminutes of reported location
            match: bool = False
            for obj_info in obj_list:
                # Cannot do cross-match without sky position
                if ('ra' not in obj_info) or ('dec' not in obj_info):
                    continue

                # Angular distance between this object and item in catalogue
                angular_dist: float = angular_distance(obj_info['ra'], obj_info['dec'], ra, dec)
                if angular_dist > 10:
                    continue
                if obj_info['type'] != "OC":
                    # Possibly object is actually an open cluster, but has the wrong type recorded in NGC catalogue
                    if name.endswith(str(obj_info['ngc'])) and obj_info['ngc']:
                        logging.info("Dodgy match between {} with NGC{}".format(name, obj_info['ngc']))
                        obj_info['type'] = "OC"
                    elif name.endswith(str(obj_info['ic'])) and obj_info['ic']:
                        logging.info("Dodgy match between {} with IC{}".format(name, obj_info['ic']))
                        obj_info['type'] = "OC"

                    # ... but if name field doesn't match NGC number, reject this match
                    else:
                        logging.info("Rejecting match of {} with M{} NGC{} IC{}".format(
                            name, obj_info['m'], obj_info['ngc'], obj_info['ic']))
                        continue
                # logging.info("{} matched to M{} NGC{} IC{} (distance {} arcmin)".
                #              format(name, obj_info['m'], obj_info['ngc'], obj_info['ic'], angular_dist))
                obj_info['dist'] = dist_float
                obj_info['source_dist'] = 'open_clusters'
                match = True
                break
            if not match:
                logging.info("No match found for {}".format(name))


def read_globular_cluster_catalogue(obj_list: List[Dict[str, Any]]) -> None:
    """
    Read data from the globular cluster catalogue at <globularClusters/catalogue.dat>.

    :param obj_list:
        A list of dictionaries describing objects.
    :return:
        None
    """
    # Import data from catalogue of globular clusters
    with open("../globularClusters/catalogue.dat") as f:
        for line in f:
            # Ignore blank lines and comment lines
            if len(line.strip()) == 0:
                continue
            if line[0] == '#':
                continue
            name: str = line[1:23].strip()

            # Read position of object
            ra: float = (int(line[23:25])) + 1.0 / 60 * (int(line[26:28])) + 1.0 / 3600 * (float(line[30:33]))
            dec: float = (int(line[36:38])) + 1.0 / 60 * (int(line[39:41])) + 1.0 / 3600 * (int(line[42:44]))
            if line[35] == '-':
                dec *= -1

            # Read distance of object
            dist_str = line[61:67].strip()
            if dist_str == "":
                continue
            dist_float: float = (float(dist_str)) / 1.0e3  # Convert kpc into Mpc

            # Search existing objects for matching globular clusters within 20 arcminutes of reported location
            match: bool = False
            for obj_info in obj_list:
                # Cannot do cross-match without sky position
                if ('ra' not in obj_info) or ('dec' not in obj_info):
                    continue

                # Angular distance between this object and item in catalogue
                angular_dist: float = angular_distance(obj_info['ra'], obj_info['dec'], ra, dec)
                if angular_dist > 20:
                    continue
                if obj_info['type'] != "Gb":
                    # Possibly object is actually an open cluster, but has the wrong type recorded in NGC catalogue
                    if name.endswith(str(obj_info['ngc'])) and obj_info['ngc']:
                        logging.info("Dodgy match between {} with NGC{}".format(name, obj_info['ngc']))
                        obj_info['type'] = "Gb"
                    elif name.endswith(str(obj_info['ic'])) and obj_info['ic']:
                        logging.info("Dodgy match between {} with IC{}".format(name, obj_info['ic']))
                        obj_info['type'] = "Gb"

                    # ... but if name field doesn't match NGC number, reject this match
                    else:
                        logging.info("Rejecting match of {} with M{} NGC{} IC{}".format(
                            name, obj_info['m'], obj_info['ngc'], obj_info['ic']))
                        continue
                # logging.info("{} matched to M{} NGC{} IC{} (distance {} arcmin)".
                #              format(name, obj_info['m'], obj_info['ngc'], obj_info['ic'], angular_dist))
                obj_info['dist'] = dist_float
                obj_info['source_dist'] = 'globular_clusters'
                match = True
                break
            if not match:
                logging.info("No match found for {}".format(name))


def clean_object_catalogue(obj_list: List[Dict[str, Any]]) -> None:
    """
    Perform cleaning on object catalogue before it is written to disk.

    :param obj_list:
        A list of dictionaries describing objects.
    :return:
        None
    """
    # Clean output
    for obj_info in obj_list:
        # Clean up various kinds of spurious objects in NGC catalogue into a common type
        if "type" not in obj_info:
            obj_info["type"] = "-"

        # Convert list of names into a JSON list
        if "names" in obj_info:
            names: List[str] = obj_info["names"]
        else:
            names = []
        obj_info["names"] = json.dumps(names)

        # Populate a field with the primary name for each object: a Messier designation if present, otherwise
        # NGC/IC number
        name: str = "-"
        if obj_info["m"]:
            name = "M%d" % obj_info["m"]
        elif obj_info["ngc"]:
            name = "NGC%d" % obj_info["ngc"]
        elif obj_info["ic"]:
            name = "IC%d" % obj_info["ic"]
        obj_info["name"] = name


def add_manual_objects(obj_list: List[Dict[str, Any]], object_stub_entry: Dict[str, Any]) -> None:
    """
    Add manually created objects to object catalogue before it is written to disk.

    :param obj_list:
        A list of dictionaries describing objects.
    :param object_stub_entry:
        Dictionary with null entries which acts as a stub entry for an object.
    :return:
        None
    """
    # Add Sun to catalogue
    # catalogue.append(
    #   {"name":"Sun","names":json.dumps(["Sun"]),
    #    "ra":0,"dec":0,"mag":-26,"type":"sun","dist":0,"z":"0 0","m":0,"ngc":0,"ic":0}
    # )
    # Add SgrA to catalogue
    sgr_a = copy.deepcopy(object_stub_entry)
    sgr_a.update({
        "name": "SgrA",
        "names": json.dumps(["SgrA"]),
        "ra": 17.761124,
        "dec": -29.00775,
        "source": "manual",
        "type": "SgrA",
        "dist": 7.94e-3,
        "source_dist": "manual",
        "z": "0 0"
    })
    obj_list.append(sgr_a)


def write_output(obj_list: List[Dict[str, Any]]) -> None:
    """
    Write object catalogue to disk.

    :param obj_list:
        A list of dictionaries describing objects.
    :return:
        None
    """
    # Write output as an enormous JSON file
    with open("output/ngc_merged.dat", "w") as f:
        catalogue_str: str = json.dumps(obj_list)
        f.write(catalogue_str)

    # Work out reference magnitude to use for each object
    for obj_info in obj_list:
        mag: float = 999
        for band in ["V", "G", "B"]:
            if band in obj_info["mag"]:
                mag = float(obj_info["mag"][band]["value"])
                break
        obj_info['reference_magnitude'] = mag

    # Sort objects in order of decreasing brightness
    obj_list.sort(key=operator.itemgetter('reference_magnitude'))

    # Write output as an enormous text file
    with open("output/ngc_merged.txt", "w") as f:
        f.write(
            "# {:4s} {:6s} {:8s} {:17s} {:17s} {:17s} {:17s} {:17s} {:17s} {:5s}\n"
            .format("M", "NGC", "IC", "RA", "Dec", "Mag", "Axis major", "Axis minor", "Axis PA", "Type"))

        for obj_info in obj_list:
            # Ignore objects with no magnitudes
            if obj_info['reference_magnitude'] > 100:
                continue

            # Deal with None values
            axis_major: float = obj_info["axis_major"] if obj_info["axis_major"] is not None else 0
            axis_minor: float = obj_info["axis_minor"] if obj_info["axis_minor"] is not None else 0
            axis_pa: float = obj_info["axis_pa"] if obj_info["axis_pa"] is not None else 0

            # Write entry for this object
            f.write("{:6d} {:6d} {:8d} {:17.12f} {:17.12f} {:17.12f} {:17.12f} {:17.12f} {:17.12f} {:5s}\n".format(
                obj_info["m"], obj_info["ngc"], obj_info["ic"],
                obj_info["ra"], obj_info["dec"], obj_info["reference_magnitude"], axis_major, axis_minor, axis_pa,
                obj_info["type"]
            ))


def merge_deep_sky_catalogues() -> None:
    """
    Main entry point for creating a merged catalogue from all the available catalogues of deep sky objects.
    """
    # Index of which NGC objects are also Messier objects
    ngc_to_messier: Dict[int, int] = {}  # ngc_to_messier[NGC num] = Messier number
    ic_to_messier: Dict[int, int] = {}  # ic_messiers[IC_num] = Messier number

    # English names for NGC objects
    ngc_names: Dict[int, List[str]] = {}
    ic_names: Dict[int, List[str]] = {}

    # Read file containing the popular English names for deep sky objects
    read_english_names(ic_names=ic_names, ic_to_messier=ic_to_messier,
                       ngc_to_messier=ngc_to_messier, ngc_names=ngc_names)

    # Make empty catalogue of objects we've seen
    obj_list: List[Dict[str, Any]] = []

    # Pointers to where Messier, NGC and IC objects are within the array <obj_list>
    messier_ptr: Dict[int, int] = {}
    ngc_ptr: Dict[int, int] = {}
    ic_ptr: Dict[int, int] = {}

    # Create stub entries for all Messier, NGC and IC objects
    object_stub_entry: Dict[str, Any] = populate_stub_catalogue_entries(
        obj_list=obj_list, ic_ptr=ic_ptr, ic_names=ic_names, ic_to_messier=ic_to_messier,
        messier_ptr=messier_ptr, ngc_ptr=ngc_ptr, ngc_to_messier=ngc_to_messier, ngc_names=ngc_names)

    # Read data from the Messier catalogue
    read_messier_catalogue(obj_list=obj_list, messier_ptr=messier_ptr)

    # Read redshift-independent distances
    read_redshift_independent_distances(obj_list=obj_list, ic_ptr=ic_ptr, ngc_ptr=ngc_ptr)

    # Read NGC2000 catalogue
    read_ngc_2000(obj_list=obj_list, ic_ptr=ic_ptr, ngc_ptr=ngc_ptr)

    # Read OpenNGC catalogue
    read_open_ngc(obj_list=obj_list, ic_ptr=ic_ptr, ngc_ptr=ngc_ptr)

    # Read catalogue of open clusters
    read_open_cluster_catalogue(obj_list=obj_list)

    # Read catalogue of globular clusters
    read_globular_cluster_catalogue(obj_list=obj_list)

    # Data cleaning
    clean_object_catalogue(obj_list=obj_list)

    # Add objects such as Sgr A* manually
    add_manual_objects(obj_list=obj_list, object_stub_entry=object_stub_entry)

    # Create output files
    write_output(obj_list=obj_list)


# Do it right away if we're run as a script
if __name__ == "__main__":
    # Create output log listing the cross-matching between various NGC catalogues
    log_file_path = "output/merge.log"
    os.system("rm -f {}".format(log_file_path))


    # Set up logging
    class InfoFilter(logging.Filter):
        def filter(self, rec):
            return rec.levelno in (logging.DEBUG, logging.INFO)


    h1 = logging.StreamHandler(sys.stdout)
    h1.setLevel(logging.DEBUG)
    h1.addFilter(InfoFilter())
    h2 = logging.StreamHandler()
    h2.setLevel(logging.WARNING)

    logging.basicConfig(level=logging.INFO,
                        format='[%(asctime)s] %(levelname)s:%(filename)s:%(message)s',
                        datefmt='%d/%m/%Y %H:%M:%S',
                        handlers=[
                            logging.FileHandler(log_file_path),
                            h1, h2
                        ])
    logger = logging.getLogger(__name__)

    # Merge deep sky catalogues
    merge_deep_sky_catalogues()
