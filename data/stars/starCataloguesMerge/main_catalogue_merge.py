#!/usr/bin/python3
# -*- coding: utf-8 -*-
# main_catalogue_merge.py
#
# -------------------------------------------------
# Copyright 2015-2022 Dominic Ford
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

import glob
import gzip
import json
import logging
import os
import re
import sys
from math import pi, floor, hypot, atan2
from typing import Optional

import numpy as np

from star_descriptor import StarDescriptor, StarList
from star_name_conversion import StarNameConversion

# Constants used for units conversions
AU = 1.49598e11  # metres
LYR = 9.4605284e15  # lightyear in metres

# Reject parallaxes with more than a 30% error
maximum_allowed_parallax_error = 0.3


def read_from_ybsc(star_data: StarList):
    star_name_converter = StarNameConversion()

    # Read stars from Bright Star Catalogue
    logging.info("Reading data from Yale Bright Star Catalogue...")
    source = "bright_stars"
    rejection_count = 0
    total_count = 0
    for line in gzip.open("../brightStars/catalog.gz", "rt"):
        total_count += 1
        bs_number = None
        hd_number = None
        try:
            bs_number = int(line[0:4])
            ra_hours = float(line[75:77])
            ra_minutes = float(line[77:79])
            ra_seconds = float(line[79:83])
            decl_degrees = float(line[84:86])
            decl_minutes = float(line[86:88])
            decl_seconds = float(line[88:90])
            decl_sign = line[83]
            mag = float(line[102:107])
            variability = line[51:60].strip().split('/')[0]
            if variability and (variability[-1] == '?'):
                variability = ""  # Clear variability flag if uncertain
            ra = (ra_hours + ra_minutes / 60 + ra_seconds / 3600)  # hours, J2000
            decl = (decl_degrees + decl_minutes / 60 + decl_seconds / 3600)  # degrees, J2000
            if decl_sign == '-':
                decl *= -1
        except ValueError:
            logging.info("Rejecting HR {:d}".format(bs_number))
            rejection_count += 1
            continue

        try:
            hd_number = int(line[25:31])
        except ValueError:
            pass

        # Flamsteed number of star
        flamsteed_number = None
        try:
            flamsteed_number = int(line[4:7])
        except ValueError:
            pass

        bayer_letter = line[7:10].strip()  # Bayer letter of star, e.g. "Alp"
        bayer_const = line[11:14].strip()  # Constellation abbreviation, e.g. "Peg"
        bayer_letter_num = line[10]  # Number which goes after Bayer letter, for example Alpha-2

        star = StarDescriptor()  # type: Optional[StarDescriptor]

        if bayer_letter in star_name_converter.greekAlphabetTeX:
            greek_letter = star_name_converter.greekAlphabetTeX[bayer_letter]
            if bayer_letter_num in '123456789':
                greek_letter += "^{}".format(bayer_letter_num)
            star.names_bayer_letter = greek_letter
            star.names_const = bayer_const
        if flamsteed_number is not None:
            star.names_flamsteed_number = flamsteed_number

        star.names_bs_num = bs_number
        if hd_number:
            star.names_hd_num = hd_number

        # Variable star ID may be an NSV number, a designation e.g. PV Cep, or a Bayer letter (which we ignore)
        if variability.isdigit():
            star.names_nsv_num = int(variability)
        elif (variability and
              (re.sub(' ', '', variability) != re.sub(' ', '', line[7:14].strip())) and
              (variability.lower() != 'var')):
            star.add_catalogue_name(new_name=variability)

        # Record this star's position and brightness
        star.ra = ra * 180 / 12
        star.decl = decl
        star.source_pos = source
        star.mag_V = mag
        star.mag_V_source = source
        star.is_variable = bool(variability)
        star.in_ybsc = True

        # Ensure this star is recorded in the output catalogue
        star_data.add_star(star_descriptor=star)

    logging.info("  Total of {:d} stars.".format(len(star_data)))
    logging.info("  Rejected {:d} / {:d} catalogue entries.".format(rejection_count, total_count))


def read_from_hipparcos(star_data: StarList):
    # Read data from Hipparcos catalogue
    logging.info("Reading Hipparcos catalogue...")
    source = "hipparcos"
    rejection_count = 0
    total_count = 0
    for line in gzip.open("../tycho1/hip_main.dat.gz", "rt"):
        total_count += 1
        hipparcos_num = None  # type: Optional[int]
        hd_number = None  # type: Optional[int]
        parallax = None  # type: Optional[float]
        parallax_error = None  # type: Optional[float]
        mag_bv = None  # type: Optional[float]
        proper_motion = None  # type: Optional[float]
        proper_motion_pa = None  # type: Optional[float]

        try:
            hipparcos_num = int(line[2:14])
            mag = float(line[41:46])
            ra = float(line[51:63])  # degrees, J2000 (but proper motion for epoch 1991.25)
            decl = float(line[64:76])  # degrees, J2000 (but proper motion for epoch 1991.25)
        except ValueError:
            # logging.info("Rejecting HIP {:d}".format(hipparcos_num))
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

        # Fetch star descriptor
        star, hd_mismatch, hipparcos_mismatch = star_data.match_star(
            ra=ra, decl=decl, mag=mag,
            source=source, match_threshold=0.1,
            hd_number=hd_number, hipparcos_num=hipparcos_num)

        # If HD / Hipparcos ID matched another star, wipe don't over-write it
        if hd_mismatch:
            hd_number = None
        if hipparcos_mismatch:
            hipparcos_num = None

        # Record this star's position and parallax
        star.in_hipp = True
        star.ra = ra  # degrees, J2000
        star.decl = decl  # degrees, J2000
        star.source_pos = source
        if parallax and (parallax_error < parallax * maximum_allowed_parallax_error):
            star.parallax = parallax
            star.source_par = source
        if proper_motion:
            star.proper_motion = proper_motion
            star.proper_motion_pa = proper_motion_pa

        # Record this star's brightness
        star.mag_V = mag
        star.mag_V_source = source
        if mag_bt is not None:
            star.mag_BT = mag_bt
            star.mag_BT_source = source
        if mag_vt is not None:
            star.mag_VT = mag_vt
            star.mag_VT_source = source
        if mag_bv is not None:
            star.color_bv = mag_bv

        # Record this star's catalogue IDs
        if hd_number:
            star.names_hd_num = hd_number
        if hipparcos_num:
            star.names_hip_num = hipparcos_num

        # Ensure this star is recorded in the output catalogue
        star_data.add_star(star_descriptor=star)

    logging.info("  Total of {:d} stars.".format(len(star_data)))
    logging.info("  Rejected {:d} / {:d} catalogue entries.".format(rejection_count, total_count))


def read_from_tycho_1(star_data: StarList):
    # Read data from Tycho 1 catalogue (this contains Henry Draper numbers, missing from Tycho 2)
    logging.info("Reading Tycho 1 catalogue...")
    source = "tycho1"
    rejection_count = 0
    total_count = 0
    for line in open("../tycho1/tyc_main.dat"):
        total_count += 1
        hipparcos_num = None  # type: Optional[int]
        hd_number = None  # type: Optional[int]
        tycho_id = None  # type: Optional[str]
        parallax = None  # type: Optional[float]
        parallax_error = None  # type: Optional[float]
        mag_bv = None  # type: Optional[float]
        proper_motion = None  # type: Optional[float]
        proper_motion_pa = None  # type: Optional[float]

        try:
            tycho_id = "{:d}-{:d}-{:d}".format(int(line[2:6]), int(line[6:12]), int(line[13]))
            ra = float(line[51:63])  # degrees, J2000 (but proper motion for epoch 1991.25)
            decl = float(line[64:76])  # degrees, J2000 (but proper motion for epoch 1991.25)
            mag = float(line[41:46])
            mag_bv = float(line[245:251])
        except ValueError:
            # logging.info("Rejecting Tycho ID {}".format(tycho_id))
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

        # Fetch star descriptor
        star, hd_mismatch, hipparcos_mismatch = star_data.match_star(
            ra=ra, decl=decl, mag=mag,
            source=source, match_threshold=0.1,
            hd_number=hd_number, hipparcos_num=hipparcos_num, tycho_id=tycho_id)

        # If HD / Hipparcos ID matched another star, wipe don't over-write it
        if hd_mismatch:
            hd_number = None
        if hipparcos_mismatch:
            hipparcos_num = None

        # Record this star's catalogue IDs
        if tycho_id:
            star.names_tyc_num = tycho_id
        if hipparcos_num:
            star.names_hip_num = hipparcos_num
        if hd_number:
            star.names_hd_num = hd_number

        # Record this star's position and parallax
        star.in_tycho1 = True
        star.ra = ra
        star.decl = decl
        star.source_pos = source
        if parallax is not None and (parallax_error < parallax * maximum_allowed_parallax_error):
            star.parallax = parallax
            star.source_par = source
        if proper_motion:
            star.proper_motion = proper_motion
            star.proper_motion_pa = proper_motion_pa

        # Record this star's brightness
        star.mag_V = mag
        star.mag_V_source = source
        star.color_bv = mag_bv
        
        # Ensure this star is recorded in the output catalogue
        star_data.add_star(star_descriptor=star)

    logging.info("  Total of {:d} stars.".format(len(star_data)))
    logging.info("  Rejected {:d} / {:d} catalogue entries.".format(rejection_count, total_count))


def read_from_tycho_2(star_data: StarList):
    # Read data from Tycho 2 catalogue
    logging.info("Reading Tycho 2 catalogue...")
    source = "tycho2"
    rejection_count = 0
    total_count = 0
    for filename in glob.glob("../tycho2/tyc2.dat.*.gz"):
        for line in gzip.open(filename, "rt"):
            total_count += 1
            hipparcos_num = None  # type: Optional[int]
            tycho_id = None  # type: Optional[str]

            try:
                tycho_id = "{:d}-{:d}-{:d}".format(int(line[0:4]), int(line[5:10]), int(line[11]))
                ra = float(line[15:27])  # degrees, J2000 (but proper motion for epoch 1991.25)
                decl = float(line[28:40])  # degrees, J2000 (but proper motion for epoch 1991.25)
                mag_bt = float(line[110:116])
                mag_vt = float(line[123:129])
                mag = mag_vt - 0.090 * (
                        mag_bt - mag_vt)  # Johnson V-band; see http://heasarc.nasa.gov/W3Browse/all/tycho2.html
                mag_bv = 0.850 * (mag_bt - mag_vt)  # B-V colour
            except ValueError:
                # logging.info("Rejecting Tycho ID {}".format(tycho_id))
                rejection_count += 1
                continue
            try:
                hipparcos_num = int(line[142:148])
            except ValueError:
                pass

            # Fetch star descriptor
            star, hd_mismatch, hipparcos_mismatch = star_data.match_star(
                ra=ra, decl=decl, mag=mag,
                source=source, match_threshold=0.01,
                hipparcos_num=hipparcos_num, tycho_id=tycho_id)

            # If Hipparcos ID matched another star, wipe don't over-write it
            if hipparcos_mismatch:
                hipparcos_num = None

            # Record this star's catalogue IDs
            if tycho_id:
                star.names_tyc_num = tycho_id
            if hipparcos_num:
                star.names_hip_num = hipparcos_num

            # Record this star's position and parallax
            star.in_tycho2 = True
            star.ra = ra
            star.decl = decl
            star.source_pos = source

            # Record this star's brightness
            star.mag_V = mag
            star.mag_V_source = source
            star.mag_BT = mag_bt
            star.mag_BT_source = source
            star.mag_VT = mag_vt
            star.mag_VT_source = source
            star.color_bv = mag_bv

            # Ensure this star is recorded in the output catalogue
            star_data.add_star(star_descriptor=star)

    logging.info("  Total of {:d} stars.".format(len(star_data)))
    logging.info("  Rejected {:d} / {:d} catalogue entries.".format(rejection_count, total_count))


def read_from_hipparcos_new(star_data: StarList):
    # Read data from Hipparcos new reduction
    logging.info("Reading Hipparcos new reduction catalogue...")
    source = "hipparcos_new"
    rejection_count = 0
    total_count = 0
    new_entry_count = 0
    for line in gzip.open("../hipparcosNewReduction/hip2.dat.gz", "rt"):
        total_count += 1
        hipparcos_num = None  # type: Optional[int]
        parallax = None  # type: Optional[float]
        parallax_error = None  # type: Optional[float]
        mag_bv = None  # type: Optional[float]
        proper_motion = None  # type: Optional[float]
        proper_motion_pa = None  # type: Optional[float]

        try:
            hipparcos_num = int(line[0:6])
        except ValueError:
            pass

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

        if hipparcos_num is not None and hipparcos_num in star_data.lookup_by_hip_num:
            uid = star_data.lookup_by_hip_num[hipparcos_num]
            star = star_data.star_list[uid]
        else:
            rejection_count += 1
            continue

        if parallax and (parallax_error < parallax * maximum_allowed_parallax_error):
            star.parallax = parallax
            star.source_par = source
        if proper_motion:
            star.proper_motion = proper_motion
            star.proper_motion_pa = proper_motion_pa

        # Ensure this star is recorded in the output catalogue
        star_data.add_star(star_descriptor=star)

    logging.info("  Total of {:d} stars.".format(len(star_data)))
    logging.info("  Rejected {:d} / {:d} catalogue entries.".format(rejection_count, total_count))


def read_from_gaia_dr1(star_data: StarList):
    # Read data from Gaia DR1
    logging.info("Reading Gaia DR1 catalogue...")
    source = "gaia_dr1"
    rejection_count = 0
    total_count = 0
    new_entry_count = 0
    for line in gzip.open("../gaiaDR1/tgas.dat.gz", "rt"):
        total_count += 1
        hipparcos_num = None  # type: Optional[int]
        tycho_id = None  # type: Optional[str]
        parallax = None  # type: Optional[float]
        parallax_error = None  # type: Optional[float]
        proper_motion = None  # type: Optional[float]
        proper_motion_pa = None  # type: Optional[float]

        try:
            tycho_id = line[7:19].strip()
        except ValueError:
            pass

        try:
            hipparcos_num = int(line[0:6])
        except ValueError:
            pass

        if (tycho_id is None) and (hipparcos_num is None):
            logging.info("Rejecting Gaia DR1 star: could not read either HIP or TYC identifier.")
            rejection_count += 1
            continue

        try:
            # epoch = float(line[68:74])  # Equals 2015.0
            ra = float(line[75:89])  # degrees, J2000 (but proper motion for epoch 2015.0)
            decl = float(line[97:111])  # degrees, J2000 (but proper motion for epoch 2015.0)
            mag = float(line[433:439])
        except ValueError:
            # logging.info("Rejecting Gaia DR1 star {} / {}".format(tycho_id, hipparcos_num))
            rejection_count += 1
            continue

        try:
            parallax = float(line[119:125])
            parallax_error = float(line[126:130])
        except ValueError:
            parallax = None
            parallax_error = None

        try:
            pm_ra = float(line[131:140])
            pm_dec = float(line[148:157])
            proper_motion = hypot(pm_ra, pm_dec)
            proper_motion_pa = (atan2(pm_ra, pm_dec) * 180 / pi + 720) % 360
        except ValueError:
            proper_motion = None
            proper_motion_pa = None

        # Fetch star descriptor
        star, hd_mismatch, hipparcos_mismatch = star_data.match_star(
            ra=ra, decl=decl, mag=mag,
            source=source, match_threshold=0.01,
            hipparcos_num=hipparcos_num, tycho_id=tycho_id)

        # If Hipparcos ID matched another star, wipe don't over-write it
        if hipparcos_mismatch:
            hipparcos_num = None

        # If we created a new entry, record this
        if star.id is None:
            new_entry_count += 1

        # Record this star's catalogue IDs
        if tycho_id:
            star.names_tyc_num = tycho_id
        if hipparcos_num:
            star.names_hip_num = hipparcos_num

        # Record this star's position and parallax
        star.in_gaia_dr1 = True
        star.ra = ra  # degrees, J2000
        star.decl = decl  # degrees, J2000
        star.source_pos = source
        if parallax is not None and (parallax_error < parallax * maximum_allowed_parallax_error):
            star.parallax = parallax
            star.source_par = source
        if proper_motion is not None:
            star.proper_motion = proper_motion
            star.proper_motion_pa = proper_motion_pa

        # Gaia G-band magnitudes are subject to errors
        # star_data[uid]['mag']['G'] = {'value': mag, 'source': source}

        # Ensure this star is recorded in the output catalogue
        star_data.add_star(star_descriptor=star)

    logging.info("  Total of {:d} stars.".format(len(star_data)))
    logging.info("  Rejected {:d} / {:d} catalogue entries.".format(rejection_count, total_count))


def read_from_gaia_dr2(star_data: StarList):
    # Read data from Gaia DR2
    logging.info("Reading Gaia DR2 catalogue...")
    source = "gaia_dr2"
    rejection_count = 0
    total_count = 0
    new_entry_count = 0
    for filename in glob.glob("../gaiaDR2/*.csv.gz"):
        columns = []
        for line in gzip.open(filename, "rt"):
            hipparcos_num = None  # type: Optional[int]
            parallax = None  # type: Optional[float]
            parallax_error = None  # type: Optional[float]
            proper_motion = None  # type: Optional[float]
            proper_motion_pa = None  # type: Optional[float]

            # First line contains column headings
            if not columns:
                columns = line.strip().split(",")
                continue

            # Read data lines
            total_count += 1
            words = line.strip().split(",")
            ra = float(words[columns.index("ra")])  # type: float
            decl = float(words[columns.index("dec")])  # type: float
            dr2_id = words[columns.index("source_id")]  # type: str
            tycho_id = words[columns.index("tycho")]  # type: str

            try:
                hipparcos_num = int(words[columns.index("hipparcos")])
            except ValueError:
                pass

            try:
                parallax = float(words[columns.index("parallax")])
                parallax_error = float(words[columns.index("parallax_error")])
            except ValueError:
                parallax = None
                parallax_error = None

            try:
                pm_ra = float(words[columns.index("pmra")])
                pm_dec = float(words[columns.index("pmdec")])
                proper_motion = hypot(pm_ra, pm_dec)
                proper_motion_pa = (atan2(pm_ra, pm_dec) * 180 / pi + 720) % 360
            except ValueError:
                proper_motion = None
                proper_motion_pa = None

            # if (not tycho_id) and (not hipparcos_num):
            #    logging.info("Rejecting Gaia DR2 star: could not read either HIP or TYC identifier.")
            #    rejection_count += 1
            #   continue

            # Fetch star descriptor
            star, hd_mismatch, hipparcos_mismatch = star_data.match_star(
                ra=ra, decl=decl,
                mag=float(words[columns.index("phot_g_mean_mag")]),
                source=source, match_threshold=0.01,
                hipparcos_num=hipparcos_num, tycho_id=tycho_id)

            # If Hipparcos ID matched another star, wipe don't over-write it
            if hipparcos_mismatch:
                hipparcos_num = None

            # Make logging entry if we created a new entry
            if star.id is None:
                new_entry_count += 1
                star_data.add_star(star_descriptor=star)  # Assign ID
                if tycho_id:
                    logging.info("Creating new entry ({}) for Gaia DR2 star TYC {}".format(star.id, tycho_id))
                elif hipparcos_num:
                    logging.info("Creating new entry ({}) for Gaia DR2 star HIP {}".format(star.id, hipparcos_num))

            # Record this star's catalogue IDs
            if dr2_id is not None:
                star.names_dr2_num = dr2_id
            if tycho_id:
                star.names_tyc_num = tycho_id
            if hipparcos_num:
                star.names_hip_num = hipparcos_num

            # Record this star's position and parallax
            star.in_gaia_dr2 = True
            star.ra = ra  # degrees, J2000
            star.decl = decl  # degrees, J2000
            star.source_pos = source
            if parallax is not None and (parallax_error < parallax * maximum_allowed_parallax_error):
                star.parallax = parallax
                star.source_par = source
            if proper_motion is not None:
                star.proper_motion = proper_motion
                star.proper_motion_pa = proper_motion_pa

            # Record this star's brightness
            mag = words[columns.index("phot_g_mean_mag")]
            if mag:
                star.mag_G = mag
                star.mag_G_source = source

            mag = words[columns.index("phot_bp_mean_mag")]
            if mag:
                star.mag_BP = mag
                star.mag_BP_source = source

            mag = words[columns.index("phot_rp_mean_mag")]
            if mag:
                star.mag_RP = mag
                star.mag_RP_source = source

            # Ensure this star is recorded in the output catalogue
            star_data.add_star(star_descriptor=star)

    logging.info("  Total of {:d} stars.".format(len(star_data)))
    logging.info("  Rejected {:d} / {:d} catalogue entries.".format(rejection_count, total_count))


def read_bayer_flamsteed_cross_index(star_data: StarList):
    star_name_converter = StarNameConversion()

    # Read the Bayer / Flamsteed cross index
    logging.info("  Total of {:d} stars.".format(len(star_data)))
    logging.info("  Total of {:d} Bayer letters.".format(star_data.count_stars_with_bayer_letters()))
    logging.info("  Total of {:d} Flamsteed numbers.".format(star_data.count_stars_with_flamsteed_numbers()))
    logging.info("Reading Cross Index catalogue...")

    rejection_count = 0
    total_count = 0
    for line in open("../bayerAndFlamsteed/catalog.dat"):
        hd_number = None
        flamsteed_number = None
        try:
            hd_number = int(line[0:6])
            bayer_letter = line[68:71].strip()  # Bayer letter of star, e.g. "Alp"
            bayer_const = line[74:77].strip()  # Constellation abbreviation, e.g. "Peg"
            bayer_letter_num = line[71:73].strip()  # Number which goes after Bayer letter, for example Alpha-2
            try:
                flamsteed_number = int(line[64:67].strip())
            except ValueError:
                flamsteed_number = None

            # Some stars are listed as c01 rather than "c  01"
            if (bayer_letter_num == "") and (len(bayer_letter) > 2) and (bayer_letter[-2] == "0"):
                bayer_letter_num = bayer_letter[-2:]
                bayer_letter = bayer_letter[:-2]
        except ValueError:
            logging.info("Rejecting HD {}".format(hd_number))
            rejection_count += 1
            continue

        # See if we can match this star to a known HD entry
        star = None  # type: Optional[StarDescriptor]
        if hd_number is not None and hd_number in star_data.lookup_by_hd_num:
            uid = star_data.lookup_by_hd_num[hd_number]
            star = star_data.star_list[uid]
        else:
            logging.info("Could not match HD {}".format(hd_number))
            rejection_count += 1
            continue

        # Look up existing Bayer letter, if any
        bayer_letter_old = star.names_bayer_letter

        # Look up existing Flamsteed number, if any
        flamsteed_number_old = star.names_flamsteed_number

        if bayer_letter:
            if bayer_letter in star_name_converter.greekAlphabetTeX:
                greek_letter = star_name_converter.greekAlphabetTeX[bayer_letter]
            else:
                greek_letter = bayer_letter
            if bayer_letter_num:
                greek_letter += "^{:d}".format(int(bayer_letter_num))
            if bayer_letter_old and (bayer_letter_old != greek_letter):
                logging.info("Warning, changing Bayer designation from <{}> to <{}> {} for HD {:d}"
                             .format(bayer_letter_old, greek_letter, bayer_const, hd_number))

            star.names_bayer_letter = greek_letter
            star.names_const = bayer_const

        if flamsteed_number:
            if flamsteed_number_old and (flamsteed_number_old != flamsteed_number):
                logging.info("Warning, changing Flamsteed designation from <{}> to <{}> {}"
                             .format(flamsteed_number_old, flamsteed_number, bayer_const))
            star.names_flamsteed_number = flamsteed_number
            star.names_const = bayer_const

    logging.info("  Total of {:d} Bayer letters.".format(star_data.count_stars_with_bayer_letters()))
    logging.info("  Total of {:d} Flamsteed numbers.".format(star_data.count_stars_with_flamsteed_numbers()))

    logging.info("  Total of {:d} stars.".format(len(star_data)))
    logging.info("  Rejected {:d} / {:d} catalogue entries.".format(rejection_count, total_count))


def read_english_names(star_data: StarList):
    # Read list of English names for stars
    logging.info("Reading English names for stars...")
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
            if cat_number in star_data.lookup_by_bs_num:
                uid = star_data.lookup_by_bs_num[cat_number]
        elif cat_name == "HD":
            if cat_number in star_data.lookup_by_hd_num:
                uid = star_data.lookup_by_hd_num[cat_number]
        elif cat_name == "HIP":
            if cat_number in star_data.lookup_by_hip_num:
                uid = star_data.lookup_by_hip_num[cat_number]
        if uid is None:
            logging.info("  Error - could not find star {} {}".format(cat_name, cat_number))
            continue

        # Add new English name for this star
        star = star_data.star_list[uid]
        star.add_english_name(new_name=name.strip(), prepend=True)

        # Add new manually-set parallax, if available
        if parallax:
            star.source_par = 'manual'
            star.parallax = parallax


def merge_star_catalogues():
    star_name_converter = StarNameConversion()

    # Make sure output directory exists
    os.system("mkdir -p output")

    # Big list of information about each star
    star_data = StarList()

    # Read each catalogue in turn
    read_from_ybsc(star_data=star_data)
    read_from_hipparcos(star_data=star_data)
    read_from_tycho_1(star_data=star_data)
    read_from_tycho_2(star_data=star_data)
    read_from_hipparcos_new(star_data=star_data)
    read_from_gaia_dr1(star_data=star_data)
    read_from_gaia_dr2(star_data=star_data)
    read_bayer_flamsteed_cross_index(star_data=star_data)
    read_english_names(star_data=star_data)

    # Work out distances of stars from parallaxes
    logging.info("Working out distances from parallaxes...")
    for item in star_data.star_list:
        if item.parallax:
            parallax = item.parallax
            if parallax > 0.0000001:
                item.dist = AU / (parallax * 1e-3 / 3600 / 180 * pi) / LYR  # Parallax in mas; Distance in LYR
            else:
                item.parallax = None


    # Create a histogram of the brightness of all the stars we have inserted into catalogue
    logging.info("Creating list of stars in order of brightness...")
    bin_size = 0.25
    star_magnitudes = []
    brightness_histogram = {}
    for uid, star in enumerate(star_data.star_list):
        mag_reference = None
        if star.mag_V is not None:
            mag_reference = float(star.mag_V)
        elif star.mag_G is not None:
            mag_reference = float(star.mag_G)

        if mag_reference is not None:
            star_magnitudes.append((uid, mag_reference, star.ra))
            key = floor(mag_reference / bin_size) * bin_size
            if key not in brightness_histogram:
                brightness_histogram[key] = 0
            brightness_histogram[key] += 1

    logging.info("  Total of {:d} stars with valid magnitudes.".format(len(star_magnitudes)))

    # Display a brightness histogram as a table
    logging.info("Brightness histogram:")
    total_stars = 0
    for i in np.arange(-2, 20, bin_size):
        val = 0
        if i in brightness_histogram:
            val = brightness_histogram[i]
        total_stars += val
        logging.info("  Mag {:6.2f} to {:6.2f} -- {:7d} stars (total {:7d} stars)".format(i, i + bin_size,
                                                                                          val, total_stars))

    # Sort stars into order of V-band magnitude before writing output.
    # Sort on RA as secondary field to produce reproducible output
    logging.info("Sorting list of stars in order of brightness")
    star_magnitudes.sort(key=lambda x: (x[1], x[2]))

    # Produce output catalogues
    logging.info("Producing output data catalogues")

    # Make sure that StarCharter regenerates its binary file
    os.system("rm -f output/*")

    # Write output as a text file for use in StarCharter
    with gzip.open("output/star_charter_stars.dat.gz", "wt") as output_star_charter:

        # Also write output as a big lump of JSON which other python scripts can use
        with gzip.open("output/all_stars.json.gz", "wt") as output_json:

            # Loop over all the objects we've catalogued
            for item in star_magnitudes:
                # item = [uid, reference magnitude, RA]

                # Some variables we feed data into
                parallax = dist = proper_motion = proper_motion_pa = 0
                mag_bv = ybsn_num = hd_num = hip_num = nsv = variable = 0
                source_pos = source_par = ""
                tycho_id = dr2_id = name_const = name_bayer_letter = name_flamsteed_num = ""
                names_english = names_catalogue_ref = []

                # name_1 is the star's Bayer letter, as a UTF8 character
                # name_2 is 'x-c' where x is name_1 and c is the constellation
                # name_3 is the star's English name, which spaces rendered as underscores
                # name_4 is the star's catalogue name, e.g. "V337_Car"
                # name_5 is 'y' where y is the Flamsteed number
                name_1 = name_2 = name_3 = name_4 = name_5 = "-"

                # Populate variables with data from the StarDescriptor structure
                uid = item[0]
                star = star_data.star_list[uid]
                ra = star.ra
                decl = star.decl
                source_pos = star.source_pos
                mag_list = star.magnitude_dictionary()
                if star.parallax is not None:
                    parallax = star.parallax
                    dist = star.dist
                    source_par = star.source_par
                if star.proper_motion is not None:
                    proper_motion = star.proper_motion
                    proper_motion_pa = star.proper_motion_pa
                if star.color_bv is not None:
                    mag_bv = star.color_bv
                if star.is_variable:
                    variable = 1

                # Catalogue numbers for this star
                if star.names_bs_num is not None:
                    ybsn_num = star.names_bs_num
                if star.names_hd_num is not None:
                    hd_num = star.names_hd_num
                if star.names_nsv_num is not None:
                    nsv = star.names_nsv_num
                if star.names_hip_num is not None:
                    hip_num = star.names_hip_num
                if star.names_tyc_num is not None:
                    tycho_id = star.names_tyc_num
                if star.names_dr2_num is not None:
                    dr2_id = star.names_dr2_num

                # Something dodgy has happened if this star doesn't have any catalogue identifications
                if (not ybsn_num) and (not hd_num) and (not hip_num) and (not tycho_id) and (not dr2_id):
                    logging.info("Warning: Star in database with no valid catalogue IDs")

                # Names for this star
                if star.names_english:
                    name_3 = re.sub(' ', '_', star.names_english[0])  # StarCharter using whitespace-sep columns
                    names_english = star.names_english  # JSON output allows spaces in the names of stars
                if star.names_catalogue_ref:
                    name_4 = re.sub(' ', '_', star.names_catalogue_ref[0])  # StarCharter using whitespace-sep columns
                    names_catalogue_ref = star.names_catalogue_ref  # JSON output allows spaces in the names of stars
                if star.names_const:
                    name_const = star.names_const
                    if star.names_bayer_letter:
                        name_bayer_letter = star.names_bayer_letter
                        name_1 = star_name_converter.greek_html_to_utf8(
                            instr=star_name_converter.name_to_html(instr=name_bayer_letter)
                        )
                        name_2 = "{}-{}".format(name_1, star.names_const)
                    if star.names_flamsteed_number:
                        name_5 = "{}".format(star.names_flamsteed_number)
                        name_flamsteed_num = star.names_flamsteed_number

                # Limiting magnitude of 12 for StarCharter
                mag_limit = 12

                mag_reference = None
                if star.mag_V is not None:
                    mag_reference = float(star.mag_V)
                elif star.mag_G is not None:
                    mag_reference = float(star.mag_G)

                if (mag_reference is not None) and (mag_reference < mag_limit):
                    output_star_charter.write(
                        "{:6d} {:6d} {:8d} {:17.12f} {:17.12f} {:17.12f} {:17.12f} {:17.12f} {:s} {:s} {:s} {:s} {:s}\n"
                            .format(hd_num, ybsn_num, hip_num, ra, decl, mag_reference, parallax, dist,
                                    name_1, name_2, name_3, name_4, name_5)
                    )

                # Dump absolutely everything into the JSON output
                json_structure = [ra, decl, mag_list, parallax, dist, ybsn_num, hd_num, hip_num, tycho_id,
                                  names_english + names_catalogue_ref, name_const,
                                  star_name_converter.name_to_html(instr=name_bayer_letter),
                                  star_name_converter.name_to_ascii(instr=name_bayer_letter),
                                  name_flamsteed_num,
                                  source_pos, source_par,
                                  mag_bv, proper_motion, proper_motion_pa, nsv, variable, dr2_id]
                output_json.write(json.dumps(json_structure))
                output_json.write("\n")


# Do it right away if we're run as a script
if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO,
                        stream=sys.stdout,
                        format='[%(asctime)s] %(levelname)s:%(filename)s:%(message)s',
                        datefmt='%d/%m/%Y %H:%M:%S')
    logger = logging.getLogger(__name__)
    logger.info(__doc__.strip())

    merge_star_catalogues()
