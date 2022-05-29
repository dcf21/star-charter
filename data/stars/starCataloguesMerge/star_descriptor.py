# -*- coding: utf-8 -*-
# star_descriptor_merge.py
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
A data structure for storing data about stars
"""

import logging

from math import hypot, pi, cos
from typing import List, Optional


# Memory efficient data structure used for storing data about stars
class StarDescriptor:
    """
    A memory-efficient data structure used for storing data about stars
    """
    __slots__ = (
        'id',  # integer index in star_data
        'ra',  # degrees, J2000
        'decl',  # degrees, J2000
        'color_bv',
        'dist',
        'parallax',
        'source_par',
        'proper_motion',
        'proper_motion_pa',
        'source_pos',
        'mag_V',
        'mag_V_source',
        'mag_BT',
        'mag_BT_source',
        'mag_VT',
        'mag_VT_source',
        'mag_G',
        'mag_G_source',
        'mag_BP',
        'mag_BP_source',
        'mag_RP',
        'mag_RP_source',
        'is_variable',
        'in_ybsc',
        'in_hipp',
        'in_tycho1',
        'in_tycho2',
        'in_gaia_dr1',
        'in_gaia_dr2',
        'is_variable',
        'names_english',  # List of English names for star
        'names_catalogue_ref',  # List of catalogue references for each star (e.g. V337_Car)
        'names_bayer_letter',  # Bayer letter, stored in TeX format for legacy reasons, possibly with superscript number
        'names_const',  # Flamsteed/Bayer three-letter constellation abbreviations, e.g. "Peg"
        'names_flamsteed_number',  # Flamsteed numbers, stored as integers
        'names_hd_num',  # Integer
        'names_bs_num',  # Integer
        'names_nsv_num',  # Integer
        'names_hip_num',  # Integer
        'names_tyc_num',  # String, e.g. "1-2-3"
        'names_dr2_num',  # String
    )

    def __init__(self):
        self.id = None  # type: Optional[int]
        self.ra = None  # type: Optional[float]
        self.decl = None  # type: Optional[float]
        self.color_bv = None  # type: Optional[float]
        self.dist = None  # type: Optional[float]
        self.parallax = None  # type: Optional[float]
        self.source_par = None  # type: Optional[str]
        self.proper_motion = None  # type: Optional[float]
        self.proper_motion_pa = None  # type: Optional[float]
        self.source_pos = None  # type: Optional[str]

        self.in_ybsc = False  # type: bool
        self.in_hipp = False  # type: bool
        self.in_tycho1 = False  # type: bool
        self.in_tycho2 = False  # type: bool
        self.in_gaia_dr1 = False  # type: bool
        self.in_gaia_dr2 = False  # type: bool
        self.is_variable = False  # type: bool

        self.mag_V = None  # type: Optional[float]
        self.mag_V_source = None  # type: Optional[str]
        self.mag_BT = None  # type: Optional[float]
        self.mag_BT_source = None  # type: Optional[str]
        self.mag_VT = None  # type: Optional[float]
        self.mag_VT_source = None  # type: Optional[str]
        self.mag_G = None  # type: Optional[float]
        self.mag_G_source = None  # type: Optional[str]
        self.mag_BP = None  # type: Optional[float]
        self.mag_BP_source = None  # type: Optional[str]
        self.mag_RP = None  # type: Optional[float]
        self.mag_RP_source = None  # type: Optional[str]

        self.names_english = None  # type: Optional[List[str]]
        self.names_catalogue_ref = None  # type: Optional[List[str]]
        self.names_bayer_letter = None  # type: Optional[str]
        self.names_const = None  # type: Optional[str]
        self.names_flamsteed_number = None  # type: Optional[int]
        self.names_hd_num = None  # type: Optional[int]
        self.names_bs_num = None  # type: Optional[int]
        self.names_nsv_num = None  # type: Optional[int]
        self.names_hip_num = None  # type: Optional[int]
        self.names_tyc_num = None  # type: Optional[str]
        self.names_dr2_num = None  # type: Optional[str]

    def add_english_name(self, new_name: str, prepend: bool = False):
        """
        Add a new friendly name for a star, e.g. Sirius

        :param new_name:
            The name for the star
        :param prepend:
            If true, add new name to the start of the list, not the end
        :return:
            None
        """
        if self.names_english is None:
            self.names_english = []
        if prepend:
            self.names_english.insert(0, new_name)
        else:
            self.names_english.append(new_name)

    def add_catalogue_name(self, new_name: str):
        """
        Add a new catalogue-based name for a star, e.g. V243 Cas
        :param new_name:
            The name for the star
        :return:
            None
        """
        if self.names_catalogue_ref is None:
            self.names_catalogue_ref = []
        self.names_catalogue_ref.append(new_name)

    def magnitude_dictionary(self):
        """
        Return a dictionary of all available magnitudes for this star.

        :return:
            dict
        """
        output = {}
        if self.mag_V is not None:
            output['V'] = {'value': self.mag_V, 'source': self.mag_V_source}
        if self.mag_BT is not None:
            output['BT'] = {'value': self.mag_BT, 'source': self.mag_BT_source}
        if self.mag_VT is not None:
            output['VT'] = {'value': self.mag_VT, 'source': self.mag_VT_source}
        if self.mag_G is not None:
            output['G'] = {'value': self.mag_G, 'source': self.mag_G_source}
        if self.mag_BP is not None:
            output['BP'] = {'value': self.mag_BP, 'source': self.mag_BP_source}
        if self.mag_RP is not None:
            output['RP'] = {'value': self.mag_RP, 'source': self.mag_RP_source}
        return output

    def generate_catalogue_name(self):
        """
        Generate a human-readable name for this star.

        :return:
            str
        """
        if self.names_hip_num:
            return "HIP {:d}".format(self.names_hip_num)
        if self.names_tyc_num:
            return "TYC {:s}".format(self.names_tyc_num)
        return "No name"

    def check_positions_agree(self, ra_new: float, dec_new: float, mag: float, cat_name_new: str, threshold: float):
        """
        Check that a new position for an object on the sky roughly matches its previously reported position.

        :param ra_new:
            New RA, degrees
        :param dec_new:
            New declination, degrees
        :param mag:
            Magnitude of object
        :param cat_name_new:
            Catalogue from which the new position was taken
        :param threshold:
            Threshold amount we're allowed to move star's position without a warning (degrees)
        :return:
            None
        """
        ra_diff = ra_new - self.ra
        dec_diff = dec_new - self.decl
        ang_change = hypot(dec_diff, ra_diff * cos(ra_new * pi / 180))
        if ang_change > threshold:
            logging.info("Warning: moving mag {:4.1f} star {} by {:.3f} deg ({} --> {})"
                         .format(mag, self.generate_catalogue_name(), ang_change, self.source_pos, cat_name_new)
                         )


# A list of StarDescriptor objects
class StarList:
    """
    A class which contains a list of all the stars in our star catalogue
    """

    def __init__(self):
        self.star_list = []  # type: list[StarDescriptor]

        # Look up the UID of a star by its HD catalogue number, bright star number, HIP number, etc
        self.lookup_by_hd_num = {}  # type: dict[int, int]
        self.lookup_by_bs_num = {}  # type: dict[int, int]
        self.lookup_by_hip_num = {}  # type: dict[int, int]
        self.lookup_by_tyc_num = {}  # type: dict[str, int]
        self.lookup_by_dr2_num = {}  # type: dict[str, int]

    def __len__(self):
        """
        Count the total number of stars in the list of stars

        :return:
            int
        """
        return len(self.star_list)

    def count_stars_with_bayer_letters(self):
        """
        Count the number of stars which have valid Bayer letters

        :return:
            int
        """
        count = 0
        for item in self.star_list:
            if item.names_bayer_letter is not None:
                count += 1
        return count

    def count_stars_with_flamsteed_numbers(self):
        """
        Count the number of stars which have valid Flamsteed numbers

        :return:
            int
        """
        count = 0
        for item in self.star_list:
            if item.names_flamsteed_number is not None:
                count += 1
        return count

    def add_star(self, star_descriptor: StarDescriptor):
        """
        Add a star to the list of stars we are collecting. Do not add it a second time, if we have already added it.

        :param star_descriptor:
            The descriptor of the star we are to add
        :return:
            None
        """
        # Do not add star if it already has an ID
        if star_descriptor.id is None:
            # Create a new ID for this star
            new_star_id = len(self.star_list)
            star_descriptor.id = new_star_id
            self.star_list.append(star_descriptor)
        else:
            new_star_id = star_descriptor.id

        # Update lookup tables
        if star_descriptor.names_hd_num is not None and star_descriptor.names_hd_num > 0:
            self.lookup_by_hd_num[star_descriptor.names_hd_num] = new_star_id
        if star_descriptor.names_bs_num is not None and star_descriptor.names_bs_num > 0:
            self.lookup_by_bs_num[star_descriptor.names_bs_num] = new_star_id
        if star_descriptor.names_hip_num is not None and star_descriptor.names_hip_num > 0:
            self.lookup_by_hip_num[star_descriptor.names_hip_num] = new_star_id
        if star_descriptor.names_tyc_num is not None and len(star_descriptor.names_tyc_num) > 0:
            self.lookup_by_tyc_num[star_descriptor.names_tyc_num] = new_star_id
        if star_descriptor.names_dr2_num is not None and len(star_descriptor.names_dr2_num) > 0:
            self.lookup_by_dr2_num[star_descriptor.names_dr2_num] = new_star_id

    def match_star(self, ra: float, decl: float, mag: float, source: str, match_threshold: float = 0.01,
                   hd_number: Optional[int] = None,
                   hipparcos_num: Optional[int] = None,
                   tycho_id: Optional[str] = None):
        """
        Check whether this star already exists in the list (matched by catalogue number). If it does, return the
        descriptor of the star we matched. Otherwise return a new, clean, star descriptor.

        :param ra:
            Right ascension of the star, degrees J2000
        :param decl:
            Declination of the star, degrees J2000
        :param mag:
            Magnitude of the star (approx)
        :param source:
            The name of the catalogue this star is being entered from
        :param match_threshold:
            The angular distance threshold at which we issue a warning if this star has moved too far
        :param hd_number:
            HD catalogue number for this star
        :param hipparcos_num:
            Hipparcos catalogue number for this star
        :param tycho_id:
            Tycho ID for this star
        :return:
            StarDescriptor, bool, bool
        """
        # Store whether we rejected a match of this star by HD or Hipparcos number
        hd_mismatch = False
        hipparcos_mismatch = False

        # Holder for the star we have matched
        star = None  # type: Optional[StarDescriptor]

        # See if we can match this star to a known Tycho entry.
        if tycho_id is not None and tycho_id in self.lookup_by_tyc_num:
            uid = self.lookup_by_tyc_num[tycho_id]
            star = self.star_list[uid]

        # See if we can match this star to a known HIP entry.
        # Do not match if HIP entry is already matched to another star
        if star is None:
            if hipparcos_num is not None and hipparcos_num in self.lookup_by_hip_num:
                uid = self.lookup_by_hip_num[hipparcos_num]
                star = self.star_list[uid]
                if (
                        (star.names_tyc_num is not None) and
                        (len(star.names_tyc_num) > 0) and
                        (tycho_id is not None) and
                        (len(tycho_id) > 0) and
                        (star.names_tyc_num != tycho_id)
                ):
                    star = None
                    hipparcos_mismatch = True

        # See if we can match this star to a known HD entry.
        # Do not match if HD entry is already matched to another star
        if star is None:
            if hd_number is not None and hd_number in self.lookup_by_hd_num:
                uid = self.lookup_by_hd_num[hd_number]
                star = self.star_list[uid]
                if (
                        (
                                (star.names_hip_num is not None) and
                                (star.names_hip_num > 0) and
                                (hipparcos_num is not None) and
                                (hipparcos_num > 0) and
                                (star.names_hip_num != hipparcos_num)
                        ) or
                        (
                                (star.names_tyc_num is not None) and
                                (len(star.names_tyc_num) > 0) and
                                (tycho_id is not None) and
                                (len(tycho_id) > 0) and
                                (star.names_tyc_num != tycho_id)
                        )
                ):
                    star = None
                    hd_mismatch = True

        # If matching unsuccessful, create a new entry
        if star is None:
            star = StarDescriptor()

        # If we are matching a pre-existing star, check we are not moving it too far
        if star.ra is not None:
            star.check_positions_agree(ra_new=ra, dec_new=decl, mag=mag, cat_name_new=source,
                                       threshold=match_threshold)

        return star, hd_mismatch, hipparcos_mismatch
