#!/usr/bin/python3
# -*- coding: utf-8 -*-
# catalogue_merge.py
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

"""
Take the Bright Star, Hipparcos, Tycho Catalogues, Gaia DR1 and Gaia DR2, and merge them with all the catalogue IDs
listed for each star.
"""

import os
import re
import json
from math import pi, floor, cos, hypot, atan2
import gzip
import glob

# Make sure output directory exists
os.system("mkdir -p output")

# Constants used for units conversions
AU = 1.49598e11  # metres
LYR = 9.4605284e15  # lightyear in metres

# Reject parallaxes with more than a 30% error
maximum_allowed_parallax_error = 0.3

# Convert Greek letters (i.e. Bayer designations of stars) into TeX
greekAlphabetTeX = {
    # Abbreviations used in Bright Star catalog
    'Alp': r'\alpha', 'Bet': r'\beta', 'Gam': r'\gamma', 'Del': r'\delta', 'Eps': r'\epsilon',
    'Zet': r'\zeta', 'Eta': r'\eta', 'The': r'\theta', 'Iot': r'\iota', 'Kap': r'\kappa',
    'Lam': r'\lambda', 'Mu': r'\mu', 'Nu': r'\nu', 'Xi': r'\xi', 'Omi': 'O', 'Pi': r'\pi',
    'Rho': r'\rho', 'Sig': r'\sigma', 'Tau': r'\tau', 'Ups': r'\upsilon', 'Phi': r'\phi',
    'Chi': r'\chi', 'Psi': r'\psi', 'Ome': r'\omega',

    # Abbreviations used in cross index catalog
    'alf': r'\alpha', 'bet': r'\beta', 'gam': r'\gamma', 'del': r'\delta', 'eps': r'\epsilon',
    'zet': r'\zeta', 'eta': r'\eta', 'the': r'\theta', 'iot': r'\iota', 'kap': r'\kappa',
    'lam': r'\lambda', 'mu.': r'\mu', 'nu.': r'\nu', 'ksi': r'\xi', 'omi': 'O', 'pi.': r'\pi',
    'rho': r'\rho', 'sig': r'\sigma', 'tau': r'\tau', 'ups': r'\upsilon', 'phi': r'\phi',
    'chi': r'\chi', 'psi': r'\psi', 'ome': r'\omega'
}

# Convert Greek letters (i.e. Bayer designations of stars) into HTML
greekAlphabetHTML = [[r"\\_", "_"], [r"\^1", "&#x00B9;"], [r"\^2", "&#x00B2;"], [r"\^3", "&#x00B3;"],
                     [r"\^4", "&#x2074;"], [r"\^5", "&#x2075;"], [r"\^6", "&#x2076;"], [r"\^7", "&#x2077;"],
                     [r"\\alpha", "&alpha;"], [r"\\beta", "&beta;"], [r"\\gamma", "&gamma;"], [r"\\delta", "&delta;"],
                     [r"\\epsilon", "&epsilon;"], [r"\\zeta", "&zeta;"], [r"\\eta", "&eta;"], [r"\\theta", "&theta;"],
                     [r"\\iota", "&iota;"], [r"\\kappa", "&kappa;"], [r"\\lambda", "&lambda;"], [r"\\mu", "&mu;"],
                     [r"\\nu", "&nu;"], [r"\\xi", "&xi;"], [r"\\pi", "&pi;"], [r"\\rho", "&rho;"],
                     [r"\\sigma", "&sigma;"], [r"\\tau", "&tau;"], [r"\\upsilon", "&upsilon;"], [r"\\phi", "&phi;"],
                     [r"\\chi", "&chi;"], [r"\\psi", "&psi;"], [r"\\omega", "&omega;"], ["\$", ""]]

# Convert Greek letters (i.e. Bayer designations of stars) into ASCII
greekAlphabetASCII = [[r"\\_", "_"], [r"\^1", "1"], [r"\^2", "2"], [r"\^3", "3"], [r"\^4", "4"],
                      [r"\^5", "5"], [r"\^6", "6"], [r"\^7", "7"], [r"\\alpha", "Alpha"],
                      [r"\\beta", "Beta"], [r"\\gamma", "Gamma"], [r"\\delta", "Delta"], [r"\\epsilon", "Epsilon"],
                      [r"\\zeta", "Zeta"], [r"\\eta", "Eta"], [r"\\theta", "Theta"], [r"\\iota", "Iota"],
                      [r"\\kappa", "Kappa"], [r"\\lambda", "Lambda"], [r"\\mu", "Mu"], [r"\\nu", "Nu"],
                      [r"\\xi", "Xi"], [r"\\pi", "Pi"], [r"\\rho", "Rho"], [r"\\sigma", "Sigma"], [r"\\tau", "Tau"],
                      [r"\\upsilon", "Upsilon"], [r"\\phi", "Phi"], [r"\\chi", "Chi"], [r"\\psi", "Psi"],
                      [r"\\omega", "Omega"], ["\$", ""],
                      ["^O$", "Omicron"], ["^O1$", "Omicron1"], ["^O2$", "Omicron2"]]

# Convert Greek letters from HTML entities into UTF8 characters
greekHTMLtoUTF8 = [[r"&#x00B9;", "\u00B9"], [r"&#x00B2;", "\u00B2"], [r"&#x00B3;", "\u00B3"], [r"&#x2074;", "\u2074"],
                   [r"&#x2075;", "\u2075"], [r"&#x2076;", "\u2076"], [r"&#x2077;", "\u2077"],
                   [r"&alpha;", "\u03B1"], [r"&beta;", "\u03B2"], [r"&gamma;", "\u03B3"], [r"&delta;", "\u03B4"],
                   [r"&epsilon;", "\u03B5"], [r"&zeta;", "\u03B6"], [r"&eta;", "\u03B7"], [r"&theta;", "\u03B8"],
                   [r"&iota;", "\u03B9"], [r"&kappa;", "\u03BA"], [r"&lambda;", "\u03BB"], [r"&mu;", "\u03BC"],
                   [r"&nu;", "\u03BD"], [r"&xi;", "\u03BE"], [r"&pi;", "\u03C0"], [r"&rho;", "\u03C1"],
                   [r"&sigma;", "\u03C3"], [r"&tau;", "\u03C4"], [r"&upsilon;", "\u03C5"], [r"&phi;", "\u03C6"],
                   [r"&chi;", "\u03C7"], [r"&psi;", "\u03C8"], [r"&omega;", "\u03C9"]]


def greek_html_to_utf8(instr):
    """
    Convert HTML entities for Greek letters into UTF8 characters

    :param instr:
        The name of an object, with Greek letters encoded as HTML entities
    :return:
        UTF8 encoded string
    """
    for letter in greekHTMLtoUTF8:
        instr = re.sub(letter[0], letter[1], instr)
    return instr.split("-")[0]


def name_to_html(instr):
    """
    Convert the name of an object into HTML, by substituting any greek letters with HTML entities.

    :param instr:
        Name of object as reported in catalogue
    :return:
        HTML name of object
    """

    if instr == "":
        return ""
    for subst in greekAlphabetHTML:
        instr = re.sub(subst[0], subst[1], instr)
    return instr


def name_to_ascii(instr):
    """
    Convert the name of an object into ASCII, by substituting any abbreviated greek letters with full names.

    :param instr:
        Name of object as reported in catalogue
    :return:
        ASCII name of object
    """

    if instr == "":
        return ""
    for subst in greekAlphabetASCII:
        instr = re.sub(subst[0], subst[1], instr)
    return instr


def check_positions_agree(ra_old, dec_old, ra_new, dec_new, mag, uid, cat_name_old, cat_name_new, threshold):
    """
    Check that a new position for an object on the sky roughly matches its previously reported position.
    :param ra_old:
        Previous RA, degrees
    :param dec_old:
        Previous declination, degrees
    :param ra_new:
        New RA, degrees
    :param dec_new:
        New declination, degrees
    :param mag:
        Magnitude of object
    :param uid:
        Index of object in the dictionary <star_data>
    :param cat_name_old:
        Catalogue from which the old position was taken
    :param cat_name_new:
        Catalogue from which the new position was taken
    :param threshold:
        Threshold amount we're allowed to move star's position without a warning (degrees)
    :return:
        None
    """
    ra_diff = ra_new - ra_old
    dec_diff = dec_new - dec_old
    ang_change = hypot(dec_diff, ra_diff * cos(ra_new * pi / 180))
    if ang_change > threshold:
        print("Warning: moving mag {:4.1f} star {} by {:.3f} deg ({} --> {})"
            .format(mag, uid, ang_change, cat_name_old, cat_name_new)
              )


# We assign stars incremental UIDs. This UID is the key used in all the dictionaries below
star_counter = 1

# Big dictionary of information about each star, indexed by UID
star_data = {}

star_names_english = {}  # List of English names for each star, indexed by UID
star_names_bayer_letter = {}  # Bayer letter, stored in TeX format for legacy reasons, possibly with superscript number
star_names_const = {}  # Flamsteed/Bayer three-letter constellation abbreviations, e.g. "Peg"
star_names_flamsteed_number = {}  # Flamsteed numbers, stored as integers
star_names_hd_num = {}  # Integer
star_names_bs_num = {}  # Integer
star_names_nsv_num = {}  # Integer
star_names_hip_num = {}  # Integer
star_names_tyc_num = {}  # String, e.g. "1-2-3"
star_names_dr2_num = {}  # String

# Look up the UID of a star by its HD catalogue number, bright star number, HIP number, etc
lookup_by_hd_num = {}
lookup_by_bs_num = {}
lookup_by_hip_num = {}
lookup_by_tyc_num = {}
lookup_by_dr2_num = {}

# Read stars from Bright Star Catalogue
print("Reading data from Yale Bright Star Catalogue...")
source = "bright_stars"
rejection_count = 0
total_count = 0
for line in gzip.open("../brightStars/catalog.gz", "rt"):
    total_count += 1
    bs_number = 0
    hd_number = 0
    names = []
    try:
        bs_number = int(line[0:4])
        RAhrs = float(line[75:77])
        RAmins = float(line[77:79])
        RAsecs = float(line[79:83])
        DecDeg = float(line[84:86])
        DecMins = float(line[86:88])
        DecSecs = float(line[88:90])
        DecSign = line[83]
        mag = float(line[102:107])
        variability = line[51:60].strip().split('/')[0]
        if variability and (variability[-1] == '?'):
            variability = ""  # Clear variability flag if uncertain
        RA = (RAhrs + RAmins / 60 + RAsecs / 3600)  # hours, J2000
        Dec = (DecDeg + DecMins / 60 + DecSecs / 3600)  # degrees, J2000
        if DecSign == '-':
            Dec *= -1
    except ValueError:
        print("Rejecting HR {:d}".format(bs_number))
        rejection_count += 1
        continue

    try:
        hd_number = int(line[25:31])
    except ValueError:
        pass

    # Flamsteed number of star. Use -1 if there is no Flamsteed number.
    flamsteed_number = -1
    try:
        flamsteed_number = int(line[4:7])
    except ValueError:
        pass

    bayer_letter = line[7:10].strip()  # Bayer letter of star, e.g. "Alp"
    bayer_const = line[11:14].strip()  # Constellation abbreviation, e.g. "Peg"
    bayer_letter_num = line[10]  # Number which goes after Bayer letter, for example Alpha-2

    if bayer_letter in greekAlphabetTeX:
        greek_letter = greekAlphabetTeX[bayer_letter]
        if bayer_letter_num in '123456789':
            greek_letter += "^{}".format(bayer_letter_num)
        star_names_bayer_letter[star_counter] = greek_letter
        star_names_const[star_counter] = bayer_const
    if flamsteed_number > 0:
        star_names_flamsteed_number[star_counter] = flamsteed_number

    lookup_by_bs_num[bs_number] = star_counter
    star_names_bs_num[star_counter] = bs_number
    if hd_number:
        lookup_by_hd_num[hd_number] = star_counter
        star_names_hd_num[star_counter] = hd_number

    # Variable star ID may be an NSV number, a designation e.g. PV Cep, or a Bayer letter (which we ignore)
    if variability.isdigit():
        star_names_nsv_num[star_counter] = int(variability)
    elif (variability and
          (re.sub(' ', '', variability) != re.sub(' ', '', line[7:14].strip())) and
          (variability.lower() != 'var')):
        if star_counter not in star_names_english:
            star_names_english[star_counter] = []
        star_names_english[star_counter].append(variability)

    star_data[star_counter] = {'RA': RA * 180 / 12,  # degrees, J2000
                               'Decl': Dec,  # degrees, J2000
                               'source_pos': source,
                               'mag': {'V': {'value': mag, 'source': source}},
                               'is_variable': bool(variability),
                               'in_ybsc': 1}
    star_counter += 1
print("  Total of {:d} stars.".format(star_counter - 1))
print("  Rejected {:d} / {:d} catalogue entries.".format(rejection_count, total_count))

# Read data from Hipparcos catalogue
print("Reading Hipparcos catalogue...")
source = "hipparcos"
rejection_count = 0
total_count = 0
for line in gzip.open("../tycho1/hip_main.dat.gz", "rt"):
    total_count += 1
    hipparcos_num = 0
    hd_number = 0
    parallax = parallax_error = mag_bv = proper_motion = proper_motion_pa = 0

    try:
        hipparcos_num = int(line[2:14])
        mag = float(line[41:46])
        RA = float(line[51:63])  # degrees, J2000 (but proper motion for epoch 1991.25)
        Dec = float(line[64:76])  # degrees, J2000 (but proper motion for epoch 1991.25)
    except ValueError:
        # print "Rejecting HIP {:d}".format(hipparcos_num)
        rejection_count += 1
        continue

    try:
        mag_bt = float(line[217:223])
    except ValueError:
        mag_bt = None

    try:
        mag_vt = float(line[230:236])
    except ValueError:
        mag_vt = None

    try:
        mag_bv = float(line[245:251])
    except ValueError:
        mag_bv = None

    try:
        hd_number = int(line[390:396])
    except ValueError:
        pass

    try:
        parallax = float(line[79:86])
        parallax_error = float(line[119:125])
    except ValueError:
        pass

    try:
        pm_ra = float(line[87:95])  # Proper motion mu_alpha.cos(delta), ICRS
        pm_dec = float(line[96:104])
        proper_motion = hypot(pm_ra, pm_dec)
        proper_motion_pa = (atan2(pm_ra, pm_dec) * 180 / pi + 720) % 360
    except ValueError:
        pass

    # See if we can match this star to a known HD entry. Do not match if HD entry is already matched to another star
    uid = None
    if hd_number in lookup_by_hd_num:
        uid = lookup_by_hd_num[hd_number]
        if (uid in star_names_hip_num) and (star_names_hip_num[uid] != hipparcos_num):
            uid = None
            hd_number = 0

    # If matching unsuccessful, create a new entry
    if uid is None:
        uid = star_counter
        star_data[uid] = {'mag': {}}
        star_counter += 1

    star_data[uid]['in_hipp'] = 1
    star_data[uid]['RA'] = RA  # degrees, J2000
    star_data[uid]['Decl'] = Dec  # degrees, J2000
    star_data[uid]['source_pos'] = source
    star_data[uid]['mag']['V'] = {'value': mag, 'source': source}
    if mag_bt is not None:
        star_data[uid]['mag']['BT'] = {'value': mag_bt, 'source': source}
    if mag_vt is not None:
        star_data[uid]['mag']['VT'] = {'value': mag_vt, 'source': source}
    if mag_bv is not None:
        star_data[uid]['color_bv'] = mag_bv
    if parallax and (parallax_error < parallax * maximum_allowed_parallax_error):
        star_data[uid]['parallax'] = parallax
        star_data[uid]['source_par'] = source
    if proper_motion:
        star_data[uid]['proper_motion'] = proper_motion
        star_data[uid]['proper_motion_pa'] = proper_motion_pa

    if hd_number:
        star_names_hd_num[uid] = hd_number
        lookup_by_hd_num[hd_number] = uid
    if hipparcos_num:
        lookup_by_hip_num[hipparcos_num] = uid
        star_names_hip_num[uid] = hipparcos_num

# Read data from Tycho 1 catalogue (this contains Henry Draper numbers, missing from Tycho 2)
print("  Total of {:d} stars.".format(star_counter - 1))
print("  Rejected {:d} / {:d} catalogue entries.".format(rejection_count, total_count))
print("Reading Tycho 1 catalogue...")
source = "tycho1"
rejection_count = 0
total_count = 0
for line in open("../tycho1/tyc_main.dat"):
    total_count += 1
    tycho_id = ""
    parallax = parallax_error = mag_bv = proper_motion = proper_motion_pa = 0
    hd_number = 0
    hipparcos_num = 0
    try:
        tycho_id = "{:d}-{:d}-{:d}".format(int(line[2:6]), int(line[6:12]), int(line[13]))
        RA = float(line[51:63])  # degrees, J2000 (but proper motion for epoch 1991.25)
        Dec = float(line[64:76])  # degrees, J2000 (but proper motion for epoch 1991.25)
        mag = float(line[41:46])
        mag_bv = float(line[245:251])
    except ValueError:
        # print "Rejecting Tycho ID %s" % tycho_id
        rejection_count += 1
        continue
    try:
        parallax = float(line[79:86])
        parallax_error = float(line[119:125])
    except ValueError:
        pass

    try:
        pm_ra = float(line[87:95])  # Proper motion mu_alpha.cos(delta), ICRS
        pm_dec = float(line[96:104])
        proper_motion = hypot(pm_ra, pm_dec)
        proper_motion_pa = (atan2(pm_ra, pm_dec) * 180 / pi + 720) % 360
    except ValueError:
        pass

    try:
        hd_number = int(line[309:315])
    except ValueError:
        pass

    try:
        hipparcos_num = int(line[210:216])
    except ValueError:
        pass

    # See if we can match this star to a known HD entry. Do not match if HD entry is already matched to another star
    uid = None
    if hd_number in lookup_by_hd_num:
        uid = lookup_by_hd_num[hd_number]
        if ((tycho_id and (uid in star_names_tyc_num) and (star_names_tyc_num[uid] != tycho_id)) or
                (hipparcos_num and (uid in star_names_hip_num) and (star_names_hip_num[uid] != hipparcos_num))):
            uid = None
            hd_number = 0

    # See if we can match this star to a known HIP entry. Do not match if HD entry is already matched to another star
    if uid is None:
        if hipparcos_num in lookup_by_hip_num:
            uid = lookup_by_hip_num[hipparcos_num]
            if tycho_id and (uid in star_names_tyc_num) and (star_names_tyc_num[uid] != tycho_id):
                uid = None
                hipparcos_num = 0

    # If matching unsuccessful, create a new entry
    if uid is None:
        uid = star_counter
        star_data[uid] = {'mag': {}}
        star_counter += 1

    if 'RA' in star_data[uid]:
        check_positions_agree(ra_old=star_data[uid]['RA'], dec_old=star_data[uid]['Decl'],
                              ra_new=RA, dec_new=Dec, mag=mag,
                              uid="TYC {}".format(tycho_id),
                              cat_name_old=star_data[uid]['source_pos'], cat_name_new=source, threshold=0.1)

    if tycho_id:
        star_names_tyc_num[uid] = tycho_id
        lookup_by_tyc_num[tycho_id] = uid
    if hipparcos_num:
        star_names_hip_num[uid] = hipparcos_num
        lookup_by_hip_num[hipparcos_num] = uid
    if hd_number:
        star_names_hd_num[uid] = hd_number
        lookup_by_hd_num[hd_number] = uid
    star_data[uid]['in_tycho1'] = 1
    star_data[uid]['RA'] = RA
    star_data[uid]['Decl'] = Dec
    star_data[uid]['source_pos'] = source
    star_data[uid]['mag']['V'] = {'value': mag, 'source': source}
    star_data[uid]['color_bv'] = mag_bv
    if parallax and (parallax_error < parallax * maximum_allowed_parallax_error):
        star_data[uid]['parallax'] = parallax
        star_data[uid]['source_par'] = source
    if proper_motion:
        star_data[uid]['proper_motion'] = proper_motion
        star_data[uid]['proper_motion_pa'] = proper_motion_pa

# Read data from Tycho 2 catalogue
print("  Total of {:d} stars.".format(star_counter - 1))
print("  Rejected {:d} / {:d} catalogue entries.".format(rejection_count, total_count))
print("Reading Tycho 2 catalogue...")
source = "tycho2"
rejection_count = 0
total_count = 0
for filename in glob.glob("../tycho2/tyc2.dat.*.gz"):
    for line in gzip.open(filename, "rt"):
        total_count += 1
        tycho_id = ""
        hipparcos_num = 0
        try:
            tycho_id = "{:d}-{:d}-{:d}".format(int(line[0:4]), int(line[5:10]), int(line[11]))
            RA = float(line[15:27])  # degrees, J2000 (but proper motion for epoch 1991.25)
            Dec = float(line[28:40])  # degrees, J2000 (but proper motion for epoch 1991.25)
            mag_bt = float(line[110:116])
            mag_vt = float(line[123:129])
            mag = mag_vt - 0.090 * (
                    mag_bt - mag_vt)  # Johnson V-band; see http://heasarc.nasa.gov/W3Browse/all/tycho2.html
            mag_bv = 0.850 * (mag_bt - mag_vt)  # B-V colour
        except ValueError:
            # print "Rejecting Tycho ID %s" % tycho_id
            rejection_count += 1
            continue
        try:
            hipparcos_num = int(line[142:148])
        except ValueError:
            pass

        # See if we can match this star to a known Tycho entry.
        uid = None
        if tycho_id in lookup_by_tyc_num:
            uid = lookup_by_tyc_num[tycho_id]

        # See if we can match this star to a known HIP entry.
        # Do not match if HD entry is already matched to another star
        if uid is None:
            if hipparcos_num in lookup_by_hip_num:
                uid = lookup_by_hip_num[hipparcos_num]
                if tycho_id and (uid in star_names_tyc_num) and (star_names_tyc_num[uid] != tycho_id):
                    uid = None
                    hipparcos_num = 0

        # If matching unsuccessful, create a new entry
        if uid is None:
            uid = star_counter
            star_data[uid] = {'mag': {}}
            star_counter += 1

        if 'RA' in star_data[uid]:
            check_positions_agree(ra_old=star_data[uid]['RA'], dec_old=star_data[uid]['Decl'],
                                  ra_new=RA, dec_new=Dec, mag=mag,
                                  uid="TYC {}".format(tycho_id),
                                  cat_name_old=star_data[uid]['source_pos'], cat_name_new=source, threshold=0.01)

        if tycho_id:
            star_names_tyc_num[uid] = tycho_id
            lookup_by_tyc_num[tycho_id] = uid
        if hipparcos_num:
            star_names_hip_num[uid] = hipparcos_num
            lookup_by_hip_num[hipparcos_num] = uid
        star_data[uid]['in_tycho2'] = 1
        star_data[uid]['RA'] = RA
        star_data[uid]['Decl'] = Dec
        star_data[uid]['source_pos'] = source
        star_data[uid]['mag']['V'] = {'value': mag, 'source': source}
        star_data[uid]['mag']['BT'] = {'value': mag_bt, 'source': source}
        star_data[uid]['mag']['VT'] = {'value': mag_vt, 'source': source}
        star_data[uid]['color_bv'] = mag_bv

# Read data from Hipparcos new reduction
print("  Total of {:d} stars.".format(star_counter - 1))
print("  Rejected {:d} / {:d} catalogue entries.".format(rejection_count, total_count))
print("Reading Hipparcos new reduction catalogue...")
source = "hipparcos_new"
rejection_count = 0
total_count = 0
new_entry_count = 0
for line in gzip.open("../hipparcosNewReduction/hip2.dat.gz", "rt"):
    total_count += 1
    parallax = parallax_error = mag_bv = proper_motion = proper_motion_pa = 0

    try:
        hipparcos_num = int(line[0:6])
    except ValueError:
        hipparcos_num = 0

    try:
        parallax = float(line[43:50])
        parallax_error = float(line[83:89])
    except ValueError:
        pass

    try:
        pm_ra = float(line[51:59])  # Proper motion mu_alpha.cos(delta), ICRS
        pm_dec = float(line[60:68])
        proper_motion = hypot(pm_ra, pm_dec)
        proper_motion_pa = (atan2(pm_ra, pm_dec) * 180 / pi + 720) % 360
    except ValueError:
        pass

    if hipparcos_num in lookup_by_hip_num:
        uid = lookup_by_hip_num[hipparcos_num]
    else:
        rejection_count += 1
        continue

    if parallax and (parallax_error < parallax * maximum_allowed_parallax_error):
        star_data[uid]['parallax'] = parallax
        star_data[uid]['source_par'] = source
    if proper_motion:
        star_data[uid]['proper_motion'] = proper_motion
        star_data[uid]['proper_motion_pa'] = proper_motion_pa

# Read data from Gaia DR1
print("  Total of {:d} stars.".format(star_counter - 1))
print("  Rejected {:d} / {:d} catalogue entries.".format(rejection_count, total_count))
print("Reading Gaia DR1 catalogue...")
source = "gaia_dr1"
rejection_count = 0
total_count = 0
new_entry_count = 0
for line in gzip.open("../gaiaDR1/tgas.dat.gz", "rt"):
    total_count += 1

    try:
        tycho_id = line[7:19].strip()
    except ValueError:
        tycho_id = ""

    try:
        hipparcos_num = int(line[0:6])
    except ValueError:
        hipparcos_num = 0

    if (tycho_id == "") and (hipparcos_num < 1):
        print("Rejecting Gaia DR1 star: could not read either HIP or TYC identifier.")
        rejection_count += 1
        continue

    try:
        # epoch = float(line[68:74])  # Equals 2015.0
        RA = float(line[75:89])  # degrees, J2000 (but proper motion for epoch 2015.0)
        Dec = float(line[97:111])  # degrees, J2000 (but proper motion for epoch 2015.0)
        mag = float(line[433:439])
    except ValueError:
        # print("Rejecting Gaia DR1 star {} / {}".format(tycho_id, hipparcos_num))
        rejection_count += 1
        continue

    try:
        parallax = float(line[119:125])
        parallax_error = float(line[126:130])
    except ValueError:
        parallax = parallax_error = 0

    try:
        pm_ra = float(line[131:140])
        pm_dec = float(line[148:157])
        proper_motion = hypot(pm_ra, pm_dec)
        proper_motion_pa = (atan2(pm_ra, pm_dec) * 180 / pi + 720) % 360
    except ValueError:
        proper_motion = proper_motion_pa = 0

    # See if we can match this star to a known Tycho entry.
    uid = None
    if tycho_id in lookup_by_tyc_num:
        uid = lookup_by_tyc_num[tycho_id]

    # See if we can match this star to a known HIP entry.
    # Do not match if HIP entry is already matched to another star
    if uid is None:
        if hipparcos_num in lookup_by_hip_num:
            uid = lookup_by_hip_num[hipparcos_num]
            if tycho_id and ((uid in star_names_tyc_num) and (star_names_tyc_num[uid] != tycho_id)):
                uid = None
                hipparcos_num = 0

    # If matching unsuccessful, create a new entry
    if uid is None:
        uid = star_counter
        star_data[uid] = {'mag': {}}
        star_counter += 1
        new_entry_count += 1
        if tycho_id:
            star_names_tyc_num[uid] = tycho_id
            lookup_by_tyc_num[tycho_id] = uid
            # print("Creating new entry ({:d}) for Gaia star TYC {}".format(uid, tycho_id))
        if hipparcos_num > 0:
            star_names_hip_num[uid] = hipparcos_num
            lookup_by_hip_num[hipparcos_num] = uid
            # print("Creating new entry ({:d}) for Gaia star HIP {}".format(uid, hipparcos_num))

    if 'RA' in star_data[uid]:
        star_name = ("HIP {:d}".format(hipparcos_num)) if hipparcos_num else ("TYC {}".format(tycho_id))
        check_positions_agree(ra_old=star_data[uid]['RA'], dec_old=star_data[uid]['Decl'],
                              ra_new=RA, dec_new=Dec, mag=mag,
                              uid=star_name,
                              cat_name_old=star_data[uid]['source_pos'], cat_name_new=source, threshold=0.01)

    if tycho_id:
        star_names_tyc_num[uid] = tycho_id
        lookup_by_tyc_num[tycho_id] = uid
    if hipparcos_num:
        star_names_hip_num[uid] = hipparcos_num
        lookup_by_hip_num[hipparcos_num] = uid
    star_data[uid]['in_gaiadr1'] = 1
    star_data[uid]['RA'] = RA  # degrees, J2000
    star_data[uid]['Decl'] = Dec  # degrees, J2000
    star_data[uid]['source_pos'] = source
    if parallax and (parallax_error < parallax * maximum_allowed_parallax_error):
        star_data[uid]['parallax'] = parallax
        star_data[uid]['source_par'] = source
    if proper_motion:
        star_data[uid]['proper_motion'] = proper_motion
        star_data[uid]['proper_motion_pa'] = proper_motion_pa

    # Gaia G-band magnitudes are subject to errors
    # star_data[uid]['mag']['G'] = {'value': mag, 'source': source}

# Read data from Gaia DR2
print("  Total of {:d} stars.".format(star_counter - 1))
print("  Rejected {:d} / {:d} catalogue entries.".format(rejection_count, total_count))
print("Reading Gaia DR2 catalogue...")
source = "gaia_dr2"
rejection_count = 0
total_count = 0
new_entry_count = 0
for filename in glob.glob("../gaiaDR2/*.csv.gz"):
    columns = []
    for line in gzip.open(filename, "rt"):
        # First line contains column headings
        if not columns:
            columns = line.strip().split(",")
            continue
        # Read data lines
        total_count += 1
        words = line.strip().split(",")
        RA = float(words[columns.index("ra")])
        Dec = float(words[columns.index("dec")])
        dr2_id = words[columns.index("source_id")]
        tycho_id = words[columns.index("tycho")]

        try:
            hipparcos_num = int(words[columns.index("hipparcos")])
        except ValueError:
            hipparcos_num = 0

        try:
            parallax = float(words[columns.index("parallax")])
            parallax_error = float(words[columns.index("parallax_error")])
        except ValueError:
            parallax = parallax_error = 0

        try:
            pm_ra = float(words[columns.index("pmra")])
            pm_dec = float(words[columns.index("pmdec")])
            proper_motion = hypot(pm_ra, pm_dec)
            proper_motion_pa = (atan2(pm_ra, pm_dec) * 180 / pi + 720) % 360
        except ValueError:
            proper_motion = proper_motion_pa = 0

        # if (not tycho_id) and (not hipparcos_num):
        #    print "Rejecting Gaia DR2 star: could not read either HIP or TYC identifier."
        #    rejection_count += 1
        #   continue

        # See if we can match this star to a known Tycho entry.
        uid = None
        if tycho_id in lookup_by_tyc_num:
            uid = lookup_by_tyc_num[tycho_id]

        # See if we can match this star to a known HIP entry.
        # Do not match if HIP entry is already matched to another star
        if hipparcos_num and (uid is None):
            if hipparcos_num in lookup_by_hip_num:
                uid = lookup_by_hip_num[hipparcos_num]
                if tycho_id and ((uid in star_names_tyc_num) and (star_names_tyc_num[uid] != tycho_id)):
                    uid = None
                    hipparcos_num = 0

        # If matching unsuccessful, create a new entry
        if uid is None:
            uid = star_counter
            star_data[uid] = {'mag': {}}
            star_counter += 1
            new_entry_count += 1
            if tycho_id:
                star_names_tyc_num[uid] = tycho_id
                lookup_by_tyc_num[tycho_id] = uid
                print("Creating new entry ({:d}) for Gaia DR2 star TYC {}".format(uid, tycho_id))
            if hipparcos_num:
                star_names_hip_num[uid] = hipparcos_num
                lookup_by_hip_num[hipparcos_num] = uid
                print("Creating new entry ({:d}) for Gaia DR2 star HIP {}".format(uid, hipparcos_num))

        if 'RA' in star_data[uid]:
            star_name = ("HIP {:d}".format(hipparcos_num)) if hipparcos_num else ("TYC {}".format(tycho_id))
            check_positions_agree(ra_old=star_data[uid]['RA'], dec_old=star_data[uid]['Decl'],
                                  ra_new=RA, dec_new=Dec,
                                  mag=float(words[columns.index("phot_g_mean_mag")]),
                                  uid=star_name,
                                  cat_name_old=star_data[uid]['source_pos'], cat_name_new=source, threshold=0.01)
        if dr2_id:
            star_names_dr2_num[uid] = dr2_id
            lookup_by_dr2_num[dr2_id] = uid
        if tycho_id:
            star_names_tyc_num[uid] = tycho_id
            lookup_by_tyc_num[tycho_id] = uid
        if hipparcos_num:
            star_names_hip_num[uid] = hipparcos_num
            lookup_by_hip_num[hipparcos_num] = uid

        star_data[uid]['in_gaiadr2'] = 1
        star_data[uid]['RA'] = RA  # degrees, J2000
        star_data[uid]['Decl'] = Dec  # degrees, J2000
        star_data[uid]['source_pos'] = source
        if parallax and (parallax_error < parallax * maximum_allowed_parallax_error):
            star_data[uid]['parallax'] = parallax
            star_data[uid]['source_par'] = source
        if proper_motion:
            star_data[uid]['proper_motion'] = proper_motion
            star_data[uid]['proper_motion_pa'] = proper_motion_pa

        for band in ['g', 'bp', 'rp']:
            mag = words[columns.index("phot_{}_mean_mag".format(band))]
            if mag:
                star_data[uid]['mag'][band.upper()] = {'value': float(mag), 'source': source}

# Read the Bayer / Flamsteed cross index
print("  Total of {:d} stars.".format(star_counter - 1))
print("  Total of {:d} Bayer letters.".format(len(star_names_bayer_letter)))
print("  Total of {:d} Flamsteed numbers.".format(len(star_names_flamsteed_number)))
print("Reading Cross Index catalogue...")

rejection_count = 0
total_count = 0
for line in open("../bayerAndFlamsteed/catalog.dat"):
    hd_number = 0
    flamsteed_number = 0
    try:
        hd_number = int(line[0:6])
        bayer_letter = line[68:71].strip()  # Bayer letter of star, e.g. "Alp"
        bayer_const = line[74:77].strip()  # Constellation abbreviation, e.g. "Peg"
        bayer_letter_num = line[71:73].strip()  # Number which goes after Bayer letter, for example Alpha-2
        try:
            flamsteed_number = int(line[64:67].strip())
        except ValueError:
            flamsteed_number = 0

        # Some stars are listed as c01 rather than "c  01"
        if (bayer_letter_num == "") and (len(bayer_letter) > 2) and (bayer_letter[-2] == "0"):
            bayer_letter_num = bayer_letter[-2:]
            bayer_letter = bayer_letter[:-2]
    except ValueError:
        print("Rejecting HD {:d}".format(hd_number))
        rejection_count += 1
        continue

    # See if we can match this star to a known HD entry
    uid = None
    if hd_number in lookup_by_hd_num:
        uid = lookup_by_hd_num[hd_number]
    else:
        print("Could not match HD {:d}".format(hd_number))
        rejection_count += 1
        continue

    # Look up existing Bayer letter, if any
    bayer_letter_old = None
    if uid in star_names_bayer_letter:
        bayer_letter_old = star_names_bayer_letter[uid]

    # Look up existing Flamsteed number, if any
    flamsteed_number_old = None
    if uid in star_names_flamsteed_number:
        flamsteed_number = star_names_flamsteed_number[uid]

    if bayer_letter:
        if bayer_letter in greekAlphabetTeX:
            greek_letter = greekAlphabetTeX[bayer_letter]
        else:
            greek_letter = bayer_letter
        if bayer_letter_num:
            greek_letter += "^{:d}".format(int(bayer_letter_num))
        if bayer_letter_old and (bayer_letter_old != greek_letter):
            print("Warning, changing Bayer designation from <{}> to <{}> {} for HD {:d}"
                  .format(bayer_letter_old, greek_letter, bayer_const, hd_number))

        star_names_bayer_letter[uid] = greek_letter
        star_names_const[uid] = bayer_const

    if flamsteed_number > 0:
        if flamsteed_number_old and (flamsteed_number_old != flamsteed_number):
            print("Warning, changing Flamsteed designation from <{}> to <{}> {}"
                  .format(flamsteed_number_old, flamsteed_number, bayer_const))
        star_names_flamsteed_number[uid] = flamsteed_number
        star_names_const[uid] = bayer_const

print("  Total of {:d} Bayer letters.".format(len(star_names_bayer_letter)))
print("  Total of {:d} Flamsteed numbers.".format(len(star_names_flamsteed_number)))

# Read list of English names for stars
print("  Total of {:d} stars.".format(star_counter - 1))
print("  Rejected {:d} / {:d} catalogue entries.".format(rejection_count, total_count))
print("  Created {:d} new entries.".format(new_entry_count))
print("Reading English names for stars...")
for line in open("../brightStars/starNames_iau.txt"):
    if (len(line) < 5) or (line[0] == '#'):
        continue
    words = line.split()
    cat_name = words[0]
    cat_number = int(words[1])
    parallax = float(words[2])
    name = line[20:]
    uid = None
    if cat_name == "HR":
        if cat_number in lookup_by_bs_num:
            uid = lookup_by_bs_num[cat_number]
    elif cat_name == "HD":
        if cat_number in lookup_by_hd_num:
            uid = lookup_by_hd_num[cat_number]
    elif cat_name == "HIP":
        if cat_number in lookup_by_hip_num:
            uid = lookup_by_hip_num[cat_number]
    if uid is None:
        print("  Error - could not find star {} {}".format(cat_name, cat_number))
        continue
    if uid not in star_names_english:
        star_names_english[uid] = []
    star_names_english[uid].insert(0, name.strip())
    if parallax:
        star_data[uid]['source_par'] = 'manual'
        star_data[uid]['parallax'] = parallax

# Work out distances of stars from parallaxes
print("Working out distances from parallaxes...")
for item in list(star_data.values()):
    if 'parallax' in item:
        parallax = item['parallax']
        if parallax > 0.0000001:
            item['dist'] = AU / (parallax * 1e-3 / 3600 / 180 * pi) / LYR  # Parallax in mas; Distance in LYR
        else:
            del item['parallax']

# Create a histogram of the brightness of all the stars we have inserted into catalogue
print("Creating list of stars in order of brightness...")
star_magnitudes = []
brightness_histogram = {}
for uid, info in list(star_data.items()):
    if 'V' in info['mag']:
        star_magnitudes.append([uid, info['mag']['V']['value']])
        key = floor(info['mag']['V']['value'])
        if key not in brightness_histogram:
            brightness_histogram[key] = 0
        brightness_histogram[key] += 1

print("  Total of {:d} stars with valid magnitudes.".format(len(star_magnitudes)))

# Display a brightness histogram as a table
print("Brightness histogram:")
accumulator = 0
for i in range(-2, 20):
    val = 0
    if i in brightness_histogram:
        val = brightness_histogram[i]
    accumulator += val
    print("mag {:4d} to {:4d} -- {:7d} stars (total {:7d} stars)".format(i, i + 1, val, accumulator))

# Sort stars into order of V-band magnitude before writing output
print("Sorting list of stars in order of brightness")
star_magnitudes.sort(key=lambda x: x[1])

# Produce output catalogues
print("Producing output data catalogues")

# Write output as a text file for use in StarCharter
output_star_charter = open("output/star_charter_stars.dat", "wt")

# Also write output as a big lump of JSON which other python scripts can use
output_json = open("output/all_stars.json", "wt")

# Loop over all the objects we've catalogued
for item in star_magnitudes:
    # item = [uid, V-magnitude]

    # Some variables we feed data into
    parallax = dist = proper_motion = proper_motion_pa = mag_bv = ybsn = hd = Nhip = nsv = variable = 0
    source_pos = source_par = ""
    tycho_id = dr2_id = name_const = name_bayer_letter = name_flamsteed_num = ""
    names_english = []

    # Name1 is the star's Bayer letter, as a UTF8 character
    # Name2 is 'x-c' where x is Name1 and c is the constellation
    # Name3 is the star's English name, which spaces rendered as underscores
    # Name4 is 'y' where y is the Flamsteed number
    Name1 = Name2 = Name3 = Name4 = "-"

    # Populate variables with data from the <star_data> list
    uid = item[0]
    RA = star_data[uid]['RA']
    Decl = star_data[uid]['Decl']
    source_pos = star_data[uid]['source_pos']
    mag_list = star_data[uid]['mag']
    if 'parallax' in star_data[uid]:
        parallax = star_data[uid]['parallax']
        dist = star_data[uid]['dist']
        source_par = star_data[uid]['source_par']
    if 'proper_motion' in star_data[uid]:
        proper_motion = star_data[uid]['proper_motion']
        proper_motion_pa = star_data[uid]['proper_motion_pa']
    if 'color_bv' in star_data[uid]:
        mag_bv = star_data[uid]['color_bv']
    if ('is_variable' in star_data[uid]) and (star_data[uid]['is_variable']):
        variable = 1

    # Catalogue numbers for this star
    if uid in star_names_bs_num:
        ybsn = star_names_bs_num[uid]
    if uid in star_names_hd_num:
        hd = star_names_hd_num[uid]
    if uid in star_names_nsv_num:
        nsv = star_names_nsv_num[uid]
    if uid in star_names_hip_num:
        Nhip = star_names_hip_num[uid]
    if uid in star_names_tyc_num:
        tycho_id = star_names_tyc_num[uid]
    if uid in star_names_dr2_num:
        dr2_id = star_names_dr2_num[uid]

    # Something dodgy has happened if this star doesn't have any catalogue identifications
    if (not ybsn) and (not hd) and (not Nhip) and (not tycho_id) and (not dr2_id):
        print("Warning: Star in database with no valid catalogue IDs")

    # Names for this star
    if uid in star_names_english:
        Name3 = re.sub(' ', '_', star_names_english[uid][0])  # StarCharter using whitespace-sep columns
        names_english = star_names_english[uid]  # JSON output allows spaces in the names of stars
    if uid in star_names_const:
        name_const = star_names_const[uid]
        if uid in star_names_bayer_letter:
            name_bayer_letter = star_names_bayer_letter[uid]
            Name1 = greek_html_to_utf8(name_to_html(name_bayer_letter))
            Name2 = "{}-{}".format(Name1, star_names_const[uid])
        if uid in star_names_flamsteed_number:
            Name4 = "{}".format(star_names_flamsteed_number[uid])
            name_flamsteed_num = star_names_flamsteed_number[uid]

    # Limiting magnitude of 12 for StarCharter
    if ('V' in mag_list) and (mag_list['V']['value'] < 12):
        mag = mag_list['V']['value']
        output_star_charter.write(
            "{:6d} {:6d} {:8d} {:17.12f} {:17.12f} {:17.12f} {:17.12f} {:17.12f} {:s} {:s} {:s} {:s}\n"
            .format(hd, ybsn, Nhip, RA, Decl, mag, parallax, dist, Name1, Name2, Name3, Name4)
        )

    # Dump absolutely everything into the JSON output
    json_structure = [RA, Decl, mag_list, parallax, dist, ybsn, hd, Nhip, tycho_id,
                      names_english, name_const,
                      name_to_html(name_bayer_letter), name_to_ascii(name_bayer_letter),
                      name_flamsteed_num,
                      source_pos, source_par,
                      mag_bv, proper_motion, proper_motion_pa, nsv, variable, dr2_id]
    output_json.write(json.dumps(json_structure))
    output_json.write("\n")

# Finish up and close output files
output_star_charter.close()
output_json.close()
