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

from math import hypot, pi, cos
from typing import Dict, Final, Iterator, List, Optional, Tuple, Union

from temporary_directory import TemporaryDirectory

# Constants used for units conversions
AU: Final[float] = 1.49598e11  # metres
LYR: Final[float] = 9.4605284e15  # lightyear in metres


class StarDescriptor:
    """
    A data structure used for storing data about an individual star
    """

    def __init__(self):
        self.id: Optional[int] = None
        self.ra: Optional[float] = None
        self.decl: Optional[float] = None
        self.mag_reference: Optional[float] = None
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
        self.in_gaia_edr3: bool = False
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
        self.names_edr3_id: Optional[str] = None

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


# A list of StarDescriptor objects
class StarList:
    """
    A class which wraps a list of all the stars in our star catalogue
    """

    def __init__(self, magnitude_limit: Optional[float] = None):
        # Store the requested magnitude cut-off for this catalogue
        self.magnitude_limit: Optional[float] = magnitude_limit

        # Storage for the database of stars
        self.star_db: StarDatabase = StarDatabase()

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

    def match_star(self, ra: float, decl: float, mag: float, source: str, match_threshold: float = 0.01,
                   bs_number: Optional[int] = None, hd_number: Optional[int] = None, nsv_number: Optional[int] = None,
                   hipparcos_num: Optional[int] = None, tycho_id: Optional[str] = None, edr3_id: Optional[str] = None) \
            -> StarDescriptor:
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
        :param nsv_number:
            New Standard Variable number for this star
        :param hipparcos_num:
            Hipparcos catalogue number for this star
        :param tycho_id:
            Tycho ID for this star
        :param edr3_id:
            Gaia EDR3 ID for this star
        :return:
            StarDescriptor
        """

        # Holder for the star we have matched
        star: Optional[StarDescriptor] = None

        # See if we can match this star to a known EDR3 entry.
        if edr3_id is not None and len(edr3_id) > 0:
            matches: List[StarDescriptor] = list(self.lookup_by_edr3_id(edr3_id=edr3_id))
            if len(matches) > 0:
                star: Optional[StarDescriptor] = matches[0]

        # See if we can match this star to a known Tycho entry.
        if star is None:
            if tycho_id is not None and len(tycho_id) > 0:
                matches: List[StarDescriptor] = list(self.lookup_by_tycho_id(tycho_id=tycho_id))
                if len(matches) > 0:
                    star: Optional[StarDescriptor] = matches[0]

        # See if we can match this star to a known HIP entry.
        # Do not match if HIP entry is already matched to another star
        if star is None:
            if hipparcos_num is not None and hipparcos_num > 0:
                matches: List[StarDescriptor] = list(self.lookup_by_hip_num(hip_num=hipparcos_num))
                if len(matches) > 0:
                    star: Optional[StarDescriptor] = matches[0]

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
            if hd_number is not None and hd_number > 0:
                matches: List[StarDescriptor] = list(self.lookup_by_hd_num(hd_num=hd_number))
                if len(matches) > 0:
                    star: Optional[StarDescriptor] = matches[0]

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
            star: StarDescriptor = StarDescriptor()

        # Update catalogue numbering
        if hd_number is not None and hd_number > 0:
            if star.names_hd_num is not None and star.names_hd_num != hd_number:
                logging.info("Overwriting HD{:d} with HD{:d}".format(star.names_hd_num, hd_number))
            star.names_hd_num = hd_number

        if bs_number is not None and bs_number > 0:
            if star.names_bs_num is not None and star.names_bs_num != bs_number:
                logging.info("Overwriting BS{:d} with BS{:d}".format(star.names_bs_num, bs_number))
            star.names_bs_num = bs_number

        if nsv_number is not None and nsv_number > 0:
            if star.names_nsv_num is not None and star.names_nsv_num != nsv_number:
                logging.info("Overwriting NSV{:d} with NSV{:d}".format(star.names_nsv_num, nsv_number))
            star.names_nsv_num = nsv_number

        if hipparcos_num is not None and hipparcos_num > 0:
            if star.names_hip_num is not None and star.names_hip_num != hipparcos_num:
                logging.info("Overwriting HIP{:d} with HIP{:d}".format(star.names_hip_num, hipparcos_num))
            star.names_hip_num = hipparcos_num

        # if tycho_id is not None and len(tycho_id) > 0:
        #     if star.names_tycho_id is not None and star.names_tycho_id != tycho_id:
        #         logging.info("Overwriting TYC{:s} with TYC{:s}".format(star.names_tycho_id, tycho_id))
        #     star.names_tycho_id = tycho_id

        if edr3_id is not None and len(edr3_id) > 0:
            if star.names_edr3_id is not None and star.names_edr3_id != edr3_id:
                logging.info("Overwriting EDR3-{:s} with EDR3-{:s}".format(star.names_edr3_id, edr3_id))
            star.names_edr3_id = edr3_id

        # If this star has any catalogue IDs that are already assigned to other stars, then the ID goes to the
        # star with the brighter V magnitude
        for cat_id, cat_field, cat_name in (
                (hd_number, 'hd_num', 'hd'),
                (bs_number, 'bs_num', 'bs'),
                (nsv_number, 'nsv_num', 'nsv'),
                (hipparcos_num, 'hip_num', 'hip'),
                (tycho_id, 'tycho_id', 'tyc'),
                (edr3_id, 'edr3_id', 'edr3')
        ):
            if cat_id is not None and cat_id:
                matches = list(self.star_db.search(**{cat_field: cat_id}))
                if len(matches) > 0:
                    other: StarDescriptor = matches[0]
                    if other.id != star.id:
                        other_brighter: bool = other.mag_V is not None and other.mag_V < mag
                        if other_brighter:
                            if cat_name == 'hd':
                                star.names_hd_num = None
                            elif cat_name == 'bs':
                                star.names_bs_num = None
                            elif cat_name == 'nsv':
                                star.names_nsv_num = None
                            elif cat_name == 'hip':
                                star.names_hip_num = None
                            elif cat_name == 'tyc':
                                star.names_tycho_id = None
                            elif cat_name == 'edr3':
                                star.names_edr3_id = None
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

        # If we are matching a pre-existing star, check we are not moving it too far
        if star.ra is not None:
            star.check_positions_agree(ra_new=ra, dec_new=decl, mag=mag, cat_name_new=source,
                                       threshold=match_threshold)

        return star


class StarDatabase:
    """
    A wrapper for a database of stars held in an SQLite database file.
    """

    schema: str = """
CREATE TABLE stars
(
    id                     INTEGER PRIMARY KEY,
    ra                     REAL NOT NULL,
    decl                   REAL NOT NULL,
    mag_reference          REAL,
    color_bv               REAL,
    dist                   REAL,
    parallax               REAL,
    source_par             VARCHAR(32),
    proper_motion          REAL,
    proper_motion_pa       REAL,
    source_pos             VARCHAR(32),

    in_ybsc                INTEGER,
    in_hipp                INTEGER,
    in_tycho1              INTEGER,
    in_tycho2              INTEGER,
    in_gaia_edr3           INTEGER,
    is_variable            INTEGER,

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
    names_tycho_id         VARCHAR(64),
    names_edr3_id          VARCHAR(64)
);

CREATE UNIQUE INDEX stars_1 ON stars (names_hd_num);
CREATE UNIQUE INDEX stars_2 ON stars (names_bs_num);
CREATE UNIQUE INDEX stars_3 ON stars (names_nsv_num);
CREATE UNIQUE INDEX stars_4 ON stars (names_hip_num);
CREATE UNIQUE INDEX stars_5 ON stars (names_tycho_id);
CREATE UNIQUE INDEX stars_6 ON stars (names_edr3_id);
CREATE INDEX stars_7 ON stars (mag_reference);

"""

    def __init__(self):
        # Return results as an associative array
        def dict_factory(cursor, row):
            d = {}
            for idx, col in enumerate(cursor.description):
                d[col[0]] = row[idx]
            return d

        self.tmp_dir: TemporaryDirectory = TemporaryDirectory()
        self.db_path: str = str(os.path.join(self.tmp_dir.tmp_dir, "stars.db"))
        self.db: sqlite3.Connection = sqlite3.connect(self.db_path)
        self.db.execute("PRAGMA foreign_keys=ON;")
        self.db.row_factory = dict_factory
        self.c: sqlite3.Cursor = self.db.cursor()
        self.c.executescript(self.schema)

    def __del__(self):
        self.db.commit()
        self.db.close()
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
        assert s.id is None, "Attempting to add a star which already has an ID"

        # Do not insert stars into the database with no catalogue ID
        if not s.has_valid_catalogue_id():
            return

        self.c.execute("""
INSERT INTO stars (
    ra, decl, mag_reference, color_bv, dist, parallax, source_par, proper_motion, proper_motion_pa,
    source_pos, in_ybsc, in_hipp, in_tycho1, in_tycho2, in_gaia_edr3, is_variable,
    mag_V, mag_V_source, mag_BT, mag_BT_source, mag_VT, mag_VT_source,
    mag_G, mag_G_source, mag_BP, mag_BP_source, mag_RP, mag_RP_source,
    names_english, names_catalogue_ref, names_bayer_letter, names_const,
    names_flamsteed_number, names_hd_num, names_bs_num, names_nsv_num,
    names_hip_num, names_tycho_id, names_edr3_id
) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);
""", (
            s.ra, s.decl, s.mag_reference, s.color_bv, s.dist, s.parallax, s.source_par,
            s.proper_motion, s.proper_motion_pa,
            s.source_pos, s.in_ybsc, s.in_hipp, s.in_tycho1, s.in_tycho2, s.in_gaia_edr3, s.is_variable,
            s.mag_V, s.mag_V_source, s.mag_BT, s.mag_BT_source, s.mag_VT, s.mag_VT_source,
            s.mag_G, s.mag_G_source, s.mag_BP, s.mag_BP_source, s.mag_RP, s.mag_RP_source,
            json.dumps(s.names_english), json.dumps(s.names_catalogue_ref), s.names_bayer_letter, s.names_const,
            s.names_flamsteed_number, s.names_hd_num, s.names_bs_num, s.names_nsv_num,
            s.names_hip_num, s.names_tycho_id, s.names_edr3_id
        ))

        # Update id field in Python data structure
        s.id = self.c.lastrowid

    def update_star(self, s: StarDescriptor) -> None:
        """
        Update a star's record in the SQLite3 database.
        """
        assert s.id is not None, "Attempting to update a star which doesn't already have an ID"

        self.c.execute("""
UPDATE stars SET
    ra=?, decl=?, mag_reference=?, color_bv=?, dist=?, parallax=?, source_par=?, proper_motion=?, proper_motion_pa=?,
    source_pos=?, in_ybsc=?, in_hipp=?, in_tycho1=?, in_tycho2=?, in_gaia_edr3=?, is_variable=?,
    mag_V=?, mag_V_source=?, mag_BT=?, mag_BT_source=?, mag_VT=?, mag_VT_source=?,
    mag_G=?, mag_G_source=?, mag_BP=?, mag_BP_source=?, mag_RP=?, mag_RP_source=?,
    names_english=?, names_catalogue_ref=?, names_bayer_letter=?, names_const=?,
    names_flamsteed_number=?, names_hd_num=?, names_bs_num=?, names_nsv_num=?,
    names_hip_num=?, names_tycho_id=?, names_edr3_id=?
WHERE
    id=?;
""", (
            s.ra, s.decl, s.mag_reference, s.color_bv, s.dist, s.parallax, s.source_par,
            s.proper_motion, s.proper_motion_pa,
            s.source_pos, s.in_ybsc, s.in_hipp, s.in_tycho1, s.in_tycho2, s.in_gaia_edr3, s.is_variable,
            s.mag_V, s.mag_V_source, s.mag_BT, s.mag_BT_source, s.mag_VT, s.mag_VT_source,
            s.mag_G, s.mag_G_source, s.mag_BP, s.mag_BP_source, s.mag_RP, s.mag_RP_source,
            json.dumps(s.names_english), json.dumps(s.names_catalogue_ref), s.names_bayer_letter, s.names_const,
            s.names_flamsteed_number, s.names_hd_num, s.names_bs_num, s.names_nsv_num,
            s.names_hip_num, s.names_tycho_id, s.names_edr3_id, s.id
        ))

    def delete_star(self, s: StarDescriptor) -> None:
        """
        Delete a star's record in the SQLite3 database.
        """
        assert s.id is not None, "Attempting to delete a star which doesn't already have an ID"

        self.c.execute("DELETE FROM stars WHERE id=?;", (s.id,))

    def search(self, hd_num: Optional[int] = None, bs_num: Optional[int] = None, nsv_num: Optional[int] = None,
               hip_num: Optional[int] = None, tycho_id: Optional[str] = None, edr3_id: Optional[str] = None,
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
        self.c.execute("SELECT * FROM stars WHERE ({}) ORDER BY {};".format(
            ") AND (".join(filters), sql_order_by.format(dir=sql_order_direction)
        ), tuple(arguments))

        # Iterate over results
        for row in self.c:
            output: StarDescriptor = StarDescriptor()
            output.id = row['id']
            output.ra = row['ra']
            output.decl = row['decl']
            output.mag_reference = row['mag_reference']
            output.color_bv = row['color_bv']
            output.dist = row['dist']
            output.parallax = row['parallax']
            output.source_par = row['source_par']
            output.proper_motion = row['proper_motion']
            output.proper_motion_pa = row['proper_motion_pa']
            output.source_pos = row['source_pos']

            output.in_ybsc = row['in_ybsc']
            output.in_hipp = row['in_hipp']
            output.in_tycho1 = row['in_tycho1']
            output.in_tycho2 = row['in_tycho2']
            output.in_gaia_edr3 = row['in_gaia_edr3']
            output.is_variable = row['is_variable']

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

            yield output
