#!/usr/bin/python3
# -*- coding: utf-8 -*-
# merge.py
#
# -------------------------------------------------
# Copyright 2015-2019 Dominic Ford
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

import json
import os
import re
import copy
from math import pi, cos, sin, sqrt, asin

# Create output log listing the cross-matching between various NGC catalogues
logfile = open("output/merge.log", "w")


def angular_distance(ra1, dec1, ra2, dec2):
    """
    Calculate the angular distance between two points on the sky (used for cross-matching objects_

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
    x1 = cos(ra1) * cos(dec1)
    y1 = sin(ra1) * cos(dec1)
    z1 = sin(dec1)
    x2 = cos(ra2) * cos(dec2)
    y2 = sin(ra2) * cos(dec2)
    z2 = sin(dec2)

    # Work out the linear distance between the points in Cartesian space
    distance = sqrt((x2 - x1) ** 2 + (y2 - y1) ** 2 + (z2 - z1) ** 2)

    # Convert this distance into an angle
    return 2 * asin(distance / 2) * 180 / pi * 60


# Work out which NGC objects are also Messier objects
ngc_messiers = {}  # ngc_messiers[NGC num] = Messier no
ind_messiers = {}  # ic_messiers[IC_num] = Messier no

# English names for NGC objects
ngc_names = {}
ic_names = {}

# Open the list of English popular names for NGC and IC objects
for line in open("../ngc/names.dat"):
    # Ignore blank lines
    if line.strip() == '':
        continue

    # Fetch the English names for this object
    name = line[:36].strip()

    # Fetch the catalogue name for this object, e.g. M82, NGC1234 or IC123
    words = line[37:].strip()

    # Ignore objects with no catalogue name
    if words == "":
        continue

    # NGC objects are listed simply as numbers, IC objects are prefixed 'I'
    words = words.split()
    ic = (line[36] == 'I')  # Boolean flag indicating if this object is an IC object
    n = int(words[0])  # The catalogue number of this object

    # English name is in fact an Messier object number. Populate arrays of the Messier numbers of NGC/IC objects
    if name.split()[0] == 'M':
        if ic:
            if n not in ind_messiers:
                ind_messiers[n] = int(name.split()[1])
        else:
            if n not in ngc_messiers:
                ngc_messiers[n] = int(name.split()[1])

    # Name is not a Messier object number - put this into array of English names for objects
    else:
        # Target array to insert this name into
        target = ic_names if ic else ngc_names

        # If we don't yet have any names for this object, create an array for them
        if n not in target:
            target[n] = []

        # Check whether this is a name that we've already seen for this object
        already_got_name = False
        for item in target[n]:
            if item.lower() == name.lower():
                already_got_name = True

        # If this is a new name that we've not seen before, insert it into the list
        if not already_got_name:
            target[n].append(name)

# Make empty catalogue of objects we've seen
catalogue = []

# Cross references -- e.g. mes_id[catalogue_index] = Messier number
mes_id = {}
ngc_id = {}
ic__id = {}

# Size of Messier, NGC and IC catalogues
catalogue_m_size = 110
catalogue_ngc_size = 7840
catalogue_ic_size = 5386

# Stub entry with all of the fields that we populate for each object that we see
catalogue_stub_entry = {
    "m": 0,  # Messier number (0 is null)
    "ngc": 0,  # NGC number
    "ic": 0,  # IC number
    "source": None,  # Source for position
    "source_dist": None,  # Source for distance
    "ra": None,  # RA
    "dec": None,  # Declination
    "mag": {},  # Magnitudes in various bands
    "b_v": None,  # B-V colour
    "axis_major": None,  # Major axis size, in arcminutes
    "axis_minor": None,  # Minor axis size, in arcminutes
    "axis_pa": None,  # Position angle of major axis (north-eastwards)
    "type": None,  # Type identifier string
    "hubble_type": None,  # Hubble type (galaxies only)
    "names": []  # Popular names for object
}

# Populate stub entries for Messier objects
for i in range(1, catalogue_m_size + 1):
    mes_id[i] = len(catalogue)
    catalogue.append(copy.deepcopy(catalogue_stub_entry))
    catalogue[-1]["m"] = i

# Populate stub entries for NGC objects
for i in range(1, catalogue_ngc_size + 1):
    # Test whether this object already exists as a Messier object
    # (but reject if another NGC object is already paired with that Messier object)
    if (i in ngc_messiers) and (catalogue[mes_id[ngc_messiers[i]]]["ngc"] == 0):
        n = mes_id[ngc_messiers[i]]
        ngc_id[i] = n
        catalogue[n]["ngc"] = i
    # Create a new object entry
    else:
        ngc_id[i] = len(catalogue)
        catalogue.append(copy.deepcopy(catalogue_stub_entry))
        catalogue[-1]["ngc"] = i
    # Add English names for this object if they exist
    if i in ngc_names:
        catalogue[ngc_id[i]]["names"] = ngc_names[i]

# Populate stub entries for IC objects
for i in range(1, catalogue_ic_size + 1):
    # Test whether this object already exists as a Messier object
    if (i in ind_messiers) and (catalogue[mes_id[ind_messiers[i]]]["ic"] == 0):
        n = mes_id[ind_messiers[i]]
        ic__id[i] = n
        catalogue[n]["ic"] = i
    # Create a new object entry
    else:
        ic__id[i] = len(catalogue)
        catalogue.append(copy.deepcopy(catalogue_stub_entry))
        catalogue[-1]["ic"] = i
    # Add English names for this object if they exist
    if i in ic_names:
        catalogue[ic__id[i]]["names"] = ic_names[i]

# Import V-band magnitudes from Messier catalogue
no = 0
for line in open("../messier/messier.dat"):
    # Ignore blank lines
    if line.strip() == '':
        continue
    # Get Messier catalogue number from line number
    no += 1

    # Index in catalogue of objects
    i = mes_id[no]

    # Extract RA and Dec (J2000) from ASCII file
    RAhrs = float(line[32:34])
    RAmins = float(line[34:38])
    DecDeg = float(line[40:42])
    DecMins = float(line[42:46])
    DecSign = line[39]
    DistTxt = line[48:61]

    # Some objects have multiple magnitudes
    mags = line[18:31].split(",")
    mag_list = {}
    for mag in mags:
        mag = mag.strip()
        band = ""

        # If magnitude has non-numeric characters at the end, these name a band, e.g. V or B
        while mag[-1] not in '0123456789':
            band = mag[-1] + band
            mag = mag[:-1]
        band = band.upper()

        # Add the magnitude we have just extracted from the catalogue
        if band in ['V', 'B']:
            mag_list[band] = float(mag)

    # Convert RA and Dec from hours and minutes into decimal hours and degrees
    RA = (RAhrs + RAmins / 60)
    Dec = (DecDeg + DecMins / 60)
    if DecSign == '-':
        Dec *= -1

    # Messier catalogue uses different object type names from the NGC catalogue
    # Convert Messier catalogue type names into NGC types
    obj_type = line[9:11]
    if obj_type == "GC":  # globular clusters
        obj_type = "Gb"
    if obj_type == "PN":  # planetary nebulae
        obj_type = "Pl"
    if obj_type == "BN":  # bright nebulae
        obj_type = "Nb"
    if obj_type == "GX":  # galaxies
        obj_type = "Gx"
    if obj_type == "*2":  # double stars
        obj_type = "D*"
    if obj_type == "*C":  # asterisms
        obj_type = "As*"

    # Fetch distance, expressed as value and unit
    dist = line[48:61].split()
    try:
        dist_val = float(dist[0])

        # Objects have distances various in ly, kly or Mly
        if dist[1] == 'kly':
            dist_val *= 3.06595098e-04
        elif dist[1] == 'Mly':
            dist_val *= 0.306595098
        elif dist[1] == 'ly':
            dist_val *= 3.06595098e-07
        else:
            print(("Unknown distance unit <%s>" % dist[1]))
            raise ValueError

        # Insert distance into object catalogue
        catalogue[i]['dist'] = dist_val  # in Mpc
        catalogue[i]['source_dist'] = 'messier'
    except (ValueError, IndexError):
        pass

    # Insert object's position into object catalogue
    catalogue[i]['ra'] = RA
    catalogue[i]['dec'] = Dec
    catalogue[i]['source'] = 'messier'

    # Insert object's magnitude into object catalogue
    for band, mag in list(mag_list.items()):
        catalogue[i]['mag'][band] = {'value': mag, 'source': 'messier'}

    # Insert object's type into object catalogue
    catalogue[i]['type'] = obj_type

    # Read angular size of object, and if present, insert into object catalogue
    test = re.match(r"([0-9\.]*)'", line[61:].strip())
    if test:
        catalogue[i]['axis_major'] = catalogue[i]['axis_minor'] = float(test.group(1))
        catalogue[i]['axis_pa'] = 0

# Gather redshift-independent distances
for line in open("output/NED_distances.dat"):
    # Ignore blank lines and comment lines
    if (len(line) < 8) or (line[0] == '#'):
        continue

    # Flag indicating whether this is an IC object (true), or an NGC object (false)
    ic = (line[0] == 'I')

    # The catalogue number of this source (in either NGC or IC catalogue)
    number = int(line[4:8])

    # The index of this object within the <catalogue> list
    if ic:
        i = ic__id[number]
    else:
        i = ngc_id[number]

    # Look up the distance unit as recorded in the list of distances returned by the NED web interface
    x = line[51:55].strip()

    # Reject the distance if it's not in Mpc
    if not ((len(x) > 0) and (x[0] != '-')):
        continue
    if x != "Mpc":
        logfile.write("WARNING Distance unit = " + x + " (should be Mpc)")

    # Look up the median distance to this object
    try:
        x = float(line[30:37])
        catalogue[i]['dist'] = x
        catalogue[i]['source_dist'] = 'ned'
    except ValueError:
        pass

    # Look up the redshift of this object
    # ztext contains two white-space separated values: redshift and redshift error
    catalogue[i]['ztext'] = line[72:].strip()

# Import data from NGC2000 catalogue
for line in open("../ngc/ngc2000.dat"):
    # Ignore blank lines and comment lines
    if len(line) < 20:
        continue

    # Flag indicating whether this is an IC object (true), or an NGC object (false)
    ic = (line[0] == 'I')

    # The catalogue number of this source (in either NGC or IC catalogue)
    number = int(line[1:5])

    # The index of this object within the <catalogue> list
    if ic:
        i = ic__id[number]
    else:
        i = ngc_id[number]

    # Look up the reported type of this object
    obj_type = line[7:10].strip()

    # Look up the reported angular size of this object (arcminutes)
    size = line[34:39].strip()
    if size != "":
        size = float(size)
    if len(obj_type) == 0:
        obj_type = "--"

    # Look up reported brightness and position of this object
    mag = line[40:44].strip()
    ra = (int(line[10:12])) + 1.0 / 60 * (float(line[13:17]))
    dec = (int(line[20:22])) + 1.0 / 60 * (float(line[23:25]))
    if line[19] == '-':
        dec = -dec

    # Insert data into this objects record in the list <catalogue>
    catalogue[i]['ra'] = ra
    catalogue[i]['dec'] = dec
    catalogue[i]['source'] = 'ngc2000'
    if mag and (line[44] != 'p'):
        catalogue[i]['mag']['V'] = {'value': mag, 'source': 'ngc2000'}
    catalogue[i]['type'] = obj_type
    if size:
        catalogue[i]['axis_major'] = catalogue[i]['axis_minor'] = size
        catalogue[i]['axis_pa'] = 0

# Import data from OpenNGC catalogue
for line in open("../ngc/open_ngc.csv"):

    # Flag indicating whether this is an IC object (true), or an NGC object (false)
    ic = line.startswith("IC")
    ngc = line.startswith("NGC")

    # Ignore objects which are neither NGC nor IC objects
    if not (ic or ngc):
        continue

    # Further fields are separated by ;s
    words = line.split(";")

    # Read object number within NGC / IC catalogue
    number = int(words[0][3:7] if ngc else words[0][2:6])

    # The index of this object within the <catalogue> list
    if ic:
        i = ic__id[number]
    else:
        i = ngc_id[number]

    # Read magnitudes in various filters
    for index, band in enumerate(['B', 'V', 'J', 'H', 'K']):
        if words[8 + index]:
            catalogue[i]['mag'][band] = {'value': float(words[8 + index]), 'source': 'open_ngc'}

    # Read B-V colour
    if words[8] and words[9]:
        catalogue[i]['b_v'] = float(words[8]) - float(words[9])

    # Read RA and Dec
    if words[2] and words[3]:
        catalogue[i]['ra'] = int(words[2][0:2]) + int(words[2][3:5]) / 60. + float(words[2][6:]) / 3600.
        catalogue[i]['dec'] = int(words[3][1:3]) + int(words[3][4:6]) / 60. + float(words[3][7:]) / 3600.
        catalogue[i]['source'] = 'open_ngc'
        if words[3][0] == '-':
            catalogue[i]['dec'] *= -1

    # Read major axis
    if words[5]:
        catalogue[i]['axis_major'] = float(words[5])

    # Read minor axis
    if words[6]:
        catalogue[i]['axis_minor'] = float(words[6])

    # Read position angle
    if words[7]:
        catalogue[i]['axis_pa'] = float(words[7])

    # Read Hubble type
    if words[14]:
        catalogue[i]['hubble_type'] = words[14]

    # Read popular name for object
    if words[23]:
        for name in words[23].split(","):
            target = catalogue[i]['names']
            already_got_name = False
            for item in target:
                if item.lower() == name.lower():
                    already_got_name = True
            if not already_got_name:
                target.append(name)

# Import data from catalogue of open clusters
for line in open("../openClusters/clusters.txt"):
    # Ignore blank lines and comment lines
    if len(line.strip()) == 0:
        continue
    name = line[0:18].strip()

    # Read position of object
    ra = (int(line[18:20])) + 1.0 / 60 * (int(line[21:23])) + 1.0 / 3600 * (int(line[24:26]))
    dec = (int(line[29:31])) + 1.0 / 60 * (int(line[32:34])) + 1.0 / 3600 * (int(line[35:37]))
    if line[28] == '-':
        dec *= -1

    # Read distance of object
    dist = line[55:60].strip()
    if dist == "":
        continue
    dist = (int(dist)) / 1.0e6  # Convert pc into Mpc

    # Search existing objects for matching open clusters within 10 arcminutes of reported location
    match = False
    for i in catalogue:
        # Cannot do cross-match without sky position
        if ('ra' not in i) or ('dec' not in i):
            continue

        # Angular distance between this object and item in catalogue
        AngDist = angular_distance(i['ra'], i['dec'], ra, dec)
        if AngDist > 10:
            continue
        if i['type'] != "OC":
            # Possibly object is actually an open cluster, but has the wrong type recorded in NGC catalogue
            if name.endswith(str(i['ngc'])) and i['ngc']:
                logfile.write("Dodgy match between %s with NGC%s\n" % (name, i['ngc']))
                i['type'] = "OC"
            elif name.endswith(str(i['ic'])) and i['ic']:
                logfile.write("Dodgy match between %s with IC%s\n" % (name, i['ic']))
                i['type'] = "OC"

            # ... but if name field doesn't match NGC number, reject this match
            else:
                logfile.write("Rejecting match of %s with M%s NGC%s IC%s\n" % (name, i['m'], i['ngc'], i['ic']))
                continue
        logfile.write(
            "%s matched to M%s NGC%s IC%s (distance %s arcmin)\n" % (name, i['m'], i['ngc'], i['ic'], AngDist))
        i['dist'] = dist
        i['source_dist'] = 'open_clusters'
        match = True
        break
    if not match:
        logfile.write("No match found for %s\n" % name)

# Import data from catalogue of globular clusters
for line in open("../globularClusters/catalogue.dat"):
    # Ignore blank lines and comment lines
    if len(line.strip()) == 0:
        continue
    if line[0] == '#':
        continue
    name = line[1:23].strip()

    # Read position of object
    ra = (int(line[23:25])) + 1.0 / 60 * (int(line[26:28])) + 1.0 / 3600 * (float(line[30:33]))
    dec = (int(line[36:38])) + 1.0 / 60 * (int(line[39:41])) + 1.0 / 3600 * (int(line[42:44]))
    if line[35] == '-':
        dec *= -1

    # Read distance of object
    dist = line[61:67].strip()
    if dist == "":
        continue
    dist = (float(dist)) / 1.0e3  # Convert kpc into Mpc

    # Search existing objects for matching globular clusters within 20 arcminutes of reported location
    match = False
    for i in catalogue:
        # Cannot do cross-match without sky position
        if ('ra' not in i) or ('dec' not in i):
            continue

        # Angular distance between this object and item in catalogue
        AngDist = angular_distance(i['ra'], i['dec'], ra, dec)
        if AngDist > 20:
            continue
        if i['type'] != "Gb":
            # Possibly object is actually an open cluster, but has the wrong type recorded in NGC catalogue
            if name.endswith(str(i['ngc'])) and i['ngc']:
                logfile.write("Dodgy match between %s with NGC%s\n" % (name, i['ngc']))
                i['type'] = "Gb"
            elif name.endswith(str(i['ic'])) and i['ic']:
                logfile.write("Dodgy match between %s with IC%s\n" % (name, i['ic']))
                i['type'] = "Gb"

            # ... but if name field doesn't match NGC number, reject this match
            else:
                logfile.write("Rejecting match of %s with M%s NGC%s IC%s\n" % (name, i['m'], i['ngc'], i['ic']))
                continue
        logfile.write(
            "%s matched to M%s NGC%s IC%s (distance %s arcmin)\n" % (name, i['m'], i['ngc'], i['ic'], AngDist))
        i['dist'] = dist
        i['source_dist'] = 'globular_clusters'
        match = True
        break
    if not match:
        logfile.write("No match found for %s\n" % name)

# Clean output
for i in catalogue:
    # Clean up various kinds of spurious objects in NGC catalogue, e.g. plate defects, into a common type
    if "type" not in i:
        i["type"] = "-"
    if i["type"] == "--":
        i["type"] = "-"
    if i["type"] == "?":
        i["type"] = "-"
    if i["type"] == "*?":
        i["type"] = "-"

    # Convert list of names into a JSON list
    if "names" in i:
        names = i["names"]
    else:
        names = []
    i["names"] = json.dumps(names)

    # Populate a field with the primary name for each object: a Messier designation if present, otherwise NGC/IC number
    name = "-"
    if i["m"]:
        name = "M%d" % i["m"]
    elif i["ngc"]:
        name = "NGC%d" % i["ngc"]
    elif i["ic"]:
        name = "IC%d" % i["ic"]
    i["name"] = name

# Add Sun to catalogue
# catalogue.append(
#   {"name":"Sun","names":json.dumps(["Sun"]),
#    "ra":0,"dec":0,"mag":-26,"type":"sun","dist":0,"z":"0 0","m":0,"ngc":0,"ic":0}
# )

# Add SgrA to catalogue
sgr_a = copy.deepcopy(catalogue_stub_entry)
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
catalogue.append(sgr_a)

# Write output as an enormous JSON file
f = open("output/ngc_merged.dat", "w")
s = json.dumps(catalogue)
f.write(s)
f.close()
