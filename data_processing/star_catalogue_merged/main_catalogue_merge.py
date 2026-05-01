#!/usr/bin/python3
# -*- coding: utf-8 -*-
# main_catalogue_merge.py
#
# -------------------------------------------------
# Copyright 2015-2026 Dominic Ford
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
Take the Bright Star, Hipparcos, Tycho Catalogues, Gaia EDR3, and merge them with all the catalogue IDs
listed for each star.
"""

import argparse
import glob
import gzip
import json
import logging
import numpy as np
import os
import re
import sys

from math import atan2, floor, hypot, pi
from typing import Final, Optional, List, Sequence

from star_descriptor import StarDescriptor, StarList
from star_name_conversion import StarNameConversion

# File path of this script, used to locate input and output paths
our_path: Final[str] = os.path.split(os.path.abspath(__file__))[0]
input_path: Final[str] = os.path.join(our_path, "../../data/stars")
output_path: Final[str] = os.path.join(our_path, "../../data_generated/star_catalogue_merged")

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
    ybsc_path: Final[str] = os.path.join(input_path, "brightStars/catalog.gz")
    for line in gzip.open(ybsc_path, "rt"):
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

        # Variable star ID may be an NSV number, a designation e.g. PV Cep, or a Bayer letter (which we ignore)
        nsv_number: Optional[int] = None
        if variability.isdigit():
            nsv_number = int(variability)

        # Fetch star descriptor
        star = star_data.match_star(
            ra=ra, decl=decl, mag=mag,
            source=source, match_threshold=0.1,
            hd_number=hd_number, bs_number=bs_number, nsv_number=nsv_number)

        if bayer_letter in star_name_converter.greek_alphabet_tex_lookup:
            greek_letter = star_name_converter.greek_alphabet_tex_lookup[bayer_letter]
            if bayer_letter_num in '123456789':
                greek_letter += "^{}".format(bayer_letter_num)
            star.names_bayer_letter = greek_letter
            star.names_const = bayer_const
        if flamsteed_number is not None:
            star.names_flamsteed_number = flamsteed_number

        # Variable star ID may be an NSV number, a designation e.g. PV Cep, or a Bayer letter (which we ignore)
        if (not variability.isdigit()) and (
                variability and
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

    logging.info("-> Total of {:,} stars in database.".format(len(star_data)))
    logging.info("-> Rejected {:,} / {:,} new catalogue entries.".format(rejection_count, total_count))


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
    tycho1_path: Final[str] = os.path.join(input_path, "tycho1/hip_main.dat.gz")
    for line in gzip.open(tycho1_path, "rt"):
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
        star = star_data.match_star(
            ra=ra, decl=decl, mag=mag,
            source=source, match_threshold=0.1,
            hd_number=hd_number, hipparcos_num=hipparcos_num)

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

        # Ensure this star is recorded in the output catalogue
        star_data.add_star(star_descriptor=star)

    logging.info("-> Total of {:,} stars in database.".format(len(star_data)))
    logging.info("-> Rejected {:,} / {:,} new catalogue entries.".format(rejection_count, total_count))


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
    tycho1_path: Final[str] = os.path.join(input_path, "tycho1/tyc_main.dat")
    with open(tycho1_path, "rt") as f_in:
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
            star = star_data.match_star(
                ra=ra, decl=decl, mag=mag,
                source=source, match_threshold=0.1,
                hd_number=hd_number, hipparcos_num=hipparcos_num, tycho_id=tycho_id)

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

    logging.info("-> Total of {:,} stars in database.".format(len(star_data)))
    logging.info("-> Rejected {:,} / {:,} new catalogue entries.".format(rejection_count, total_count))


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
    tycho2_path: Final[str] = os.path.join(input_path, "tycho2/tyc2.dat.*.gz")
    for filename in glob.glob(tycho2_path):
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
            star = star_data.match_star(
                ra=ra, decl=decl, mag=mag,
                source=source, match_threshold=0.01,
                hipparcos_num=hipparcos_num, tycho_id=tycho_id)

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

    logging.info("-> Total of {:,} stars in database.".format(len(star_data)))
    logging.info("-> Rejected {:,} / {:,} new catalogue entries.".format(rejection_count, total_count))


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
    hipparcos_path: Final[str] = os.path.join(input_path, "hipparcosNewReduction/hip2.dat.gz")
    for line in gzip.open(hipparcos_path, "rt"):
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

        if hipparcos_num is not None:
            matches: List[StarDescriptor] = list(star_data.lookup_by_hip_num(hip_num=hipparcos_num))
            if len(matches) > 0:
                star: StarDescriptor = matches[0]
            else:
                rejection_count += 1
                continue
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

    logging.info("-> Total of {:,} stars in database.".format(len(star_data)))
    logging.info("-> Rejected {:,} / {:,} new catalogue entries.".format(rejection_count, total_count))


def read_from_gaia_edr3(star_data: StarList) -> None:
    """
    Read stars from Gaia EDR 3 Catalogue.

    :param star_data:
        The list of stars we merge new entries into.
    :return:
        None
    """
    # Read data from Gaia EDR3
    logging.info("Reading Gaia EDR3 catalogue...")
    source: str = "gaia_edr3"
    rejection_count: int = 0
    total_count: int = 0
    new_entry_count: int = 0
    edr3_path: Final[str] = os.path.join(input_path, "gaiaEDR3/*.csv.gz")
    for filename in glob.glob(edr3_path):
        columns: Optional[List[str]] = None  # Read column headings from first line of CSV file
        file_item_count: int = 0
        for line in gzip.open(filename, "rt"):
            hipparcos_num: Optional[int] = None
            parallax: Optional[float] = None
            parallax_error: Optional[float] = None
            proper_motion: Optional[float] = None
            proper_motion_pa: Optional[float] = None

            # First line contains column headings
            if columns is None:
                columns = line.strip().split(",")
                continue

            # Read data lines
            file_item_count += 1
            total_count += 1
            words = line.strip().split(",")
            try:
                ra: float = float(words[columns.index("ra")])
                decl: float = float(words[columns.index("dec")])
                g_mag: float = float(words[columns.index("phot_g_mean_mag")])
                edr3_id: Optional[str] = words[columns.index("source_id")]
                tycho_id: Optional[str] = words[columns.index("tycho")]
            except ValueError:
                # Ignore lines with missing fields
                continue

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
                pm_ra: float = float(words[columns.index("pmra")])
                pm_dec: float = float(words[columns.index("pmdec")])
                proper_motion: Optional[float] = hypot(pm_ra, pm_dec)
                proper_motion_pa: Optional[float] = (atan2(pm_ra, pm_dec) * 180 / pi + 720) % 360
            except ValueError:
                proper_motion = None
                proper_motion_pa = None

            # Do not create new stars brighter than mag 10.5 from Gaia data
            if (not tycho_id) and (not hipparcos_num) and (g_mag < 10.5):
                # logging.info("Rejecting Gaia EDR3 star: could not read either HIP or TYC identifier.")
                rejection_count += 1
                continue

            # Fetch star descriptor
            star = star_data.match_star(
                ra=ra, decl=decl, mag=g_mag,
                source=source, match_threshold=0.01,
                hipparcos_num=hipparcos_num, tycho_id=tycho_id, edr3_id=edr3_id)

            # Make logging entry if we created a new entry
            new_entry: bool = (star.id is None)

            # Record this star's position and parallax
            star.in_gaia_edr3 = True
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
            try:
                mag = float(words[columns.index("phot_g_mean_mag")])
                if mag:
                    star.mag_G = mag
                    star.mag_G_source = source
            except ValueError:
                pass

            try:
                mag = float(words[columns.index("phot_bp_mean_mag")])
                if mag:
                    star.mag_BP = mag
                    star.mag_BP_source = source
            except ValueError:
                pass

            try:
                mag = float(words[columns.index("phot_rp_mean_mag")])
                if mag:
                    star.mag_RP = mag
                    star.mag_RP_source = source
            except ValueError:
                pass

            # Ensure this star is recorded in the output catalogue
            star_data.add_star(star_descriptor=star)

            # Make logging entry if we created a new entry
            if new_entry:
                new_entry_count += 1
                if star.names_tycho_id:
                    # logging.info("Created new entry ({:d}) for Gaia EDR3 star TYC {:s}".format(
                    #     star.id, star.names_tycho_id))
                    pass
                elif star.names_hip_num:
                    logging.info("Created new entry ({:d}) for Gaia EDR3 star HIP {:d}".format(
                        star.id, star.names_hip_num))

        # Report how many stars we found in this file
        logging.info("Finished reading file <{}>. Found {:,} stars.".format(
            os.path.split(filename)[1], file_item_count))

    logging.info("-> Total of {:,} stars in database.".format(len(star_data)))
    logging.info("-> Rejected {:,} / {:,} new catalogue entries.".format(rejection_count, total_count))


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
    logging.info("-> Total of {:,} stars in database.".format(len(star_data)))
    logging.info("-> Total of {:,} Bayer letters in database.".format(star_data.count_stars_with_bayer_letters()))
    logging.info("-> Total of {:,} Flamsteed numbers in database.".format(
        star_data.count_stars_with_flamsteed_numbers()))
    logging.info("Reading Cross Index catalogue...")

    rejection_count: int = 0
    total_count: int = 0
    catalog_path: Final[str] = os.path.join(input_path, "bayerAndFlamsteed/catalog.dat")
    with open(catalog_path, "rt") as f_in:
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
            if hd_number is not None:
                matches: List[StarDescriptor] = list(star_data.lookup_by_hd_num(hd_num=hd_number))
                if len(matches) > 0:
                    star: StarDescriptor = matches[0]
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

            # Update star in the database
            if star is not None:
                star_data.add_star(star_descriptor=star)

    logging.info("-> Total of {:,} Bayer letters in database.".format(star_data.count_stars_with_bayer_letters()))
    logging.info("-> Total of {:,} Flamsteed numbers in database.".format(
        star_data.count_stars_with_flamsteed_numbers()))
    logging.info("-> Total of {:,} stars in database.".format(len(star_data)))
    logging.info("-> Rejected {:,} / {:,} new catalogue entries.".format(rejection_count, total_count))


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
    star_names_path: Final[str] = os.path.join(input_path, "brightStars/starNames_iau.txt")
    with open(star_names_path, "rt") as f_in:
        for line in f_in:
            if (len(line) < 5) or (line[0] == '#'):
                continue
            words: Sequence[str] = line.split()
            cat_name: str = words[0]
            cat_number: int = int(words[1])
            parallax: float = float(words[2])
            name: str = line[20:]
            star: Optional[StarDescriptor] = None
            if cat_name == "HR":
                matches: List[StarDescriptor] = list(star_data.lookup_by_bs_num(bs_num=cat_number))
                if len(matches) > 0:
                    star: StarDescriptor = matches[0]
            elif cat_name == "HD":
                matches: List[StarDescriptor] = list(star_data.lookup_by_hd_num(hd_num=cat_number))
                if len(matches) > 0:
                    star: StarDescriptor = matches[0]
            elif cat_name == "HIP":
                matches: List[StarDescriptor] = list(star_data.lookup_by_hip_num(hip_num=cat_number))
                if len(matches) > 0:
                    star: StarDescriptor = matches[0]
            if star is None:
                logging.info("  Error - could not find star {} {}".format(cat_name, cat_number))
                continue

            # Add new English name for this star
            star.add_english_name(new_name=name.strip(), prepend=True)

            # Add new manually-set parallax, if available
            if parallax:
                star.source_par = 'manual'
                star.parallax = parallax

            # Update star
            star_data.add_star(star)


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
    read_from_gaia_edr3(star_data=star_data)
    read_bayer_flamsteed_cross_index(star_data=star_data)
    read_english_names(star_data=star_data)

    # Create a histogram of the brightness of all the stars we have inserted into catalogue
    bin_size: float = 0.25
    bin_min: float = -2
    bin_max: float = 20

    logging.info("-> Total of {:d} stars with valid magnitudes.".format(
        star_data.star_db.count_stars_with_valid_magnitudes()))

    # Display a brightness histogram as a table
    logging.info("Brightness histogram:")
    total_stars: int = 0
    for bin_pos in np.arange(bin_min, bin_max, bin_size):
        val: int = star_data.star_db.count_stars_in_magnitude_range(mag_min=bin_pos, mag_max=bin_pos + bin_size)
        total_stars += val
        logging.info("* Mag {:6.2f} to {:6.2f} -- {:9,} stars (total {:11,} stars)".format(
            bin_pos, bin_pos + bin_size, val, total_stars))

    # Produce output catalogues
    logging.info("Producing output data catalogues")

    # Make sure that StarCharter regenerates its binary file
    star_charter_output: Final[str] = os.path.join(output_path, "star_charter_stars.dat.gz")
    all_stars_output: Final[str] = os.path.join(output_path, "all_stars.json.gz")
    os.system("rm -f '{}' '{}' '{}/*.bin'".format(star_charter_output, all_stars_output, output_path))

    # Write output as a text file for use in StarCharter
    with gzip.open(star_charter_output, "wt") as output_star_charter:

        # Also write output as a big lump of JSON which other Python scripts can use
        with gzip.open(all_stars_output, "wt") as output_json:

            # Show progress
            progress_total: int = len(star_data)
            progress_steps: int = 20
            last_display: str = ""

            # Loop over all the objects we've catalogued
            for counter, star in enumerate(star_data.star_db.search(order_by='mag', sort_direction='asc')):
                # Progress update
                if counter % 20 == 0:
                    progress_str: str = "Writing output... {:3.0f}% done".format(
                        (floor(counter / progress_total * progress_steps) / progress_steps * 100))
                    if progress_str != last_display:
                        logging.info(progress_str)
                        last_display = progress_str
                # Some variables we feed data into
                name_flamsteed_num = 0
                name_const = name_bayer_letter = ""
                names_english = names_catalogue_ref = []

                # name_1 is the star's Bayer letter, as a UTF8 character
                # name_2 is 'x-c' where x is name_1 and c is the constellation
                # name_3 is the star's English name, which spaces rendered as underscores
                # name_4 is the star's catalogue name, e.g. "V337_Car"
                # name_5 is 'y' where y is the Flamsteed number
                name_1 = name_2 = name_3 = name_4 = name_5 = "-"

                # Populate variables with data from the StarDescriptor structure
                ra: float = star.ra if star.ra is not None else np.nan
                decl: float = star.decl if star.decl is not None else np.nan
                source_pos: str = star.source_pos if star.source_pos is not None else ""
                mag_list: dict = star.magnitude_dictionary()
                parallax: float = star.parallax if star.parallax is not None else np.nan
                dist: float = star.dist if star.dist is not None else np.nan
                source_par: str = star.source_par if star.source_par is not None else ""
                proper_motion: float = star.proper_motion if star.proper_motion is not None else np.nan
                proper_motion_pa: float = star.proper_motion_pa if star.proper_motion_pa is not None else np.nan
                mag_bv: float = star.color_bv if star.color_bv is not None else np.nan
                variable: int = 1 if star.is_variable else 0

                # Catalogue numbers for this star
                ybsn_num: int = star.names_bs_num if star.names_bs_num is not None else 0
                hd_num: int = star.names_hd_num if star.names_hd_num is not None else 0
                nsv: int = star.names_nsv_num if star.names_nsv_num is not None else 0
                hip_num: int = star.names_hip_num if star.names_hip_num is not None else 0
                tycho_id: str = star.names_tycho_id if star.names_tycho_id is not None else ""
                edr3_id: str = star.names_edr3_id if star.names_edr3_id is not None else ""

                # Something dodgy has happened if this star doesn't have any catalogue identifications
                if (not ybsn_num) and (not hd_num) and (not hip_num) and (not tycho_id) and (not edr3_id):
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
                        name_bayer_letter: str = star.names_bayer_letter
                        name_1: str = star_name_converter.greek_html_to_utf8(
                            in_str=star_name_converter.name_to_html(in_str=name_bayer_letter)
                        )
                        name_2: str = "{}-{}".format(name_1, star.names_const)
                    if star.names_flamsteed_number:
                        name_5: str = "{}".format(star.names_flamsteed_number)
                        name_flamsteed_num: int = star.names_flamsteed_number

                # Limiting magnitude of 14 for StarCharter
                mag_limit: float = 14

                if (star.mag_reference is not None) and (star.mag_reference < mag_limit):
                    output_star_charter.write(
                        "{:6d} {:6d} {:8d} {:17.12f} {:17.12f} {:17.12f} {:17.12f} {:17.12f} {:17.12f} {:s} {:s} {:s} {:s} {:s}\n"
                        .format(hd_num, ybsn_num, hip_num, ra, decl, star.mag_reference, mag_bv, parallax, dist,
                                name_1, name_2, name_3, name_4, name_5)
                    )

                # Dump absolutely everything into the JSON output
                json_structure = [ra, decl, mag_list, parallax, dist, ybsn_num, hd_num, hip_num, tycho_id,
                                  names_english + names_catalogue_ref, name_const,
                                  star_name_converter.name_to_html(in_str=name_bayer_letter),
                                  star_name_converter.name_to_ascii(in_str=name_bayer_letter),
                                  name_flamsteed_num,
                                  source_pos, source_par,
                                  mag_bv, proper_motion, proper_motion_pa, nsv, variable, edr3_id]
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
