# -*- coding: utf-8 -*-
# star_descriptor_merge.py
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
A data structure for storing data about stars
"""

import json
import logging
import os
import sqlite3

from contextlib import closing
from math import acos, cos, isfinite, pi, sin
from typing import Dict, Final, Iterator, List, Optional, Tuple, Union

import numpy as np

from .temporary_directory import TemporaryDirectory

# Constants used for units conversions
AU: Final[float] = 1.49598e11  # metres
LYR: Final[float] = 9.4605284e15  # lightyear in metres

# List of catalogue references of stars where we want very verbose debugging
debug_targets: Final[List[str]] = ["HR 4730", "HR 4731"]
# debug_targets: Final[List[str]] = ["HD 105963", "HIP 59432", "TYC 3834-1089-1"]
# debug_targets: Final[List[str]] = ["HD 12447", "TYC 40-1338-2"]
debug_all_targets: Final[bool] = False


class StarDescriptor:
    """
    A data structure used for storing data about an individual star
    """

    def __init__(self):
        self.id: Optional[int] = None
        self.db_path: Optional[str] = None
        self.child_of: Optional[int] = None
        self.is_parent: Optional[bool] = None

        self.ra: Optional[float] = None  # degrees, J2000.0
        self.decl: Optional[float] = None  # degrees, J2000.0
        self.mag_reference: Optional[float] = None
        self.is_variable: bool = False
        self.color_bv: Optional[float] = None
        self.dist: Optional[float] = None  # lightyears
        self.parallax: Optional[float] = None  # mas
        self.source_par: Optional[str] = None
        self.proper_motion: Optional[float] = None  # mas/year
        self.proper_motion_pa: Optional[float] = None  # degrees east of north
        self.source_pos: Optional[str] = None

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
        self.names_edr3_id: Optional[str] = None

    def __str__(self) -> str:
        info: List[str] = ["ID {}".format(self.id)]
        info.extend(self.all_catalogue_names())
        if self.child_of is not None and self.child_of > 0:
            info.append("child of {:d}".format(self.child_of))
        if self.is_parent:
            info.append("parent")
        return "<<{}>>".format("; ".join(info))

    def __repr__(self) -> str:
        return str(self)

    def __eq__(self, other):
        if not isinstance(other, StarDescriptor):
            return False
        if self.id is not None:
            return (self.id == other.id) and (self.db_path == other.db_path)
        return self.all_catalogue_names() == other.all_catalogue_names()

    def emit_debugging(self) -> bool:
        """
        Check whether verbose debugging has been requested for this star.
        """

        if debug_all_targets:
            return True
        if len(debug_targets) == 0:
            return False

        debug_target: str
        self_catalogue_names: List[str] = self.all_catalogue_names()
        for debug_target in debug_targets:
            if debug_target in self_catalogue_names:
                return True
        return False

    def copy(self, copy_id: bool = False) -> "StarDescriptor":
        """
        Duplicate a star descriptor.

        :param copy_id:
            If True, we duplicate the ID of the original star. If False, the duplicate has no ID assigned.
        """

        output: StarDescriptor = StarDescriptor()

        if copy_id:
            output.id = self.id
            output.db_path = self.db_path
        output.child_of = self.child_of
        output.is_parent = self.is_parent
        output.ra = self.ra
        output.decl = self.decl
        output.mag_reference = self.mag_reference
        output.is_variable = self.is_variable
        output.color_bv = self.color_bv
        output.dist = self.dist
        output.parallax = self.parallax
        output.source_par = self.source_par
        output.proper_motion = self.proper_motion
        output.proper_motion_pa = self.proper_motion_pa
        output.source_pos = self.source_pos

        output.mag_V = self.mag_V
        output.mag_V_source = self.mag_V_source
        output.mag_BT = self.mag_BT
        output.mag_BT_source = self.mag_BT_source
        output.mag_VT = self.mag_VT
        output.mag_VT_source = self.mag_VT_source
        output.mag_G = self.mag_G
        output.mag_G_source = self.mag_G_source
        output.mag_BP = self.mag_BP
        output.mag_BP_source = self.mag_BP_source
        output.mag_RP = self.mag_RP
        output.mag_RP_source = self.mag_RP_source

        output.names_english = self.names_english
        output.names_catalogue_ref = self.names_catalogue_ref
        output.names_bayer_letter = self.names_bayer_letter
        output.names_const = self.names_const
        output.names_flamsteed_number = self.names_flamsteed_number
        output.names_hd_num = self.names_hd_num
        output.names_bs_num = self.names_bs_num
        output.names_nsv_num = self.names_nsv_num
        output.names_hip_num = self.names_hip_num
        output.names_tycho_id = self.names_tycho_id
        output.names_edr3_id = self.names_edr3_id

        return output

    def adopt_id_from(self, old_star: "StarDescriptor") -> "StarDescriptor":
        self.id = old_star.id
        self.db_path = old_star.db_path
        self.child_of = old_star.child_of
        self.is_parent = old_star.is_parent
        return self

    def set_catalogue_name(self, catalogue: str, name) -> "StarDescriptor":
        if catalogue == 'edr3':
            self.names_edr3_id = name
        elif catalogue == 'tyc':
            self.names_tycho_id = name
        elif catalogue == 'hip':
            self.names_hip_num = name
        elif catalogue == 'hd':
            self.names_hd_num = name
        elif catalogue == 'nsv':
            self.names_nsv_num = name
        elif catalogue == 'bs':
            self.names_bs_num = name
        else:
            raise ValueError("Unknown catalogue <{}>".format(catalogue))

        return self

    def get_catalogue_name(self, catalogue: str):
        if catalogue == 'edr3':
            return self.names_edr3_id
        elif catalogue == 'tyc':
            return self.names_tycho_id
        elif catalogue == 'hip':
            return self.names_hip_num
        elif catalogue == 'hd':
            return self.names_hd_num
        elif catalogue == 'nsv':
            return self.names_nsv_num
        elif catalogue == 'bs':
            return self.names_bs_num
        else:
            raise ValueError("Unknown catalogue <{}>".format(catalogue))

    def delete_catalogue_entries(self) -> "StarDescriptor":
        """
        Delete all catalogue entries for a star.
        """

        self.names_edr3_id = None
        self.names_tycho_id = None
        self.names_hip_num = None
        self.names_hd_num = None
        self.names_nsv_num = None
        self.names_bs_num = None
        self.names_bayer_letter = None
        self.names_flamsteed_number = None

        return self

    def merge_new_data(self, new_star: "StarDescriptor", overwrite_id: bool = False) -> None:
        """
        Merge new data from a new star descriptor into this descriptor.

        :param new_star:
            Data structure containing the new data to merge.
        :param overwrite_id:
            If True, we take the ID of <new_star>. If False, we retain our existing ID.
        """

        if overwrite_id:
            self.id = new_star.id
            self.db_path = new_star.db_path
        if new_star.child_of is not None:
            self.child_of = new_star.child_of
        if new_star.ra is not None and isfinite(new_star.ra):
            self.ra = new_star.ra
        if new_star.decl is not None and isfinite(new_star.decl):
            self.decl = new_star.decl
        if new_star.mag_reference is not None and isfinite(new_star.mag_reference):
            self.mag_reference = new_star.mag_reference
        if new_star.is_variable is not None and isfinite(new_star.is_variable):
            self.is_variable = new_star.is_variable
        if new_star.color_bv is not None and isfinite(new_star.color_bv):
            self.color_bv = new_star.color_bv
        if new_star.dist is not None and isfinite(new_star.dist):
            self.dist = new_star.dist
        if new_star.parallax is not None and isfinite(new_star.parallax):
            self.parallax = new_star.parallax
        if new_star.source_par is not None and (len(new_star.source_par) > 0):
            self.source_par = new_star.source_par
        if new_star.proper_motion is not None and isfinite(new_star.proper_motion):
            self.proper_motion = new_star.proper_motion
        if new_star.proper_motion_pa is not None and isfinite(new_star.proper_motion_pa):
            self.proper_motion_pa = new_star.proper_motion_pa
        if new_star.source_pos is not None and (len(new_star.source_pos) > 0):
            self.source_pos = new_star.source_pos
        if new_star.mag_V is not None and isfinite(new_star.mag_V):
            self.mag_V = new_star.mag_V
        if new_star.mag_V_source is not None and (len(new_star.mag_V_source) > 0):
            self.mag_V_source = new_star.mag_V_source
        if new_star.mag_BT is not None and isfinite(new_star.mag_BT):
            self.mag_BT = new_star.mag_BT
        if new_star.mag_BT_source is not None and (len(new_star.mag_BT_source) > 0):
            self.mag_BT_source = new_star.mag_BT_source
        if new_star.mag_VT is not None and isfinite(new_star.mag_VT):
            self.mag_VT = new_star.mag_VT
        if new_star.mag_VT_source is not None and (len(new_star.mag_VT_source) > 0):
            self.mag_VT_source = new_star.mag_VT_source
        if new_star.mag_G is not None and isfinite(new_star.mag_G):
            self.mag_G = new_star.mag_G
        if new_star.mag_G_source is not None and (len(new_star.mag_G_source) > 0):
            self.mag_G_source = new_star.mag_G_source
        if new_star.mag_BP is not None and isfinite(new_star.mag_BP):
            self.mag_BP = new_star.mag_BP
        if new_star.mag_BP_source is not None and (len(new_star.mag_BP_source) > 0):
            self.mag_BP_source = new_star.mag_BP_source
        if new_star.mag_RP is not None and isfinite(new_star.mag_RP):
            self.mag_RP = new_star.mag_RP
        if new_star.mag_RP_source is not None and (len(new_star.mag_RP_source) > 0):
            self.mag_RP_source = new_star.mag_RP_source
        if new_star.names_english is not None and (len(new_star.names_english) > 0):
            self.names_english = new_star.names_english
        if new_star.names_catalogue_ref is not None and (len(new_star.names_catalogue_ref) > 0):
            self.names_catalogue_ref = new_star.names_catalogue_ref
        if new_star.names_bayer_letter is not None and (len(new_star.names_bayer_letter) > 0):
            self.names_bayer_letter = new_star.names_bayer_letter
        if new_star.names_const is not None and (len(new_star.names_const) > 0):
            self.names_const = new_star.names_const
        if new_star.names_flamsteed_number is not None and isfinite(new_star.names_flamsteed_number):
            self.names_flamsteed_number = new_star.names_flamsteed_number
        if (new_star.names_hd_num is not None) and (new_star.names_hd_num > 0):
            if ((self.names_hd_num is not None) and (self.names_hd_num > 0) and
                    (self.names_hd_num != new_star.names_hd_num)):
                logging.warning("Overwriting HD{:d} with HD{:d}".format(self.names_hd_num, new_star.names_hd_num))
            self.names_hd_num = new_star.names_hd_num
        if (new_star.names_bs_num is not None) and (new_star.names_bs_num > 0):
            if ((self.names_bs_num is not None) and (self.names_bs_num > 0) and
                    (self.names_bs_num != new_star.names_bs_num)):
                logging.warning("Overwriting HR{:d} with HR{:d}".format(self.names_bs_num, new_star.names_bs_num))
            self.names_bs_num = new_star.names_bs_num
        if (new_star.names_nsv_num is not None) and (new_star.names_nsv_num > 0):
            if ((self.names_nsv_num is not None) and (self.names_nsv_num > 0) and
                    (self.names_nsv_num != new_star.names_nsv_num)):
                logging.warning("Overwriting NSV{:d} with NSV{:d}".format(self.names_nsv_num, new_star.names_nsv_num))
            self.names_nsv_num = new_star.names_nsv_num
        if (new_star.names_hip_num is not None) and (new_star.names_hip_num > 0):
            if ((self.names_hip_num is not None) and (self.names_hip_num > 0) and
                    (self.names_hip_num != new_star.names_hip_num)):
                logging.warning("Overwriting HIP{:d} with HIP{:d}".format(self.names_hip_num, new_star.names_hip_num))
            self.names_hip_num = new_star.names_hip_num
        if (new_star.names_tycho_id is not None) and (len(new_star.names_tycho_id) > 0):
            if ((self.names_tycho_id is not None) and (len(self.names_tycho_id) > 0) and
                    (self.names_tycho_id != new_star.names_tycho_id)):
                logging.warning("Overwriting TYC-{} with TYC-{}".format(self.names_tycho_id, new_star.names_tycho_id))
            self.names_tycho_id = new_star.names_tycho_id
        if (new_star.names_edr3_id is not None) and (len(new_star.names_edr3_id) > 0):
            if ((self.names_edr3_id is not None) and (len(self.names_edr3_id) > 0) and
                    (self.names_edr3_id != new_star.names_edr3_id)):
                logging.warning("Overwriting EDR3-{} with EDR3-{}".format(self.names_edr3_id, new_star.names_edr3_id))
            self.names_edr3_id = new_star.names_edr3_id

    def has_valid_catalogue_id(self) -> bool:
        """
        Check whether this star has any valid catalogue IDs
        """

        return (bool(self.names_bs_num) or bool(self.names_hd_num) or
                bool(self.names_hip_num) or bool(self.names_tycho_id) or
                bool(self.names_edr3_id))

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
        if self.names_edr3_id:
            return "EDR3 {:s}".format(self.names_edr3_id)
        if self.names_hd_num:
            return "HD {:d}".format(self.names_hd_num)
        if self.names_bs_num:
            return "HR {:d}".format(self.names_bs_num)
        return "No name"

    def all_catalogue_names(self) -> List[str]:
        """
        Generate a human-readable list of all the names for this star.

        :return:
            List[str
        """

        names: List[str] = []

        if self.names_bs_num:
            names.append("HR {:d}".format(self.names_bs_num))
        if self.names_nsv_num:
            names.append("NSV {:d}".format(self.names_nsv_num))
        if self.names_hd_num:
            names.append("HD {:d}".format(self.names_hd_num))
        if self.names_hip_num:
            names.append("HIP {:d}".format(self.names_hip_num))
        if self.names_tycho_id:
            names.append("TYC {:s}".format(self.names_tycho_id))
        if self.names_edr3_id:
            names.append("EDR3 {:s}".format(self.names_edr3_id))
        return names

    def get_english_plus_catalogue_names(self) -> List[str]:
        """
        Return a list of this star's English names, and catalogue names.

        :return:
            List[str]
        """
        output: List[str] = []

        if self.names_english is not None:
            output.extend(self.names_english)
        if self.names_catalogue_ref is not None:
            output.extend(self.names_catalogue_ref)

        return output

    def angular_separation(self, other: "StarDescriptor") -> float:
        """
        Calculate the angular separation of a new point from the object.

        :param other:
            The other object, whose angular separation from us we are calculating.
        :return:
            Angular separation, degrees
        """

        if (
                (self.ra is None) or (not isfinite(self.ra)) or
                (self.decl is None) or (not isfinite(self.decl)) or
                (other.ra is None) or (not isfinite(other.ra)) or
                (other.decl is None) or (not isfinite(other.decl))
        ):
            return np.nan

        deg: Final[float] = pi / 180.
        hours: Final[float] = pi / 12.
        ang_sep: Final[float] = acos(min(max(
            sin(self.decl * deg) * sin(other.decl * deg) +
            cos(self.decl * deg) * cos(other.decl * deg) * cos(
                (other.ra - self.ra) * hours),
            -1.), 1.)) / deg

        return ang_sep

    def check_positions_agree(self, other: "StarDescriptor", cat_name_new: str, threshold: float) \
            -> None:
        """
        Check that a new position for an object on the sky roughly matches its previously reported position.

        :param other:
            The new position for this object.
        :param cat_name_new:
            Catalogue from which the new position was taken
        :param threshold:
            Threshold amount we're allowed to move star's position without a warning (degrees)
        :return:
            None
        """

        ang_sep: Final[float] = self.angular_separation(other=other)
        if ang_sep > threshold:
            logging.info("Warning: moving mag {:4.1f} star {} by {:.3f} deg ({} --> {})".format(
                other.mag_reference, self.generate_catalogue_name(), ang_sep, self.source_pos, cat_name_new
            ))

    def reference_magnitude(self) -> Optional[float]:
        """
        Calculate a reference (V-band) magnitude for a star. If the star has a V-band magnitude, then use that. Otherwise,
        estimate a V-band magnitude from Gaia photometry.

        :return:
            V-band magnitude
        """
        mag_reference: Optional[float] = None
        if self.mag_V is not None:
            mag_reference = self.mag_V
        elif self.mag_G is not None and self.mag_BP is not None and self.mag_RP is not None:
            # https://gea.esac.esa.int/archive/documentation/GDR2/Data_processing/chap_cu5pho/sec_cu5pho_calibr/ssec_cu5pho_PhotTransf.html
            br = self.mag_BP - self.mag_RP
            mag_reference = self.mag_G + 0.01760 + 0.006860 * br + 0.1732 * br * br
        elif self.mag_G is not None:
            mag_reference = self.mag_G + 0.01760

        return mag_reference


def debugging_active() -> bool:
    """
    Return a boolean flag indicating whether any targets are being debugged.
    """
    return debug_all_targets or (len(debug_targets) > 0)


def emit_debugging(star_list: List[StarDescriptor]) -> bool:
    """
    Check whether verbose debugging has been requested for any of a list of stars.
    """

    if len(debug_targets) == 0:
        return False

    star: StarDescriptor
    for star in star_list:
        if star.emit_debugging():
            return True
    return False


# A list of StarDescriptor objects
class StarList:
    """
    A class which wraps a list of stars, stored in an SQLite3 database.
    """

    def __init__(self, magnitude_limit: Optional[float] = None, require_unique_ids: bool = True,
                 db_path: Optional[str] = None, read_only: bool = False):
        """
        A class which wraps a list of stars, stored in an SQLite3 database.

        :param magnitude_limit:
            Do not store stars which are fainter than specified magnitude limit.
        :param require_unique_ids:
            Boolean flag indicating whether SQL schema should require stars to have unique catalogue numbers.
        :param db_path:
            Optional path to SQLite3 database. If None, then a temporary directory is used.
        :param read_only:
            Boolean flag indicating whether to open the star list database in read-only mode.
        """

        # Store the requested magnitude cut-off for this catalogue
        self.magnitude_limit: Optional[float] = magnitude_limit
        self.require_unique_ids: bool = require_unique_ids
        self.read_only: bool = read_only

        # Storage for the database of stars
        self.star_db: StarDatabase = StarDatabase(require_unique_ids=require_unique_ids, db_path=db_path,
                                                  read_only=read_only, clean=not read_only)

    def __len__(self) -> int:
        """
        Count the total number of stars in the list of stars

        :return:
            int
        """
        return len(self.star_db)

    def lookup_by_hd_num(self, hd_num: int):
        return list(self.star_db.search(hd_num=hd_num))

    def lookup_by_bs_num(self, bs_num: int):
        return list(self.star_db.search(bs_num=bs_num))

    def lookup_by_hip_num(self, hip_num: int):
        return list(self.star_db.search(hip_num=hip_num))

    def lookup_by_tycho_id(self, tycho_id: str):
        return list(self.star_db.search(tycho_id=tycho_id))

    def lookup_by_edr3_id(self, edr3_id: str):
        return list(self.star_db.search(edr3_id=edr3_id))

    def count_stars_with_bayer_letters(self) -> int:
        """
        Count the number of stars which have valid Bayer letters

        :return:
            int
        """
        return self.star_db.count_stars_with_bayer_letters()

    def count_stars_with_flamsteed_numbers(self) -> int:
        """
        Count the number of stars which have valid Flamsteed numbers

        :return:
            int
        """
        return self.star_db.count_stars_with_flamsteed_numbers()

    def enforce_unique_catalog_ids(self, new_star: StarDescriptor) -> None:
        """
        Enforce that all the catalogue IDs should be unique after the insertion or update of a new star.

        :param new_star:
            The star that we are about to insert or update.
        :return:
            None
        """

        # Fetch the current ID of the <new_star> only if it is an ID within the same database
        current_id: Optional[int] = None
        if (new_star.db_path == self.star_db.db_path) and (new_star.id is not None):
            current_id = new_star.id

        # If this star has any catalogue IDs that are already assigned to other stars, then the ID goes to the
        # star with the brighter V magnitude
        for cat_id, cat_field, cat_name in (
                (new_star.names_hd_num, 'hd_num', 'hd'),
                (new_star.names_bs_num, 'bs_num', 'bs'),
                (new_star.names_nsv_num, 'nsv_num', 'nsv'),
                (new_star.names_hip_num, 'hip_num', 'hip'),
                (new_star.names_tycho_id, 'tycho_id', 'tyc'),
                (new_star.names_edr3_id, 'edr3_id', 'edr3')
        ):
            if cat_id is not None and cat_id:
                matches = list(self.star_db.search(**{cat_field: cat_id}))
                if len(matches) > 0:
                    other: StarDescriptor = matches[0]
                    if other.id != current_id:
                        # Report the ID clash
                        logging.info("Clash between stars with catalogue entry {} {}: ({}) vs ({})".format(
                            cat_name.upper(), cat_id, str(other), str(new_star)))

                        # Delete the catalogue ID from the fainter of the clashing stars
                        other_brighter: bool = ((other.mag_reference is not None) and
                                                (other.mag_reference < new_star.mag_reference))
                        if other_brighter:
                            if cat_name == 'hd':
                                new_star.names_hd_num = None
                            elif cat_name == 'bs':
                                new_star.names_bs_num = None
                            elif cat_name == 'nsv':
                                new_star.names_nsv_num = None
                            elif cat_name == 'hip':
                                new_star.names_hip_num = None
                            elif cat_name == 'tyc':
                                new_star.names_tycho_id = None
                            elif cat_name == 'edr3':
                                new_star.names_edr3_id = None
                        else:
                            if cat_name == 'hd':
                                other.names_hd_num = None
                            elif cat_name == 'bs':
                                other.names_bs_num = None
                            elif cat_name == 'nsv':
                                other.names_nsv_num = None
                            elif cat_name == 'hip':
                                other.names_hip_num = None
                            elif cat_name == 'tyc':
                                other.names_tycho_id = None
                            elif cat_name == 'edr3':
                                other.names_edr3_id = None

                            # Delete star records which no longer have any valid catalogue ID
                            if other.has_valid_catalogue_id():
                                self.star_db.update_star(s=other)
                            else:
                                self.star_db.delete_star(s=other)

    def add_star(self, star_descriptor: StarDescriptor) -> None:
        """
        Add a star to the list of stars we are collecting. Do not add it a second time, if we have already added it.

        :param star_descriptor:
            The descriptor of the star we are to add
        :return:
            None
        """

        # Check for ID clashes
        if self.require_unique_ids:
            self.enforce_unique_catalog_ids(new_star=star_descriptor)

        # Do not add star if it already has an ID
        if (star_descriptor.id is None) or (star_descriptor.db_path != self.star_db.db_path):
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

            # Set reference magnitude
            star_descriptor.mag_reference = star_descriptor.reference_magnitude()

            # Set distance from parallax
            if star_descriptor.parallax:
                parallax: float = star_descriptor.parallax
                if parallax > 0.0000001:
                    # Parallax in mas; Distance in LYR
                    star_descriptor.dist = AU / (parallax * 1e-3 / 3600 / 180 * pi) / LYR
                else:
                    star_descriptor.parallax = None

            # Create a new record for this star
            self.star_db.add_star(s=star_descriptor)
        else:
            self.star_db.update_star(s=star_descriptor)

    def match_star(self, new_star: StarDescriptor, new_catalogue: "StarList") -> \
            List[Tuple[StarDescriptor, Dict[str, int]]]:
        """
        Check whether a new star already exists in the database (matched by catalog number). If it does, return the
        descriptor(s) of the matching star(s) in the database. We also return a dictionary counting the total number of
        stars in the <new_catalogue> which would have produced the same match, for each matching catalogue ID. This is
        useful when multiple new stars have the same HD number, for example, because that star was found to be a
        multiple star system by Hipparcos or Gaia. We need to return a separate count for each catalogue ID, in order
        to figure out which catalogue IDs refer to the whole system, and which to individual stars.

        :param new_star:
            Descriptor of the new star we are adding to the database.
        :param new_catalogue:
            The catalogue of new stars we are adding.
        :return:
            List[Tuple[StarDescriptor, Dict[str, int]]]
        """

        output: List[Tuple[StarDescriptor, Dict[str, int]]] = []

        def append_matches(catalogue: str, match_list: List[Tuple[StarDescriptor, int]]) -> None:
            """
            :param catalogue:
                The name of the catalogue which produced this matching star.
            :param match_list:
                The matching star record; the total number of stars in <new_catalogue> with a matching catalogue ID.
            """
            match: StarDescriptor
            sibling_count: int
            for match, sibling_count in match_list:
                added: bool = False
                # Check if this match already exists; if so, don't repeat it
                for item in output:
                    if item[0] == match:
                        item[1][catalogue] = sibling_count
                        added = True
                        break
                if not added:
                    output.append((match, {catalogue: sibling_count}))

        matches: List[StarDescriptor]

        # See if we can match this star to a known EDR3 entry.
        if new_star.names_edr3_id is not None and len(new_star.names_edr3_id) > 0:
            matches = list(self.lookup_by_edr3_id(edr3_id=new_star.names_edr3_id))
            if len(matches) > 0:
                append_matches(catalogue='edr3', match_list=[
                    (match, len(new_catalogue.lookup_by_edr3_id(edr3_id=new_star.names_edr3_id)))
                    for match in matches])

        # See if we can match this star to a known Tycho entry.
        if new_star.names_tycho_id is not None and len(new_star.names_tycho_id) > 0:
            matches = list(self.lookup_by_tycho_id(tycho_id=new_star.names_tycho_id))
            if len(matches) > 0:
                append_matches(catalogue='tyc', match_list=[
                    (match, len(new_catalogue.lookup_by_tycho_id(tycho_id=new_star.names_tycho_id)))
                    for match in matches])

        # See if we can match this star to a known HIP entry.
        if new_star.names_hip_num is not None and new_star.names_hip_num > 0:
            matches = list(self.lookup_by_hip_num(hip_num=new_star.names_hip_num))
            if len(matches) > 0:
                append_matches(catalogue='hip', match_list=[
                    (match, len(new_catalogue.lookup_by_hip_num(hip_num=new_star.names_hip_num)))
                    for match in matches])

        # See if we can match this star to a known HD entry.
        if new_star.names_hd_num is not None and new_star.names_hd_num > 0:
            matches = list(self.lookup_by_hd_num(hd_num=new_star.names_hd_num))
            if len(matches) > 0:
                append_matches(catalogue='hd', match_list=[
                    (match, len(new_catalogue.lookup_by_hd_num(hd_num=new_star.names_hd_num)))
                    for match in matches])

        # See if we can match this star to a known HR entry.
        if new_star.names_bs_num is not None and new_star.names_bs_num > 0:
            matches = list(self.lookup_by_bs_num(bs_num=new_star.names_bs_num))
            if len(matches) > 0:
                append_matches(catalogue='bs', match_list=[
                    (match, len(new_catalogue.lookup_by_bs_num(bs_num=new_star.names_bs_num)))
                    for match in matches])

        # Matching complete
        return output


class StarDatabase:
    """
    A wrapper for a database of stars held in an SQLite database file.
    """

    schema: Final[str] = """
CREATE TABLE stars
(
    id                     INTEGER PRIMARY KEY,
    child_of               INTEGER,
    ra                     REAL,
    decl                   REAL,
    mag_reference          REAL,
    is_variable            INTEGER,
    color_bv               REAL,
    dist                   REAL,
    parallax               REAL,
    source_par             VARCHAR(32),
    proper_motion          REAL,
    proper_motion_pa       REAL,
    source_pos             VARCHAR(32),

    mag_V                  REAL,
    mag_V_source           VARCHAR(32),
    mag_BT                 REAL,
    mag_BT_source          VARCHAR(32),
    mag_VT                 REAL,
    mag_VT_source          VARCHAR(32),
    mag_G                  REAL,
    mag_G_source           VARCHAR(32),
    mag_BP                 REAL,
    mag_BP_source          VARCHAR(32),
    mag_RP                 REAL,
    mag_RP_source          VARCHAR(32),

    names_english          VARCHAR(256),
    names_catalogue_ref    VARCHAR(64),
    names_bayer_letter     VARCHAR(64),
    names_const            VARCHAR(64),
    names_flamsteed_number INTEGER,
    names_hd_num           INTEGER,
    names_bs_num           INTEGER,
    names_nsv_num          INTEGER,
    names_hip_num          INTEGER,
    names_tycho_id         VARCHAR(32),
    names_edr3_id          VARCHAR(32),
    
    FOREIGN KEY (child_of) REFERENCES stars (id)
);

CREATE {unique} INDEX stars_1 ON stars (names_hd_num);
CREATE {unique} INDEX stars_2 ON stars (names_bs_num);
CREATE {unique} INDEX stars_3 ON stars (names_nsv_num);
CREATE {unique} INDEX stars_4 ON stars (names_hip_num);
CREATE {unique} INDEX stars_5 ON stars (names_tycho_id);
CREATE {unique} INDEX stars_6 ON stars (names_edr3_id);
CREATE INDEX stars_7 ON stars (mag_reference);
CREATE INDEX stars_8 ON stars (child_of);
CREATE INDEX stars_9 ON stars (decl);

"""

    def __init__(self, require_unique_ids: bool = True, db_path: Optional[str] = None, clean: bool = True,
                 read_only: bool = False):
        """
        A class which wraps an SQLite3 database containing a catalogue of stars.

        :param require_unique_ids:
            Boolean flag indicating whether SQL schema should require stars to have unique catalogue numbers.
        :param db_path:
            Optional path to SQLite3 database. If None, then a temporary directory is used.
        :param clean:
            Boolean flag indicating whether to ensure the database is clean before starting.
        :param read_only:
            Boolean flag indicating whether to open the database in read-only mode.
        """

        # Return results as an associative array
        def dict_factory(cursor, row):
            d = {}
            for idx, col in enumerate(cursor.description):
                d[col[0]] = row[idx]
            return d

        self.tmp_dir: Optional[TemporaryDirectory] = None
        self.db_path: str
        self.require_unique_ids: bool = require_unique_ids
        self.clean: bool = clean

        if db_path is None:
            self.tmp_dir = TemporaryDirectory()
            self.db_path = str(os.path.join(self.tmp_dir.tmp_dir, "stars.db"))
            db_already_exists: bool = False
        else:
            self.db_path = db_path
            if clean:
                os.system("rm -f '{}'".format(db_path))
            db_already_exists = os.path.exists(db_path)
        self.db: sqlite3.Connection = sqlite3.connect(self.db_path)
        self.db.execute("PRAGMA foreign_keys=ON;")
        if read_only:
            self.db.execute("PRAGMA query_only=ON;")
        self.db.row_factory = dict_factory
        self.c: sqlite3.Cursor = self.db.cursor()

        # If the database does not already exist, set up its schema now
        if not db_already_exists:
            self.c.executescript(self.schema.format(unique="UNIQUE" if require_unique_ids else ""))

    def __del__(self):
        self.db.commit()
        self.db.close()

        if self.tmp_dir is not None:
            self.tmp_dir.clean_up()

    def __len__(self) -> int:
        """
        Count the total number of stars in the database of stars

        :return:
            int
        """
        self.c.execute("SELECT COUNT(*) FROM stars;")
        result = self.c.fetchall()
        return result[0]['COUNT(*)']

    def count_stars_with_valid_magnitudes(self) -> int:
        """
        Count the number of stars which have valid reference magnitudes

        :return:
            int
        """
        self.c.execute("SELECT COUNT(*) FROM stars WHERE mag_reference IS NOT NULL;")
        result = self.c.fetchall()
        return result[0]['COUNT(*)']

    def count_stars_in_magnitude_range(self, mag_min: float, mag_max: float) -> int:
        """
        Count the number of stars within a requested magnitude range

        :param mag_min:
            The magnitude range to search
        :param mag_max:
            The magnitude range to search
        :return:
            int
        """
        self.c.execute("SELECT COUNT(*) FROM stars WHERE mag_reference >= ? AND mag_reference < ?;",
                       (mag_min, mag_max))
        result = self.c.fetchall()
        return result[0]['COUNT(*)']

    def count_stars_with_bayer_letters(self) -> int:
        """
        Count the number of stars which have valid Bayer letters

        :return:
            int
        """
        self.c.execute("SELECT COUNT(*) FROM stars WHERE names_bayer_letter IS NOT NULL;")
        result = self.c.fetchall()
        return result[0]['COUNT(*)']

    def count_stars_with_flamsteed_numbers(self) -> int:
        """
        Count the number of stars which have valid Flamsteed numbers

        :return:
            int
        """
        self.c.execute("SELECT COUNT(*) FROM stars WHERE names_flamsteed_number IS NOT NULL;")
        result = self.c.fetchall()
        return result[0]['COUNT(*)']

    def add_star(self, s: StarDescriptor) -> None:
        """
        Add a star to the SQLite3 database.
        """

        if s.db_path == self.db_path:
            assert s.id is None, "Attempting to add a star which already has an ID"

        # Do not insert stars into the database with no catalogue ID
        if not s.has_valid_catalogue_id():
            return

        # Insert into database
        try:
            self.c.execute("""
INSERT INTO stars (
    child_of, ra, decl, mag_reference, color_bv, dist, parallax, source_par,
    proper_motion, proper_motion_pa, source_pos, is_variable,
    mag_V, mag_V_source, mag_BT, mag_BT_source, mag_VT, mag_VT_source,
    mag_G, mag_G_source, mag_BP, mag_BP_source, mag_RP, mag_RP_source,
    names_english, names_catalogue_ref, names_bayer_letter, names_const,
    names_flamsteed_number, names_hd_num, names_bs_num, names_nsv_num,
    names_hip_num, names_tycho_id, names_edr3_id
) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);
""", (
                s.child_of, s.ra, s.decl, s.mag_reference, s.color_bv, s.dist, s.parallax, s.source_par,
                s.proper_motion, s.proper_motion_pa, s.source_pos, s.is_variable,
                s.mag_V, s.mag_V_source, s.mag_BT, s.mag_BT_source, s.mag_VT, s.mag_VT_source,
                s.mag_G, s.mag_G_source, s.mag_BP, s.mag_BP_source, s.mag_RP, s.mag_RP_source,
                json.dumps(s.names_english), json.dumps(s.names_catalogue_ref), s.names_bayer_letter, s.names_const,
                s.names_flamsteed_number, s.names_hd_num, s.names_bs_num, s.names_nsv_num,
                s.names_hip_num, s.names_tycho_id, s.names_edr3_id
            ))
        except sqlite3.IntegrityError:
            logging.error("Failed to add star {}".format(s))
            raise

        # Update id field in Python data structure
        s.id = self.c.lastrowid
        s.db_path = self.db_path

        # Logging
        if self.require_unique_ids and s.emit_debugging():
            logging.info("INSERT {}".format(str(s)))

    def update_star(self, s: StarDescriptor) -> None:
        """
        Update a star's record in the SQLite3 database.
        """

        assert s.db_path == self.db_path, "Attempting to update a star from a different database"
        assert s.id is not None, "Attempting to update a star which doesn't already have an ID"

        # Update database
        try:
            self.c.execute("""
UPDATE stars SET
    child_of=?, ra=?, decl=?, mag_reference=?, color_bv=?, dist=?, parallax=?, source_par=?,
    proper_motion=?, proper_motion_pa=?, source_pos=?, is_variable=?,
    mag_V=?, mag_V_source=?, mag_BT=?, mag_BT_source=?, mag_VT=?, mag_VT_source=?,
    mag_G=?, mag_G_source=?, mag_BP=?, mag_BP_source=?, mag_RP=?, mag_RP_source=?,
    names_english=?, names_catalogue_ref=?, names_bayer_letter=?, names_const=?,
    names_flamsteed_number=?, names_hd_num=?, names_bs_num=?, names_nsv_num=?,
    names_hip_num=?, names_tycho_id=?, names_edr3_id=?
WHERE
    id=?;
""", (
                s.child_of, s.ra, s.decl, s.mag_reference, s.color_bv, s.dist, s.parallax, s.source_par,
                s.proper_motion, s.proper_motion_pa, s.source_pos, s.is_variable,
                s.mag_V, s.mag_V_source, s.mag_BT, s.mag_BT_source, s.mag_VT, s.mag_VT_source,
                s.mag_G, s.mag_G_source, s.mag_BP, s.mag_BP_source, s.mag_RP, s.mag_RP_source,
                json.dumps(s.names_english), json.dumps(s.names_catalogue_ref), s.names_bayer_letter, s.names_const,
                s.names_flamsteed_number, s.names_hd_num, s.names_bs_num, s.names_nsv_num,
                s.names_hip_num, s.names_tycho_id, s.names_edr3_id, s.id
            ))
        except sqlite3.IntegrityError:
            logging.error("Failed to update star {}".format(s))
            raise

        # Check that we hit exactly one database entry
        if self.c.rowcount != 1:
            logging.warning("UPDATE operation of the <stars> table modified {:d} rows for ID {:d}.".format(
                self.c.rowcount, s.id))

        # Logging
        if self.require_unique_ids and s.emit_debugging():
            logging.info("UPDATE {}".format(str(s)))

    def delete_star(self, s: StarDescriptor) -> None:
        """
        Delete a star's record in the SQLite3 database.
        """

        assert s.db_path == self.db_path, "Attempting to delete a star from a different database"
        assert s.id is not None, "Attempting to delete a star which doesn't already have an ID"

        # Update database
        self.c.execute("DELETE FROM stars WHERE id=?;", (s.id,))

        # Check that we hit exactly one database entry
        if self.c.rowcount != 1:
            logging.warning("DELETE operation of the <stars> table modified {:d} rows for ID {:d}.".format(
                self.c.rowcount, s.id))

        # Logging
        if self.require_unique_ids and s.emit_debugging():
            logging.info("DELETE {}".format(str(s)))

    def search_by_position(self, ra: float, decl: float, mag: float,
                           position_tolerance: float = 1 / 60. / 60., mag_tolerance: float = 0.1):
        """
        Search for stars in the SQLite3 database by position and magnitude.

        :param ra:
            The RA to search around (hours; J2000.0)
        :param decl:
            The declination to search around (degrees; J2000.0)
        :param mag:
            The magnitude to match against
        :param position_tolerance:
            The search radius around the specified position (degrees).
        :param mag_tolerance:
            The allowed difference is magnitude.
        """

        # Run query
        search_cursor: sqlite3.Cursor
        with closing(self.db.cursor()) as search_cursor:
            search_cursor.execute("""
SELECT s.*, EXISTS (SELECT 1 FROM stars x WHERE x.child_of=s.id) AS is_parent,
       acos(min(1, max(-1,
            sin(s.decl*pi()/180)*sin(?*pi()/180) +
            cos(s.decl*pi()/180)*cos(?*pi()/180)*cos((s.ra - ?)*pi()/12)
            )))*180/pi() AS ang_dist
FROM stars s
WHERE (s.decl BETWEEN ? AND ?) AND (ang_dist < ?) AND (s.mag_reference BETWEEN ? AND ?)
ORDER BY ang_dist;
""", (decl, decl, ra,
      decl - position_tolerance, decl + position_tolerance, position_tolerance,
      mag - mag_tolerance, mag + mag_tolerance))

            # Iterate over results
            for row in search_cursor:
                output = self.build_star_from_sql_row(row=row)
                yield output

    def search(self, hd_num: Optional[int] = None, bs_num: Optional[int] = None, nsv_num: Optional[int] = None,
               hip_num: Optional[int] = None, tycho_id: Optional[str] = None, edr3_id: Optional[str] = None,
               has_parent: Optional[bool] = None, is_parent: Optional[bool] = None, child_of: Optional[int] = None,
               mag_brightest: Optional[float] = None, mag_faintest: Optional[float] = None,
               order_by: str = 'id', sort_direction: str = 'asc') -> Iterator[StarDescriptor]:
        """
        Search for stars in the SQLite3 database.

        :param hd_num:
            Search by HD number.
        :param bs_num:
            Search by bright star number.
        :param nsv_num:
            Search by New Standard Variable number.
        :param hip_num:
            Search by Hipparcos number.
        :param tycho_id:
            Search by Tycho ID.
        :param edr3_id:
            Search by Gaia EDR3 ID.
        :param has_parent:
            Search exclusively for stars which are part of multiple star systems, or not.
        :param is_parent:
            Search exclusively for the parent records of multi-component systems, or not.
        :param child_of:
            Search exclusively for stars which are part of a specific multiple star system.
        :param mag_brightest:
            Return only stars fainter than specified magnitude.
        :param mag_faintest:
            Return only stars brighter than specified magnitude.
        :param order_by:
            Order the results by ('id', 'mag')
        :param sort_direction:
            Order results ('asc', 'desc')
        :return:
            Iterator over StarDescriptor objects.
        """

        filters: List[str] = ['1']
        arguments: List[float | str] = []

        # Set up search filters
        if hd_num is not None:
            filters.append('names_hd_num=?')
            arguments.append(hd_num)
        if bs_num is not None:
            filters.append('names_bs_num=?')
            arguments.append(bs_num)
        if nsv_num is not None:
            filters.append('names_nsv_num=?')
            arguments.append(nsv_num)
        if hip_num is not None:
            filters.append('names_hip_num=?')
            arguments.append(hip_num)
        if tycho_id is not None:
            filters.append('names_tycho_id=?')
            arguments.append(tycho_id)
        if edr3_id is not None:
            filters.append('names_edr3_id=?')
            arguments.append(edr3_id)
        if has_parent is not None:
            if has_parent:
                filters.append('child_of IS NOT NULL')
            else:
                filters.append('child_of IS NULL')
        if is_parent is not None:
            if is_parent:
                filters.append('EXISTS (SELECT 1 FROM stars xx WHERE xx.child_of=s.id)')
            else:
                filters.append('NOT EXISTS (SELECT 1 FROM stars xx WHERE xx.child_of=s.id)')
        if child_of is not None:
            filters.append('child_of=?')
            arguments.append(child_of)
        if mag_brightest is not None:
            filters.append('mag_reference IS NOT NULL AND mag_reference>=?')
            arguments.append(mag_brightest)
        if mag_faintest is not None:
            filters.append('mag_reference IS NOT NULL AND mag_reference<=?')
            arguments.append(mag_faintest)

        # Set up sorting
        if order_by.lower() == 'id':
            sql_order_by: str = 'id {dir:s}'
        elif order_by.lower() == 'mag':
            # Sort by RA as secondary sort criteria to ensure output is reproducible
            sql_order_by = 'mag_reference {dir:s}, ra {dir:s}'
        else:
            raise ValueError("Unexpected sort field <{}>".format(order_by))

        if sort_direction.lower() == 'asc':
            sql_order_direction: str = 'ASC'
        elif sort_direction.lower() == 'desc':
            sql_order_direction = 'DESC'
        else:
            raise ValueError("Unexpected sort order <{}>".format(sort_direction))

        # Run query
        search_cursor: sqlite3.Cursor
        with closing(self.db.cursor()) as search_cursor:
            search_cursor.execute("""
SELECT s.*, EXISTS (SELECT 1 FROM stars x WHERE x.child_of=s.id) AS is_parent
FROM stars s
WHERE ({}) ORDER BY {};
""".format(") AND (".join(filters), sql_order_by.format(dir=sql_order_direction)), tuple(arguments))

            # Iterate over results
            for row in search_cursor:
                output = self.build_star_from_sql_row(row=row)
                yield output

    def build_star_from_sql_row(self, row: dict) -> StarDescriptor:
        output: StarDescriptor = StarDescriptor()
        output.id = row['id']
        output.db_path = self.db_path
        output.child_of = row['child_of']
        output.is_parent = row['is_parent']
        output.ra = row['ra']
        output.decl = row['decl']
        output.mag_reference = row['mag_reference']
        output.is_variable = row['is_variable']
        output.color_bv = row['color_bv']
        output.dist = row['dist']
        output.parallax = row['parallax']
        output.source_par = row['source_par']
        output.proper_motion = row['proper_motion']
        output.proper_motion_pa = row['proper_motion_pa']
        output.source_pos = row['source_pos']

        output.mag_V = row['mag_V']
        output.mag_V_source = row['mag_V_source']
        output.mag_BT = row['mag_BT']
        output.mag_BT_source = row['mag_BT_source']
        output.mag_VT = row['mag_VT']
        output.mag_VT_source = row['mag_VT_source']
        output.mag_G = row['mag_G']
        output.mag_G_source = row['mag_G_source']
        output.mag_BP = row['mag_BP']
        output.mag_BP_source = row['mag_BP_source']
        output.mag_RP = row['mag_RP']
        output.mag_RP_source = row['mag_RP_source']

        output.names_english = json.loads(row['names_english'])
        output.names_catalogue_ref = json.loads(row['names_catalogue_ref'])
        output.names_bayer_letter = row['names_bayer_letter']
        output.names_const = row['names_const']
        output.names_flamsteed_number = row['names_flamsteed_number']
        output.names_hd_num = row['names_hd_num']
        output.names_bs_num = row['names_bs_num']
        output.names_nsv_num = row['names_nsv_num']
        output.names_hip_num = row['names_hip_num']
        output.names_tycho_id = row['names_tycho_id']
        output.names_edr3_id = row['names_edr3_id']
        return output
