#!/usr/bin/python3
# -*- coding: utf-8 -*-
# main_catalogue_merge.py
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

"""
Take the Bright Star, Hipparcos, Tycho Catalogues, Gaia DR1 and Gaia DR2, and merge them with all the catalogue IDs
listed for each star.
"""

import argparse
import glob
import gzip
import json
import logging
import os
import re
import sys

from math import pi, floor, hypot, atan2
from typing import Dict, Final, Optional, List, Sequence, Tuple

import numpy as np

from star_descriptor import StarDescriptor, StarList
from star_name_conversion import StarNameConversion

# Constants used for units conversions
AU: Final[float] = 1.49598e11  # metres
LYR: Final[float] = 9.4605284e15  # lightyear in metres

# Reject parallaxes with more than a 30% error
maximum_allowed_parallax_error: Final[float] = 0.3


def read_from_ybsc(star_data: StarList) -> None:
    """
    Read stars from Yale Bright Star Catalogue.

    :param star_data:
        The list of stars we merge new entries into.
    :return:
        None
    """
    star_name_converter: StarNameConversion = StarNameConversion()

    # Read stars from Bright Star Catalogue
    logging.info("Reading data from Yale Bright Star Catalogue...")
    source: str = "bright_stars"
    rejection_count: int = 0
    total_count: int = 0
    for line in gzip.open("../brightStars/catalog.gz", "rt"):
        total_count += 1
        bs_number: Optional[int] = None
        hd_number: Optional[int] = None
        try:
            bs_number: int = int(line[0:4])
            ra_hours: float = float(line[75:77])
            ra_minutes: float = float(line[77:79])
            ra_seconds: float = float(line[79:83])
            decl_degrees: float = float(line[84:86])
            decl_minutes: float = float(line[86:88])
            decl_seconds: float = float(line[88:90])
            decl_sign: str = line[83]
            mag: float = float(line[102:107])
            variability: str = line[51:60].strip().split('/')[0]
            if variability and (variability[-1] == '?'):
                variability = ""  # Clear variability flag if uncertain
            ra: float = (ra_hours + ra_minutes / 60 + ra_seconds / 3600)  # hours, J2000
            decl: float = (decl_degrees + decl_minutes / 60 + decl_seconds / 3600)  # degrees, J2000
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
        flamsteed_number: Optional[int] = None
        try:
            flamsteed_number = int(line[4:7])
        except ValueError:
            pass

        bayer_letter: str = line[7:10].strip()  # Bayer letter of star, e.g. "Alp"
        bayer_const: str = line[11:14].strip()  # Constellation abbreviation, e.g. "Peg"
        bayer_letter_num: str = line[10]  # Number which goes after Bayer letter, for example Alpha-2

        star: Optional[StarDescriptor] = StarDescriptor()

        if bayer_letter in star_name_converter.greek_alphabet_tex_lookup:
            greek_letter = star_name_converter.greek_alphabet_tex_lookup[bayer_letter]
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


def read_from_hipparcos(star_data: StarList) -> None:
    """
    Read stars from Hipparcos Catalogue.

    :param star_data:
        The list of stars we merge new entries into.
    :return:
        None
    """
    # Read data from Hipparcos catalogue
    logging.info("Reading Hipparcos catalogue...")
    source: str = "hipparcos"
    rejection_count: int = 0
    total_count: int = 0
    for line in gzip.open("../tycho1/hip_main.dat.gz", "rt"):
        total_count += 1
        hipparcos_num: Optional[int] = None
        hd_number: Optional[int] = None
        parallax: Optional[float] = None
        parallax_error: Optional[float] = None
        mag_bv: Optional[float] = None
        proper_motion: Optional[float] = None
        proper_motion_pa: Optional[float] = None

        try:
            hipparcos_num: int = int(line[2:14])
            mag: float = float(line[41:46])
            ra: float = float(line[51:63])  # degrees, J2000 (but proper motion for epoch 1991.25)
            decl: float = float(line[64:76])  # degrees, J2000 (but proper motion for epoch 1991.25)
        except ValueError:
            # logging.info("Rejecting HIP {:d}".format(hipparcos_num))
            rejection_count += 1
            continue

        try:
            mag_bt: Optional[float] = float(line[217:223])
        except ValueError:
            mag_bt = None

        try:
            mag_vt: Optional[float] = float(line[230:236])
        except ValueError:
            mag_vt = None

        try:
            mag_bv: Optional[float] = float(line[245:251])
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
        star, ids_to_delete = star_data.match_star(
            ra=ra, decl=decl, mag=mag,
            source=source, match_threshold=0.1,
            hd_number=hd_number, hipparcos_num=hipparcos_num)

        # If HD / Hipparcos ID matched another star, we don't overwrite it
        if 'hd' in ids_to_delete and ids_to_delete['hd']:
            hd_number = None
        if 'hip' in ids_to_delete and ids_to_delete['hip']:
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


def read_from_tycho_1(star_data: StarList) -> None:
    """
    Read stars from Tycho 1 Catalogue.

    :param star_data:
        The list of stars we merge new entries into.
    :return:
        None
    """
    # Read data from Tycho 1 catalogue (this contains Henry Draper numbers, missing from Tycho 2)
    logging.info("Reading Tycho 1 catalogue...")
    source: str = "tycho1"
    rejection_count: int = 0
    total_count: int = 0
    with open("../tycho1/tyc_main.dat") as f_in:
        for line in f_in:
            total_count += 1
            hipparcos_num: Optional[int] = None
            hd_number: Optional[int] = None
            tycho_id: Optional[str] = None
            parallax: Optional[float] = None
            parallax_error: Optional[float] = None
            mag_bv: Optional[float] = None
            proper_motion: Optional[float] = None
            proper_motion_pa: Optional[float] = None

            try:
                tycho_id: str = "{:d}-{:d}-{:d}".format(int(line[2:6]), int(line[6:12]), int(line[13]))
                ra: float = float(line[51:63])  # degrees, J2000 (but proper motion for epoch 1991.25)
                decl: float = float(line[64:76])  # degrees, J2000 (but proper motion for epoch 1991.25)
                mag: float = float(line[41:46])
                mag_bv: float = float(line[245:251])
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
                pm_ra: float = float(line[87:95])  # Proper motion mu_alpha.cos(delta), ICRS
                pm_dec: float = float(line[96:104])
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
            star, ids_to_delete = star_data.match_star(
                ra=ra, decl=decl, mag=mag,
                source=source, match_threshold=0.1,
                hd_number=hd_number, hipparcos_num=hipparcos_num, tycho_id=tycho_id)

            # If HD / Hipparcos ID matched another star, we don't overwrite it
            if 'hd' in ids_to_delete and ids_to_delete['hd']:
                hd_number = None
            if 'hip' in ids_to_delete and ids_to_delete['hip']:
                hipparcos_num = None
            if 'tyc' in ids_to_delete and ids_to_delete['tyc']:
                tycho_id = None

            # Record this star's catalogue IDs
            if tycho_id:
                star.names_tycho_id = tycho_id
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


def read_from_tycho_2(star_data: StarList) -> None:
    """
    Read stars from Tycho 2 Catalogue.

    :param star_data:
        The list of stars we merge new entries into.
    :return:
        None
    """
    # Read data from Tycho 2 catalogue
    logging.info("Reading Tycho 2 catalogue...")
    source: str = "tycho2"
    rejection_count: int = 0
    total_count: int = 0
    for filename in glob.glob("../tycho2/tyc2.dat.*.gz"):
        for line in gzip.open(filename, "rt"):
            total_count += 1
            hipparcos_num: Optional[int] = None
            tycho_id: Optional[str] = None

            try:
                tycho_id: str = "{:d}-{:d}-{:d}".format(int(line[0:4]), int(line[5:10]), int(line[11]))
                ra: float = float(line[15:27])  # degrees, J2000 (but proper motion for epoch 1991.25)
                decl: float = float(line[28:40])  # degrees, J2000 (but proper motion for epoch 1991.25)
                mag_bt: float = float(line[110:116])
                mag_vt: float = float(line[123:129])
                mag: float = mag_vt - 0.090 * (
                        mag_bt - mag_vt)  # Johnson V-band; see http://heasarc.nasa.gov/W3Browse/all/tycho2.html
                mag_bv: float = 0.850 * (mag_bt - mag_vt)  # B-V colour
            except ValueError:
                # logging.info("Rejecting Tycho ID {}".format(tycho_id))
                rejection_count += 1
                continue
            try:
                hipparcos_num = int(line[142:148])
            except ValueError:
                pass

            # Fetch star descriptor
            star, ids_to_delete = star_data.match_star(
                ra=ra, decl=decl, mag=mag,
                source=source, match_threshold=0.01,
                hipparcos_num=hipparcos_num, tycho_id=tycho_id)

            # If Hipparcos ID matched another star, we don't overwrite it
            if 'hip' in ids_to_delete and ids_to_delete['hip']:
                hipparcos_num = None
            if 'tyc' in ids_to_delete and ids_to_delete['tyc']:
                tycho_id = None

            # Record this star's catalogue IDs
            if tycho_id:
                star.names_tycho_id = tycho_id
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


def read_from_hipparcos_new(star_data: StarList) -> None:
    """
    Read stars from Hipparcos new reduction catalogue.

    :param star_data:
        The list of stars we merge new entries into.
    :return:
        None
    """
    # Read data from Hipparcos new reduction
    logging.info("Reading Hipparcos new reduction catalogue...")
    source: str = "hipparcos_new"
    rejection_count: int = 0
    total_count: int = 0
    new_entry_count: int = 0
    for line in gzip.open("../hipparcosNewReduction/hip2.dat.gz", "rt"):
        total_count += 1
        hipparcos_num: Optional[int] = None
        parallax: Optional[float] = None
        parallax_error: Optional[float] = None
        mag_bv: Optional[float] = None
        proper_motion: Optional[float] = None
        proper_motion_pa: Optional[float] = None

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
            pm_ra: float = float(line[51:59])  # Proper motion mu_alpha.cos(delta), ICRS
            pm_dec: float = float(line[60:68])
            proper_motion = hypot(pm_ra, pm_dec)
            proper_motion_pa = (atan2(pm_ra, pm_dec) * 180 / pi + 720) % 360
        except ValueError:
            pass

        if hipparcos_num is not None and hipparcos_num in star_data.lookup_by_hip_num:
            uid: int = star_data.lookup_by_hip_num[hipparcos_num]
            star: StarDescriptor = star_data.star_list[uid]
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
    source: str = "gaia_dr1"
    rejection_count: int = 0
    total_count: int = 0
    new_entry_count: int = 0
    for line in gzip.open("../gaiaDR1/tgas.dat.gz", "rt"):
        total_count += 1
        hipparcos_num: Optional[int] = None
        tycho_id: Optional[str] = None
        parallax: Optional[float] = None
        parallax_error: Optional[float] = None
        proper_motion: Optional[float] = None
        proper_motion_pa: Optional[float] = None

        try:
            tycho_id: Optional[str] = line[7:19].strip()
        except ValueError:
            pass

        try:
            hipparcos_num: Optional[int] = int(line[0:6])
        except ValueError:
            pass

        if (tycho_id is None) and (hipparcos_num is None):
            logging.info("Rejecting Gaia DR1 star: could not read either HIP or TYC identifier.")
            rejection_count += 1
            continue

        try:
            # epoch = float(line[68:74])  # Equals 2015.0
            ra: float = float(line[75:89])  # degrees, J2000 (but proper motion for epoch 2015.0)
            decl: float = float(line[97:111])  # degrees, J2000 (but proper motion for epoch 2015.0)
            mag: float = float(line[433:439])
        except ValueError:
            # logging.info("Rejecting Gaia DR1 star {} / {}".format(tycho_id, hipparcos_num))
            rejection_count += 1
            continue

        try:
            parallax: Optional[float] = float(line[119:125])
            parallax_error: Optional[float] = float(line[126:130])
        except ValueError:
            parallax = None
            parallax_error = None

        try:
            pm_ra: Optional[float] = float(line[131:140])
            pm_dec: Optional[float] = float(line[148:157])
            proper_motion: Optional[float] = hypot(pm_ra, pm_dec)
            proper_motion_pa: Optional[float] = (atan2(pm_ra, pm_dec) * 180 / pi + 720) % 360
        except ValueError:
            proper_motion = None
            proper_motion_pa = None

        # Fetch star descriptor
        star, ids_to_delete = star_data.match_star(
            ra=ra, decl=decl, mag=mag,
            source=source, match_threshold=0.01,
            hipparcos_num=hipparcos_num, tycho_id=tycho_id)

        # If Hipparcos ID matched another star, we don't overwrite it
        if 'hip' in ids_to_delete and ids_to_delete['hip']:
            hipparcos_num = None
        if 'tyc' in ids_to_delete and ids_to_delete['tyc']:
            tycho_id = None

        # If we created a new entry, record this
        if star.id is None:
            new_entry_count += 1

        # Record this star's catalogue IDs
        if tycho_id:
            star.names_tycho_id = tycho_id
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
    source: str = "gaia_dr2"
    rejection_count: int = 0
    total_count: int = 0
    new_entry_count: int = 0
    for filename in glob.glob("../gaiaDR2/*.csv.gz"):
        columns = []
        for line in gzip.open(filename, "rt"):
            hipparcos_num: Optional[int] = None
            parallax: Optional[float] = None
            parallax_error: Optional[float] = None
            proper_motion: Optional[float] = None
            proper_motion_pa: Optional[float] = None

            # First line contains column headings
            if not columns:
                columns = line.strip().split(",")
                continue

            # Read data lines
            total_count += 1
            words = line.strip().split(",")
            ra: float = float(words[columns.index("ra")])
            decl: float = float(words[columns.index("dec")])
            dr2_id: str = words[columns.index("source_id")]
            tycho_id: Optional[str] = words[columns.index("tycho")]

            try:
                hipparcos_num: Optional[int] = int(words[columns.index("hipparcos")])
            except ValueError:
                pass

            try:
                parallax: Optional[float] = float(words[columns.index("parallax")])
                parallax_error: Optional[float] = float(words[columns.index("parallax_error")])
            except ValueError:
                parallax = None
                parallax_error = None

            try:
                pm_ra: Optional[float] = float(words[columns.index("pmra")])
                pm_dec: Optional[float] = float(words[columns.index("pmdec")])
                proper_motion: Optional[float] = hypot(pm_ra, pm_dec)
                proper_motion_pa: Optional[float] = (atan2(pm_ra, pm_dec) * 180 / pi + 720) % 360
            except ValueError:
                proper_motion = None
                proper_motion_pa = None

            # if (not tycho_id) and (not hipparcos_num):
            #    logging.info("Rejecting Gaia DR2 star: could not read either HIP or TYC identifier.")
            #    rejection_count += 1
            #   continue

            # Fetch star descriptor
            star, ids_to_delete = star_data.match_star(
                ra=ra, decl=decl, mag=float(words[columns.index("phot_g_mean_mag")]),
                source=source, match_threshold=0.01,
                hipparcos_num=hipparcos_num, tycho_id=tycho_id, dr2_id=dr2_id)

            # If Hipparcos ID matched another star, we don't overwrite it
            if 'hip' in ids_to_delete and ids_to_delete['hip']:
                hipparcos_num = None
            if 'tyc' in ids_to_delete and ids_to_delete['tyc']:
                tycho_id = None
            if 'dr2' in ids_to_delete and ids_to_delete['dr2']:
                continue

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
                star.names_tycho_id = tycho_id
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
                star.mag_G = float(mag)
                star.mag_G_source = source

            mag = words[columns.index("phot_bp_mean_mag")]
            if mag:
                star.mag_BP = float(mag)
                star.mag_BP_source = source

            mag = words[columns.index("phot_rp_mean_mag")]
            if mag:
                star.mag_RP = float(mag)
                star.mag_RP_source = source

            # Ensure this star is recorded in the output catalogue
            star_data.add_star(star_descriptor=star)

    logging.info("  Total of {:d} stars.".format(len(star_data)))
    logging.info("  Rejected {:d} / {:d} catalogue entries.".format(rejection_count, total_count))


def read_bayer_flamsteed_cross_index(star_data: StarList) -> None:
    """
    Read cross-matches between the Bayer and Flamsteed catalogues.

    :param star_data:
        The list of stars we merge new entries into.
    :return:
        None
    """
    star_name_converter: StarNameConversion = StarNameConversion()

    # Read the Bayer / Flamsteed cross-index
    logging.info("  Total of {:d} stars.".format(len(star_data)))
    logging.info("  Total of {:d} Bayer letters.".format(star_data.count_stars_with_bayer_letters()))
    logging.info("  Total of {:d} Flamsteed numbers.".format(star_data.count_stars_with_flamsteed_numbers()))
    logging.info("Reading Cross Index catalogue...")

    rejection_count: int = 0
    total_count: int = 0
    with open("../bayerAndFlamsteed/catalog.dat") as f_in:
        for line in f_in:
            hd_number: Optional[int] = None
            flamsteed_number: Optional[int] = None
            try:
                hd_number: Optional[int] = int(line[0:6])
                bayer_letter: str = line[68:71].strip()  # Bayer letter of star, e.g. "Alp"
                bayer_const: str = line[74:77].strip()  # Constellation abbreviation, e.g. "Peg"
                bayer_letter_num: str = line[71:73].strip()  # Number which goes after Bayer letter, for example Alpha-2
                try:
                    flamsteed_number: Optional[int] = int(line[64:67].strip())
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
            star: Optional[StarDescriptor] = None
            if hd_number is not None and hd_number in star_data.lookup_by_hd_num:
                uid: int = star_data.lookup_by_hd_num[hd_number]
                star = star_data.star_list[uid]
            else:
                logging.info("Could not match HD {}".format(hd_number))
                rejection_count += 1
                continue

            # Look up existing Bayer letter, if any
            bayer_letter_old: Optional[str] = star.names_bayer_letter

            # Look up existing Flamsteed number, if any
            flamsteed_number_old: Optional[int] = star.names_flamsteed_number

            if bayer_letter:
                if bayer_letter in star_name_converter.greek_alphabet_tex_lookup:
                    greek_letter: str = star_name_converter.greek_alphabet_tex_lookup[bayer_letter]
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


def read_english_names(star_data: StarList) -> None:
    """
    Read popular names for stars.

    :param star_data:
        The list of stars we merge new entries into.
    :return:
        None
    """
    # Read list of English names for stars
    logging.info("Reading English names for stars...")
    with open("../brightStars/starNames_iau.txt") as f_in:
        for line in f_in:
            if (len(line) < 5) or (line[0] == '#'):
                continue
            words: Sequence[str] = line.split()
            cat_name: str = words[0]
            cat_number: int = int(words[1])
            parallax: float = float(words[2])
            name: str = line[20:]
            uid: Optional[int] = None
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
            star: StarDescriptor = star_data.star_list[uid]
            star.add_english_name(new_name=name.strip(), prepend=True)

            # Add new manually-set parallax, if available
            if parallax:
                star.source_par = 'manual'
                star.parallax = parallax


def reference_magnitude(star) -> Optional[float]:
    """
    Calculate a reference (V-band) magnitude for a star. If the star has a V-band magnitude, then use that. Otherwise,
    estimate a V-band magnitude from Gaia photometry.

    :param star:
        Star descriptor
    :type star:
        StarDescriptor
    :return:
        V-band magnitude
    """
    mag_reference: Optional[float] = None
    if star.mag_V is not None:
        mag_reference = star.mag_V
    elif star.mag_G is not None and star.mag_BP is not None and star.mag_RP is not None:
        # https://gea.esac.esa.int/archive/documentation/GDR2/Data_processing/chap_cu5pho/sec_cu5pho_calibr/ssec_cu5pho_PhotTransf.html
        br = star.mag_BP - star.mag_RP
        mag_reference = star.mag_G + 0.01760 + 0.006860 * br + 0.1732 * br * br
    elif star.mag_G is not None:
        mag_reference = star.mag_G + 0.01760

    return mag_reference


def merge_star_catalogues(magnitude_limit: Optional[float] = None) -> None:
    """
    Main entry point for merging all available star catalogues.

    :param magnitude_limit:
        The magnitude of the faintest stars to include in the output catalogue
    :return:
        None
    """
    star_name_converter: StarNameConversion = StarNameConversion()

    # Make sure output directory exists
    os.system("mkdir -p output")

    # Big list of information about each star
    star_data: StarList = StarList(magnitude_limit=magnitude_limit)

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
            parallax: float = item.parallax
            if parallax > 0.0000001:
                item.dist = AU / (parallax * 1e-3 / 3600 / 180 * pi) / LYR  # Parallax in mas; Distance in LYR
            else:
                item.parallax = None

    # Create a histogram of the brightness of all the stars we have inserted into catalogue
    logging.info("Creating list of stars in order of brightness...")
    bin_size: float = 0.25
    star_magnitudes: List[Tuple[float, float, float]] = []
    brightness_histogram: Dict[float, int] = {}
    for uid, star in enumerate(star_data.star_list):
        mag_reference: Optional[float] = reference_magnitude(star)

        if mag_reference is not None:
            star_magnitudes.append((uid, mag_reference, star.ra))
            key: float = floor(mag_reference / bin_size) * bin_size
            if key not in brightness_histogram:
                brightness_histogram[key] = 0
            brightness_histogram[key] += 1

    logging.info("  Total of {:d} stars with valid magnitudes.".format(len(star_magnitudes)))

    # Display a brightness histogram as a table
    logging.info("Brightness histogram:")
    total_stars: int = 0
    for bin_pos in np.arange(-2, 20, bin_size):
        val: int = 0
        if bin_pos in brightness_histogram:
            val = brightness_histogram[bin_pos]
        total_stars += val
        logging.info("  Mag {:6.2f} to {:6.2f} -- {:7d} stars (total {:7d} stars)".format(
            bin_pos, bin_pos + bin_size, val, total_stars))

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
                mag_bv = ybsn_num = hd_num = hip_num = nsv = variable = name_flamsteed_num = 0
                source_pos = source_par = ""
                tycho_id = dr2_id = name_const = name_bayer_letter = ""
                names_english = names_catalogue_ref = []

                # name_1 is the star's Bayer letter, as a UTF8 character
                # name_2 is 'x-c' where x is name_1 and c is the constellation
                # name_3 is the star's English name, which spaces rendered as underscores
                # name_4 is the star's catalogue name, e.g. "V337_Car"
                # name_5 is 'y' where y is the Flamsteed number
                name_1 = name_2 = name_3 = name_4 = name_5 = "-"

                # Populate variables with data from the StarDescriptor structure
                uid: int = int(item[0])
                star: StarDescriptor = star_data.star_list[uid]
                ra: float = star.ra
                decl: float = star.decl
                source_pos: str = star.source_pos
                mag_list: dict = star.magnitude_dictionary()
                if star.parallax is not None:
                    parallax: Optional[float] = star.parallax
                    dist: Optional[float] = star.dist
                    source_par: Optional[str] = star.source_par
                if star.proper_motion is not None:
                    proper_motion: Optional[float] = star.proper_motion
                    proper_motion_pa: Optional[float] = star.proper_motion_pa
                if star.color_bv is not None:
                    mag_bv: Optional[float] = star.color_bv
                if star.is_variable:
                    variable: int = 1

                # Catalogue numbers for this star
                if star.names_bs_num is not None:
                    ybsn_num: int = star.names_bs_num
                if star.names_hd_num is not None:
                    hd_num: int = star.names_hd_num
                if star.names_nsv_num is not None:
                    nsv: int = star.names_nsv_num
                if star.names_hip_num is not None:
                    hip_num: int = star.names_hip_num
                if star.names_tycho_id is not None:
                    tycho_id: str = star.names_tycho_id
                if star.names_dr2_num is not None:
                    dr2_id: str = star.names_dr2_num

                # Something dodgy has happened if this star doesn't have any catalogue identifications
                if (not ybsn_num) and (not hd_num) and (not hip_num) and (not tycho_id) and (not dr2_id):
                    logging.info("Warning: Star in database with no valid catalogue IDs")
                    continue

                # Names for this star
                if star.names_english:
                    # StarCharter using whitespace-sep columns
                    name_3: str = re.sub(' ', '_', star.names_english[0])

                    # JSON output allows spaces in the names of stars
                    names_english: List[str] = star.names_english
                if star.names_catalogue_ref:
                    # StarCharter using whitespace-sep columns
                    name_4: str = re.sub(' ', '_', star.names_catalogue_ref[0])

                    # JSON output allows spaces in the names of stars
                    names_catalogue_ref: List[str] = star.names_catalogue_ref
                if star.names_const:
                    name_const: Optional[str] = star.names_const
                    if star.names_bayer_letter:
                        name_bayer_letter: Optional[str] = star.names_bayer_letter
                        name_1: str = star_name_converter.greek_html_to_utf8(
                            in_str=star_name_converter.name_to_html(in_str=name_bayer_letter)
                        )
                        name_2: str = "{}-{}".format(name_1, star.names_const)
                    if star.names_flamsteed_number:
                        name_5: str = "{}".format(star.names_flamsteed_number)
                        name_flamsteed_num: Optional[int] = star.names_flamsteed_number

                # Limiting magnitude of 14 for StarCharter
                mag_limit: float = 14

                mag_reference: Optional[float] = reference_magnitude(star)

                if (mag_reference is not None) and (mag_reference < mag_limit):
                    output_star_charter.write(
                        "{:6d} {:6d} {:8d} {:17.12f} {:17.12f} {:17.12f} {:17.12f} {:17.12f} {:17.12f} {:s} {:s} {:s} {:s} {:s}\n"
                        .format(hd_num, ybsn_num, hip_num, ra, decl, mag_reference, mag_bv, parallax, dist,
                                name_1, name_2, name_3, name_4, name_5)
                    )

                # Dump absolutely everything into the JSON output
                json_structure = [ra, decl, mag_list, parallax, dist, ybsn_num, hd_num, hip_num, tycho_id,
                                  names_english + names_catalogue_ref, name_const,
                                  star_name_converter.name_to_html(in_str=name_bayer_letter),
                                  star_name_converter.name_to_ascii(in_str=name_bayer_letter),
                                  name_flamsteed_num,
                                  source_pos, source_par,
                                  mag_bv, proper_motion, proper_motion_pa, nsv, variable, dr2_id]
                output_json.write(json.dumps(json_structure))
                output_json.write("\n")


# Do it right away if we're run as a script
if __name__ == "__main__":
    # Read command-line arguments
    parser = argparse.ArgumentParser(description=__doc__)

    # Add command-line options
    parser.add_argument('--magnitude-limit', required=False, dest='magnitude_limit', type=float,
                        help='Faintest magnitude of stars to store in catalogue')
    args = parser.parse_args()

    # Set up logging
    logging.basicConfig(level=logging.INFO,
                        stream=sys.stdout,
                        format='[%(asctime)s] %(levelname)s:%(filename)s:%(message)s',
                        datefmt='%d/%m/%Y %H:%M:%S')
    logger = logging.getLogger(__name__)
    logger.info(__doc__.strip())

    # Perform merger operation
    merge_star_catalogues(magnitude_limit=args.magnitude_limit)
