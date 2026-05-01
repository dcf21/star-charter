# -*- coding: utf-8 -*-
# star_descriptor_merge.py
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
A data structure for storing data about stars
"""

import logging

from math import hypot, pi, cos
from typing import Dict, List, Optional, Tuple, Union


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
        'names_tycho_id',  # String, e.g. "1-2-3"
        'names_dr2_num',  # String
    )

    def __init__(self):
        self.id: Optional[int] = None
        self.ra: Optional[float] = None
        self.decl: Optional[float] = None
        self.color_bv: Optional[float] = None
        self.dist: Optional[float] = None
        self.parallax: Optional[float] = None
        self.source_par: Optional[str] = None
        self.proper_motion: Optional[float] = None
        self.proper_motion_pa: Optional[float] = None
        self.source_pos: Optional[str] = None

        self.in_ybsc: bool = False
        self.in_hipp: bool = False
        self.in_tycho1: bool = False
        self.in_tycho2: bool = False
        self.in_gaia_dr1: bool = False
        self.in_gaia_dr2: bool = False
        self.is_variable: bool = False

        self.mag_V: Optional[float] = None
        self.mag_V_source: Optional[str] = None
        self.mag_BT: Optional[float] = None
        self.mag_BT_source: Optional[str] = None
        self.mag_VT: Optional[float] = None
        self.mag_VT_source: Optional[str] = None
        self.mag_G: Optional[float] = None
        self.mag_G_source: Optional[str] = None
        self.mag_BP: Optional[float] = None
        self.mag_BP_source: Optional[str] = None
        self.mag_RP: Optional[float] = None
        self.mag_RP_source: Optional[str] = None

        self.names_english: Optional[List[str]] = None
        self.names_catalogue_ref: Optional[List[str]] = None
        self.names_bayer_letter: Optional[str] = None
        self.names_const: Optional[str] = None
        self.names_flamsteed_number: Optional[int] = None
        self.names_hd_num: Optional[int] = None
        self.names_bs_num: Optional[int] = None
        self.names_nsv_num: Optional[int] = None
        self.names_hip_num: Optional[int] = None
        self.names_tycho_id: Optional[str] = None
        self.names_dr2_num: Optional[str] = None

    def add_english_name(self, new_name: str, prepend: bool = False) -> None:
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

    def add_catalogue_name(self, new_name: str) -> None:
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

    def magnitude_dictionary(self) -> Dict[str, Dict[str, Union[float, Optional[str]]]]:
        """
        Return a dictionary of all available magnitudes for this star.

        :return:
            dict
        """
        output: Dict[str, Dict[str, Union[float, Optional[str]]]] = {}
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

    def generate_catalogue_name(self) -> str:
        """
        Generate a human-readable name for this star.

        :return:
            str
        """
        if self.names_hip_num:
            return "HIP {:d}".format(self.names_hip_num)
        if self.names_tycho_id:
            return "TYC {:s}".format(self.names_tycho_id)
        return "No name"

    def check_positions_agree(self, ra_new: float, dec_new: float, mag: float, cat_name_new: str, threshold: float) \
            -> None:
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
        ra_diff: float = ra_new - self.ra
        dec_diff: float = dec_new - self.decl
        ang_change: float = hypot(dec_diff, ra_diff * cos(ra_new * pi / 180))
        if ang_change > threshold:
            logging.info("Warning: moving mag {:4.1f} star {} by {:.3f} deg ({} --> {})"
                         .format(mag, self.generate_catalogue_name(), ang_change, self.source_pos, cat_name_new)
                         )


# A list of StarDescriptor objects
class StarList:
    """
    A class which contains a list of all the stars in our star catalogue
    """

    def __init__(self, magnitude_limit: Optional[float] = None):
        # Store the requested magnitude cut-off for this catalogue
        self.magnitude_limit: Optional[float] = magnitude_limit

        # Storage for the list of stars
        self.star_list: List[StarDescriptor] = []

        # Look up the UID of a star by its HD catalogue number, bright star number, HIP number, etc
        self.lookup_by_hd_num: Dict[int, int] = {}
        self.lookup_by_bs_num: Dict[int, int] = {}
        self.lookup_by_hip_num: Dict[int, int] = {}
        self.lookup_by_tycho_id: Dict[str, int] = {}
        self.lookup_by_dr2_id: Dict[str, int] = {}

    def __len__(self) -> int:
        """
        Count the total number of stars in the list of stars

        :return:
            int
        """
        return len(self.star_list)

    def count_stars_with_bayer_letters(self) -> int:
        """
        Count the number of stars which have valid Bayer letters

        :return:
            int
        """
        count: int = 0
        for item in self.star_list:
            if item.names_bayer_letter is not None:
                count += 1
        return count

    def count_stars_with_flamsteed_numbers(self) -> int:
        """
        Count the number of stars which have valid Flamsteed numbers

        :return:
            int
        """
        count: int = 0
        for item in self.star_list:
            if item.names_flamsteed_number is not None:
                count += 1
        return count

    def add_star(self, star_descriptor: StarDescriptor) -> None:
        """
        Add a star to the list of stars we are collecting. Do not add it a second time, if we have already added it.

        :param star_descriptor:
            The descriptor of the star we are to add
        :return:
            None
        """
        # Do not add star if it already has an ID
        if star_descriptor.id is None:
            # Check that this star satisfies the requested magnitude cut-off
            if self.magnitude_limit is not None:
                accept_star: bool = False
                x: StarDescriptor = star_descriptor
                for item in (x.mag_V, x.mag_G, x.mag_VT, x.mag_BT, x.mag_BP, x.mag_RP):
                    if item is not None and item <= self.magnitude_limit:
                        accept_star = True
                        break
                if not accept_star:
                    return

            # Create a new ID for this star
            new_star_id: int = len(self.star_list)
            star_descriptor.id = new_star_id
            self.star_list.append(star_descriptor)
        else:
            new_star_id = star_descriptor.id

        # Update lookup tables
        for cat_id, cat_index in (
                (star_descriptor.names_hd_num, self.lookup_by_hd_num),
                (star_descriptor.names_bs_num, self.lookup_by_bs_num),
                (star_descriptor.names_hip_num, self.lookup_by_hip_num),
                (star_descriptor.names_tycho_id, self.lookup_by_tycho_id),
                (star_descriptor.names_dr2_num, self.lookup_by_dr2_id)
        ):
            if cat_id is not None and cat_id:
                # Check that this catalogue ID doesn't refer to a different star
                assert cat_index.get(cat_id, new_star_id) == new_star_id
                # Create index entry for this catalogue ID
                cat_index[cat_id] = new_star_id

    def match_star(self, ra: float, decl: float, mag: float, source: str, match_threshold: float = 0.01,
                   bs_number: Optional[int] = None, hd_number: Optional[int] = None,
                   hipparcos_num: Optional[int] = None, tycho_id: Optional[str] = None, dr2_id: Optional[str] = None) \
            -> Tuple[StarDescriptor, Dict[str, bool]]:
        """
        Check whether this star already exists in the list (matched by catalogue number). If it does, return the
        descriptor of the star we matched. Otherwise, return a new, clean, star descriptor.

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
        :param bs_number:
            Bright star catalogue number for this star
        :param hd_number:
            HD catalogue number for this star
        :param hipparcos_num:
            Hipparcos catalogue number for this star
        :param tycho_id:
            Tycho ID for this star
        :param dr2_id:
            Gaia DR2 ID for this star
        :return:
            StarDescriptor, dictionary of IDs to delete (they match other brighter stars)
        """
        # Store whether we rejected a match of this star by HD or Hipparcos number
        ids_for_deletion: Dict[str, bool] = {}

        # Holder for the star we have matched
        star: Optional[StarDescriptor] = None

        # See if we can match this star to a known Gaia DR2 entry.
        if dr2_id is not None and dr2_id in self.lookup_by_dr2_id:
            uid: int = self.lookup_by_dr2_id[dr2_id]
            star: Optional[StarDescriptor] = self.star_list[uid]

        # See if we can match this star to a known Tycho entry.
        if tycho_id is not None and tycho_id in self.lookup_by_tycho_id:
            uid: int = self.lookup_by_tycho_id[tycho_id]
            star: Optional[StarDescriptor] = self.star_list[uid]

        # See if we can match this star to a known HIP entry.
        # Do not match if HIP entry is already matched to another star
        if star is None:
            if hipparcos_num is not None and hipparcos_num in self.lookup_by_hip_num:
                uid: int = self.lookup_by_hip_num[hipparcos_num]
                star: Optional[StarDescriptor] = self.star_list[uid]
                # If this HIP ID already matches a record with a different Tycho ID, we need to create a new
                # record for this Tycho ID, so discard match.
                if (
                        (star.names_tycho_id is not None) and
                        (len(star.names_tycho_id) > 0) and
                        (tycho_id is not None) and
                        (len(tycho_id) > 0) and
                        (star.names_tycho_id != tycho_id)
                ):
                    star = None

        # See if we can match this star to a known HD entry.
        # Do not match if HD entry is already matched to another star
        if star is None:
            if hd_number is not None and hd_number in self.lookup_by_hd_num:
                uid = self.lookup_by_hd_num[hd_number]
                star = self.star_list[uid]
                # If this HD ID already matches a record with a different Tycho ID, we need to create a new
                # record for this Tycho ID, so discard match. Likewise with HIP IDs.
                if (
                        (
                                (star.names_hip_num is not None) and
                                (star.names_hip_num > 0) and
                                (hipparcos_num is not None) and
                                (hipparcos_num > 0) and
                                (star.names_hip_num != hipparcos_num)
                        ) or
                        (
                                (star.names_tycho_id is not None) and
                                (len(star.names_tycho_id) > 0) and
                                (tycho_id is not None) and
                                (len(tycho_id) > 0) and
                                (star.names_tycho_id != tycho_id)
                        )
                ):
                    star = None

        # If matching unsuccessful, create a new entry
        if star is None:
            star = StarDescriptor()

        # If this star has any catalogue IDs that are already assigned to other stars, then the ID goes to the
        # star with the brighter V magnitude
        star_id: int = star.id

        for cat_id, cat_index, cat_name in (
                (hd_number, self.lookup_by_hd_num, 'hd'),
                (bs_number, self.lookup_by_bs_num, 'bs'),
                (hipparcos_num, self.lookup_by_hip_num, 'hip'),
                (tycho_id, self.lookup_by_tycho_id, 'tyc'),
                (dr2_id, self.lookup_by_dr2_id, 'dr2')
        ):
            if cat_id is not None and cat_id:
                if cat_index.get(cat_id, star_id) != star_id:
                    other_id: int = cat_index[cat_id]
                    other: StarDescriptor = self.star_list[other_id]
                    other_brighter: bool = other.mag_V is not None and other.mag_V < mag
                    if other_brighter:
                        ids_for_deletion[cat_name] = True
                    else:
                        ids_for_deletion[cat_name] = False
                        del cat_index[cat_id]
                        if cat_name == 'hd':
                            other.names_hd_num = None
                        elif cat_name == 'bs':
                            other.names_bs_num = None
                        elif cat_name == 'hip':
                            other.names_hip_num = None
                        elif cat_name == 'tyc':
                            other.names_tycho_id = None
                        elif cat_name == 'dr2':
                            other.names_dr2_num = None

        # If we are matching a pre-existing star, check we are not moving it too far
        if star.ra is not None:
            star.check_positions_agree(ra_new=ra, dec_new=decl, mag=mag, cat_name_new=source,
                                       threshold=match_threshold)

        return star, ids_for_deletion
