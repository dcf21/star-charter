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
import logging
import numpy as np
import os
import re
import sys

from math import atan2, floor, hypot, isfinite, pi
from typing import Callable, Dict, Final, Optional, List, Sequence, Tuple

from star_database.star_descriptor import StarDescriptor, StarList, debugging_active, emit_debugging
from star_database.star_name_conversion import StarNameConversion

# File path of this script, used to locate input and output paths
our_path: Final[str] = os.path.split(os.path.abspath(__file__))[0]
input_path: Final[str] = os.path.join(our_path, "../../data/stars")
output_path: Final[str] = os.path.join(our_path, "../../data_generated/star_catalogue_merged")

# Reject parallaxes with more than a 30% error
maximum_allowed_parallax_error: Final[float] = 0.3


def generate_debugging_output(target: StarList, new_entries: StarList) -> None:
    """
    Generate verbose debugging information about the list of catalogue references in <debug_targets>.

    :param target:
        The target star database we are to populate.
    :param new_entries:
        The database of new stars to add into the target database.
    :return:
        None
    """

    # Loop over all the objects we are to add
    new_star: StarDescriptor
    for new_star in new_entries.star_db.search(order_by='id', sort_direction='asc'):
        # Search for matches
        matches: List[Tuple[StarDescriptor, Dict[str, int]]]
        matches = target.match_star(new_star=new_star, new_catalogue=new_entries)

        # Decide whether verbose logging is needed for this target
        display_debugging: bool = emit_debugging(star_list=[new_star] + [i[0] for i in matches])

        # Display verbose logging where needed
        if display_debugging:
            logging.info("Verbose tracking -- New star {} matches {}".format(new_star, matches))


def list_multiple_star_systems(target: StarList, mag_cutoff: Optional[float] = None, child_of: Optional[int] = None,
                               min_children: Optional[int] = None, max_children: Optional[int] = None,
                               indent_level: int = 1, output_logs: bool = True) -> List[str]:
    """
    Display a list of the brightest multiple-star systems.

    :param target:
        The target star database.
    :param mag_cutoff:
        Do not show stars fainter than this.
    :param child_of:
        Show the descendents of a specific parent system.
    :param min_children:
        Only show systems with at least this number of children.
    :param max_children:
        Only show systems with up to this number of children.
    :param indent_level:
        The level of indent to use
    :param output_logs:
        If true, emit logging output.
    :return:
        List of strings
    """

    output: List[str] = []

    # Set up filters
    filters: dict = {
        'order_by': 'mag'
    }

    if child_of is None:
        filters['has_parent'] = False
        filters['is_parent'] = True
    else:
        filters['child_of'] = child_of

    # Loop over all the parent stars in the database
    star: StarDescriptor
    for star in target.star_db.search(**filters):
        if (mag_cutoff is not None) and ((star.mag_reference is None) or (star.mag_reference > mag_cutoff)):
            break
        indent: str = "--> " * indent_level
        output_str: str = "{} {} (mag {:.1f})".format(indent, str(star), star.reference_magnitude())
        child_logging: List[str] = list_multiple_star_systems(
            target=target, mag_cutoff=mag_cutoff, indent_level=indent_level + 1, output_logs=False, child_of=star.id)

        if (
                ((min_children is None) or (len(child_logging) >= min_children)) and
                ((max_children is None) or (len(child_logging) <= max_children))
        ):
            output.append(output_str)
            output.extend(child_logging)

    # If requested display output
    if output_logs:
        for item in output:
            logging.info(item)

    # Return output
    return output


def display_multiple_star_statistics(target: StarList) -> None:
    """
    Display statistics about the membership of multiple-star systems.

    :param target:
        The target star database.
    :return:
        None
    """

    # Multiple star parents
    parents: Dict[int, Dict[str, Optional[int]]] = {}

    parent_stub: Dict[str, Optional[int]] = {
        'child_count': 0,
        'max_child_depth': 0,
        'parent_id': None
    }

    # Loop over all the stars in the database
    new_star: StarDescriptor
    for new_star in target.star_db.search(order_by='id', has_parent=True):
        parent_id: int = new_star.child_of
        if parent_id not in parents:
            parents[parent_id] = parent_stub.copy()
        parents[parent_id]['child_count'] += 1
    for new_star in target.star_db.search(order_by='id', has_parent=True):
        if new_star.id in parents:
            parents[new_star.id]['parent_id'] = new_star.child_of

    # Display histogram of the number of members in each system.
    histogram: Dict[int, int] = {}
    for parent in parents.values():
        child_count: int = parent['child_count']
        if child_count not in histogram:
            histogram[child_count] = 0
        histogram[child_count] += 1

    # Produce no output if there are no multiple-star systems to report
    if len(histogram) == 0:
        return

    logging.info("Multiple star systems membership histogram")
    for child_count in sorted(histogram.keys()):
        logging.info("--> Systems with {:3d} members: {:4d}".format(child_count, histogram[child_count]))

    # Display histogram of the hierarchy depth of each system.
    for parent in parents.values():
        family_tree: List[dict] = [parent]
        while family_tree[-1]['parent_id'] is not None:
            family_tree.append(parents[family_tree[-1]['parent_id']])
        family_tree[-1]['max_child_depth'] = len(family_tree)

    histogram: Dict[int, int] = {}
    for parent in parents.values():
        max_child_depth: int = parent['max_child_depth']
        if max_child_depth > 0:
            if max_child_depth not in histogram:
                histogram[max_child_depth] = 0
            histogram[max_child_depth] += 1

    logging.info("Multiple star systems depth histogram")
    for child_count in sorted(histogram.keys()):
        logging.info("--> Systems with {:3d} levels of children: {:4d}".format(
            child_count, histogram[child_count]))


def merge_databases(target: StarList, new_entries: StarList, source: str) -> None:
    """
    Merge all the stars into the <target> catalogue, matching by ID where possible.

    :param target:
        The target star database we are to populate.
    :param new_entries:
        The database of new stars to add into the target database.
    :param source:
        The name of the catalogue of new entries (used in log messages).
    :return:
        None
    """

    # Distance beyond which we report a warning that a star match seems dubious
    match_threshold: Final[float] = 0.025

    # Count outcomes
    matches_ambiguous: int = 0
    matches_multiple_star: int = 0
    matches_single_star: int = 0
    matches_by_position: int = 0
    matches_new_star: int = 0
    total_stars: int = 0

    # Loop over all the objects we are to add
    new_star: StarDescriptor
    for new_star in new_entries.star_db.search(order_by='id', sort_direction='asc'):
        total_stars += 1

        # Search for matches
        matches: List[Tuple[StarDescriptor, Dict[str, int]]]
        matches = target.match_star(new_star=new_star, new_catalogue=new_entries)

        # If we had multiple ambiguous matches, issue a warning
        if len(matches) > 1:
            matches_ambiguous += 1
            filter_multiple_matches(target=target, new_star=new_star, matches=matches)

        # If we got a match, merge the new entry into the match
        elif len(matches) > 0:
            old_star: StarDescriptor = matches[0][0]
            match_count_by_catalogue: Dict[str, int] = matches[0][1]

            # The number of stars in the new data source that match <old_star>. If this is greater than one, then
            # this has been resolved into a multiple-star system.
            new_entry_count_for_old_star: int = max(match_count_by_catalogue.values())

            if new_entry_count_for_old_star <= 1:
                # Star has not been split into a multiple-star system
                matches_single_star += 1

                # If we are matching a pre-existing star, check we are not moving it too far
                if ((old_star.ra is not None) and isfinite(old_star.ra) and
                        (old_star.decl is not None) and isfinite(old_star.decl) and
                        (new_star.ra is not None) and isfinite(new_star.ra) and
                        (new_star.decl is not None) and isfinite(new_star.decl)):
                    old_star.check_positions_agree(
                        other=new_star, cat_name_new=source, threshold=match_threshold)

                # Merge new data into old entry
                old_star.merge_new_data(new_star=new_star)
                target.add_star(star_descriptor=old_star)
            else:
                # Star has been split into a multiple star system
                matches_multiple_star += 1

                # Mark this as a subcomponent of the multiple star system
                split_multiple_star(target=target, match_count_by_catalogue=match_count_by_catalogue,
                                    new_star=new_star, old_star=old_star)
        else:
            # Star did not match any existing database entries
            positional_match: Optional[StarDescriptor] = match_by_position(target=target, new_star=new_star)
            if positional_match is not None:
                # Check whether the star is a good positional match to any previous database entry
                matches_by_position += 1
                positional_match.merge_new_data(new_star=new_star)
                target.add_star(star_descriptor=positional_match)
            else:
                matches_new_star += 1
                target.add_star(star_descriptor=new_star)

    # Final logging update
    logging.info("""
-> Found {:,} new stars; {:,} existing singletons; {:,} new multiple stars; {:,} ambiguous catalogue IDs; \
{:,} matches by position; total {:,}.
""".format(matches_new_star, matches_single_star, matches_multiple_star, matches_ambiguous, matches_by_position,
           total_stars).strip())
    logging.info("-> Total of {:,} stars in database.".format(len(target)))


def match_by_position(target: StarList, new_star: StarDescriptor) -> Optional[StarDescriptor]:
    """
    Try to match a new star against existing stars, purely by its position and magnitude.

    :param target:
        The list of stars that we are populating.
    :param new_star:
        The new star to add to the list.
    :return:
        The star descriptor for any matching star.
    """

    position_tolerance: Final[float] = 0.6 / 60 / 60  # 0.6 arcseconds
    mag_tolerance: Final[float] = 0.1  # 0.1 magnitudes

    if (new_star.ra is None) or (new_star.decl is None) or (new_star.mag_reference is None):
        return None

    matches: List[StarDescriptor] = list(target.star_db.search_by_position(
        ra=new_star.ra, decl=new_star.decl, mag=new_star.mag_reference,
        position_tolerance=position_tolerance, mag_tolerance=mag_tolerance
    ))

    if len(matches) == 1:
        positional_match: StarDescriptor = matches[0]
        if emit_debugging(star_list=[new_star, positional_match]):
            logging.info("Matched <{}> to <{}>; separation <{:.4f} arcsec>".format(
                new_star, positional_match, new_star.angular_separation(other=positional_match) * 3600))
        return positional_match
    else:
        return None


def filter_multiple_matches(target: StarList, new_star: StarDescriptor,
                            matches: List[Tuple[StarDescriptor, Dict[str, int]]]) -> None:
    """
    Handle cases where a data record <new_star> matches more than one entry in the database of stars. Choose which
    entry to merge the new data into.

    :param target:
        The list of stars that we are populating.
    :param new_star:
        The new star to add to the list, which matches multiple possible entries in <target>.
    :param matches:
        The list of entries in <target> that <new_star> matches.
    """

    output_debugging: Final[bool] = emit_debugging(star_list=[new_star])

    if output_debugging:
        logging.warning("Ambiguous match of <{new_name}> to <{old_names}>".format(
            new_name=str(new_star), old_names=">    <".join([str(item) for item in matches])
        ))

    # Compile information about each match
    match_info: List[Tuple[StarDescriptor, dict, dict]] = []
    for match in matches:
        # Count conflicting IDs
        conflicting_ids: int = 0
        for catalogue in ('edr3', 'tyc', 'hip', 'hd', 'nsv', 'bs'):
            old_star_ref = match[0].get_catalogue_name(catalogue=catalogue)
            new_star_ref = new_star.get_catalogue_name(catalogue=catalogue)
            if (old_star_ref is not None) and (new_star_ref is not None) and (old_star_ref != new_star):
                conflicting_ids += 1

        # Compile match statistics
        i: dict = {
            'is_parent': match[0].is_parent,
            'is_child': match[0].child_of is not None,
            'parent_of_other_match': len([1 for x in matches if x[0].child_of == match[0].id]),
            'match_siblings': max(match[1].values()),
            'conflicting_ids': conflicting_ids,
            'processed': False
        }

        i['reject'] = i['parent_of_other_match']
        i['must_preserve'] = i['is_parent'] or i['is_child']
        i['must_split'] = (i['match_siblings'] > 1)
        i['simple_merge'] = (not i['must_preserve']) and (not i['must_split'])

        # Add to list
        match_info.append((match[0], match[1], i))

    # Categorise matches
    merge_matches: List[Tuple[StarDescriptor, dict, dict]] = []
    rejected_matches: List[Tuple[StarDescriptor, dict, dict]] = []
    final_match: Optional[Tuple[StarDescriptor, dict, dict]] = None

    for i in match_info:  # Item we can merge simply
        if (not i[2]['reject']) and i[2]['simple_merge']:
            i[2]['processed'] = True
            merge_matches.append(i)
    for i in match_info:  # Item where we are a child of a binary system
        if (not i[2]['reject']) and (not i[2]['processed']) and (final_match is None) and i[2]['must_split']:
            i[2]['processed'] = True
            final_match = i
    for i in match_info:  # Item we must preserve because it is a parent / child
        if (not i[2]['reject']) and (not i[2]['processed']) and (final_match is None) and i[2]['must_preserve']:
            i[2]['processed'] = True
            final_match = i
    for i in match_info:  # Item we can't process
        if not i[2]['processed']:
            i[2]['processed'] = True
            rejected_matches.append(i)

    # Do simple merge operations
    if len(merge_matches) > 0:
        # Merge stars
        merged_star: StarDescriptor = StarDescriptor()
        for item in merge_matches:
            merged_star.merge_new_data(new_star=item[0])
        merged_star.merge_new_data(new_star=new_star)

        if final_match is None:
            # Adopt ID of last merged stars
            merged_star.adopt_id_from(old_star=merge_matches[-1][0])
            # Delete all matches except the one whose ID we adopted
            for item in merge_matches[:-1]:
                target.star_db.delete_star(s=item[0])
        else:
            # Delete all matches
            for item in merge_matches:
                target.star_db.delete_star(s=item[0])
    else:
        merged_star = new_star

    # Remove all catalogue IDs which matched the other stars, to avoid conflict
    for match in rejected_matches:
        for catalogue in ('edr3', 'tyc', 'hip', 'hd', 'nsv', 'bs'):
            if catalogue in match[1]:
                merged_star.set_catalogue_name(catalogue=catalogue, name=None)

    # Split binary
    if (final_match is not None) and (final_match[2]['must_split']):
        split_multiple_star(target=target, match_count_by_catalogue=final_match[1],
                            new_star=merged_star, old_star=final_match[0])

    else:
        # Preserve a parent star
        if final_match is not None:
            merged_star.adopt_id_from(old_star=final_match[0])

        # Nothing more to preserve
        target.add_star(star_descriptor=merged_star)

    if output_debugging:
        logging.warning("--> Resolved <{new_star}> to become <{merged_star}>".format(
            new_star=new_star, merged_star=merged_star
        ))


def split_multiple_star(target: StarList, match_count_by_catalogue: Dict[str, int], new_star: StarDescriptor,
                        old_star: StarDescriptor):
    """
    Split a star record in the database to reflect that it has been resolved into a multiple-star system.

    :param target:
        The list of stars that we are populating.
    :param match_count_by_catalogue:
        The number of existing stars matching <new_star> in each star catalogue.
    :param new_star:
        The new star to add to the list, which has been identified as a member of a multiple-star system.
    :param old_star:
        The existing star in the list which is the parent record for the multiple-star system.
    """
    # Mark this as a subcomponent of the multiple star system
    new_star.child_of = old_star.id

    # We might create two child stars
    additional_child: Optional[StarDescriptor] = None

    # The member star should not receive the catalogue ID of the whole system. For each catalogue ID, give
    # it to the child if it is unique, and to the parent if multiple stars match it.
    for catalogue in ('edr3', 'tyc', 'hip', 'hd', 'nsv', 'bs'):
        old_star_ref = old_star.get_catalogue_name(catalogue=catalogue)
        new_star_ref = new_star.get_catalogue_name(catalogue=catalogue)

        # If both parent and child have entries in a star catalogue, decide how to split them
        if (old_star_ref is not None) and (new_star_ref is not None):
            # If the entry is the same for both parent and child, then give it to the parent if it matches multiple
            # stars in the new data source, or the child if it matches only one star.
            if old_star_ref == new_star_ref:
                if catalogue in match_count_by_catalogue and match_count_by_catalogue[catalogue] > 1:
                    # Give to parent; delete from child
                    new_star.set_catalogue_name(catalogue=catalogue, name=None)
                else:
                    # Give to child; delete from parent
                    old_star.set_catalogue_name(catalogue=catalogue, name=None)

            # If the entry differs for parent and child, then create two children, one with each catalogue ID
            if old_star_ref != new_star_ref:
                # Create additional child
                additional_child = old_star.copy().delete_catalogue_entries()
                additional_child.child_of = old_star.id
                additional_child.set_catalogue_name(catalogue=catalogue, name=old_star_ref)

                # Give to child; delete from parent
                old_star.set_catalogue_name(catalogue=catalogue, name=None)

    # Add new member star, and update the parent star
    target.add_star(star_descriptor=old_star)
    target.add_star(star_descriptor=new_star)
    if additional_child is not None:
        target.add_star(star_descriptor=additional_child)
        if emit_debugging(star_list=[old_star, new_star, additional_child]):
            logging.info("Splitting multiple star system -- parent {} and children {}    {}.".format(
                old_star, new_star, additional_child))
    else:
        if emit_debugging(star_list=[old_star, new_star]):
            logging.info("Splitting multiple star system -- parent {} and child {}.".format(old_star, new_star))


def read_from_ybsc() -> Tuple[StarList, str]:
    """
    Read stars from Yale Bright Star Catalogue.

    :return:
        The list of stars we have read , The source string for the catalogue we read.
    """

    output: StarList = StarList(require_unique_ids=False)

    star_name_converter: StarNameConversion = StarNameConversion()

    # Read stars from Bright Star Catalogue
    logging.info("Reading data from Yale Bright Star Catalogue...")
    source: str = "bright_stars"
    rejection_count: int = 0
    ybsc_path: Final[str] = os.path.join(input_path, "brightStars/catalog.gz")
    for line in gzip.open(ybsc_path, "rt"):
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

        # Create star descriptor
        star: StarDescriptor = StarDescriptor()
        star.names_hd_num = hd_number
        star.names_bs_num = bs_number
        star.names_nsv_num = nsv_number

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

        # Ensure this star is recorded in the output catalogue
        output.add_star(star_descriptor=star)

    logging.info("-> Read {:,} stars; rejected {:,} stars.".format(len(output), rejection_count))
    return output, source


def read_from_hipparcos() -> Tuple[StarList, str]:
    """
    Read stars from Hipparcos Catalogue.

    :return:
        The list of stars we have read , The source string for the catalogue we read.
    """

    output: StarList = StarList(require_unique_ids=False)

    # Read data from Hipparcos catalogue
    logging.info("Reading Hipparcos catalogue...")
    source: str = "hipparcos"
    rejection_count: int = 0
    tycho1_path: Final[str] = os.path.join(input_path, "tycho1/hip_main.dat.gz")
    for line in gzip.open(tycho1_path, "rt"):
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

        # Create star descriptor
        star: StarDescriptor = StarDescriptor()
        star.names_hd_num = hd_number
        star.names_hip_num = hipparcos_num

        # Record this star's position and parallax
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
        output.add_star(star_descriptor=star)

    logging.info("-> Read {:,} stars; rejected {:,} stars.".format(len(output), rejection_count))
    return output, source


def read_from_tycho_1() -> Tuple[StarList, str]:
    """
    Read stars from Tycho 1 Catalogue.

    :return:
        The list of stars we have read , The source string for the catalogue we read.
    """

    output: StarList = StarList(require_unique_ids=False)

    # Read data from Tycho 1 catalogue (this contains Henry Draper numbers, missing from Tycho 2)
    logging.info("Reading Tycho 1 catalogue...")
    source: str = "tycho1"
    rejection_count: int = 0
    tycho1_path: Final[str] = os.path.join(input_path, "tycho1/tyc_main.dat")
    with open(tycho1_path, "rt") as f_in:
        for line in f_in:
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

            # Create star descriptor
            star: StarDescriptor = StarDescriptor()
            star.names_hd_num = hd_number
            star.names_hip_num = hipparcos_num
            star.names_tycho_id = tycho_id

            # Record this star's position and parallax
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
            output.add_star(star_descriptor=star)

    logging.info("-> Read {:,} stars; rejected {:,} stars.".format(len(output), rejection_count))
    return output, source


def read_from_tycho_2() -> Tuple[StarList, str]:
    """
    Read stars from Tycho 2 Catalogue.

    :return:
        The list of stars we have read , The source string for the catalogue we read.
    """

    output: StarList = StarList(require_unique_ids=False)

    # Read data from Tycho 2 catalogue
    logging.info("Reading Tycho 2 catalogue...")
    source: str = "tycho2"
    rejection_count: int = 0
    tycho2_path: Final[str] = os.path.join(input_path, "tycho2/tyc2.dat.*.gz")
    for filename in glob.glob(tycho2_path):
        for line in gzip.open(filename, "rt"):
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

            # Create star descriptor
            star: StarDescriptor = StarDescriptor()
            star.names_hip_num = hipparcos_num
            star.names_tycho_id = tycho_id

            # Record this star's position and parallax
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
            output.add_star(star_descriptor=star)

    logging.info("-> Read {:,} stars; rejected {:,} stars.".format(len(output), rejection_count))
    return output, source


def read_from_hipparcos_new() -> Tuple[StarList, str]:
    """
    Read stars from Hipparcos new reduction catalogue.

    :return:
        The list of stars we have read , The source string for the catalogue we read.
    """

    output: StarList = StarList(require_unique_ids=False)

    # Read data from Hipparcos new reduction
    logging.info("Reading Hipparcos new reduction catalogue...")
    source: str = "hipparcos_new"
    rejection_count: int = 0
    hipparcos_path: Final[str] = os.path.join(input_path, "hipparcosNewReduction/hip2.dat.gz")
    for line in gzip.open(hipparcos_path, "rt"):
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

        # Create star descriptor
        star: StarDescriptor = StarDescriptor()
        star.names_hip_num = hipparcos_num

        if parallax and (parallax_error < parallax * maximum_allowed_parallax_error):
            star.parallax = parallax
            star.source_par = source
        if proper_motion:
            star.proper_motion = proper_motion
            star.proper_motion_pa = proper_motion_pa

        # Ensure this star is recorded in the output catalogue
        output.add_star(star_descriptor=star)

    logging.info("-> Read {:,} stars; rejected {:,} stars.".format(len(output), rejection_count))
    return output, source


def read_from_gaia_edr3() -> Tuple[StarList, str]:
    """
    Read stars from Gaia EDR 3 Catalogue.

    :return:
        The list of stars we have read , The source string for the catalogue we read.
    """

    output: StarList = StarList(require_unique_ids=False)

    # Read data from Gaia EDR3
    logging.info("Reading Gaia EDR3 catalogue...")
    source: str = "gaia_edr3"
    rejection_count: int = 0
    edr3_path: Final[str] = os.path.join(input_path, "gaiaEDR3/*.csv.gz")
    for filename in sorted(glob.glob(edr3_path)):
        logging.info("Reading file <{}>...".format(filename))
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

            # Create star descriptor
            star: StarDescriptor = StarDescriptor()
            if hipparcos_num is not None and hipparcos_num > 0:
                star.names_hip_num = hipparcos_num
            if tycho_id is not None and len(tycho_id.strip()) > 0:
                star.names_tycho_id = tycho_id.strip()
            if edr3_id is not None and len(edr3_id.strip()) > 0:
                star.names_edr3_id = edr3_id

            # Record this star's position and parallax
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
            output.add_star(star_descriptor=star)

    logging.info("-> Read {:,} stars; rejected {:,} stars.".format(len(output), rejection_count))
    return output, source


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
            total_count += 1
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


def read_english_names() -> Tuple[StarList, str]:
    """
    Read popular names for stars.

    :return:
        The list of stars we have read , The source string for the catalogue we read.
    """

    output: StarList = StarList(require_unique_ids=False)

    # Read list of English names for stars
    logging.info("Reading English names for stars...")
    source: str = "iau_wgsn"

    star_names_path: Final[str] = os.path.join(input_path, "brightStars/starNames_iau.txt")
    with open(star_names_path, "rt") as f_in:
        for line in f_in:
            if (len(line) < 5) or (line[0] == '#'):
                continue

            star: StarDescriptor = StarDescriptor()
            try:
                star.names_hip_num = int(line[90:].split()[0])
            except ValueError:
                pass
            try:
                star.names_hd_num = int(line[97:].split()[0])
            except ValueError:
                pass
            star.add_english_name(new_name=line[18:36].strip(), prepend=True)

            # Update star
            if star.names_hip_num or star.names_hd_num:
                output.add_star(star_descriptor=star)

    return output, source


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
    os.system("mkdir -p {}".format(output_path))

    # Big list of information about each star
    db_path: Final[str] = os.path.join(output_path, "all_stars.sqlite3")
    star_data: StarList = StarList(magnitude_limit=magnitude_limit, require_unique_ids=True, db_path=db_path)

    # List of functions that read input catalogues
    reader_functions: List[Callable] = [
        read_from_ybsc, read_from_hipparcos, read_from_tycho_1, read_from_tycho_2, read_from_hipparcos_new,
        read_from_gaia_edr3
    ]

    # Read each catalogue in turn
    reader_function: Callable
    for reader_function in reader_functions:
        new_entries: StarList
        source: str

        # Read input catalogue
        new_entries, source = reader_function()

        # If debugging is requested, search new data for the targets we are monitoring
        if debugging_active():
            generate_debugging_output(target=star_data, new_entries=new_entries)

        # Merge new catalogue into output
        merge_databases(target=star_data, new_entries=new_entries, source=source)

        # Debugging
        list_multiple_star_systems(target=star_data, max_children=1)
        display_multiple_star_statistics(target=star_data)

    # Update Bayer / Flamsteed star names
    read_bayer_flamsteed_cross_index(star_data=star_data)

    # Update English star names
    new_entries, source = read_english_names()
    merge_databases(target=star_data, new_entries=new_entries, source=source)

    # Create a histogram of the brightness of all the stars we have inserted into catalogue
    bin_size: float = 0.25
    bin_min: float = -2
    bin_max: float = 20

    logging.info("-> Total of {:,} stars with valid magnitudes.".format(
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
    os.system("rm -f '{}' '{}/*.bin'".format(star_charter_output, output_path))

    # Limiting magnitude of 14 for StarCharter
    mag_limit: Final[float] = 14
    stars_within_magnitude_limit: Final[int] = star_data.star_db.count_stars_in_magnitude_range(
        mag_min=-50, mag_max=mag_limit)

    # Write output as a text file for use in StarCharter
    with gzip.open(star_charter_output, "wt") as output_star_charter:
        # Show progress
        progress_total: Final[int] = stars_within_magnitude_limit
        progress_steps: Final[int] = 20
        last_display: str = ""

        # Loop over all the objects we've catalogued
        for counter, star in enumerate(star_data.star_db.search(order_by='mag', mag_faintest=mag_limit)):
            # Progress update
            if counter % 100 == 0:
                progress_str: str = "Writing output... {:3.0f}% done".format(
                    (floor(counter / progress_total * progress_steps) / progress_steps * 100))
                if progress_str != last_display:
                    logging.info(progress_str)
                    last_display = progress_str

            # name_1 is the star's Bayer letter, as a UTF8 character
            # name_2 is 'x-c' where x is name_1 and c is the constellation
            # name_3 is the star's English name, which spaces rendered as underscores
            # name_4 is the star's catalogue name, e.g. "V337_Car"
            # name_5 is 'y' where y is the Flamsteed number
            name_1 = name_2 = name_3 = name_4 = name_5 = "-"

            # Populate variables with data from the StarDescriptor structure
            ra: float = star.ra if star.ra is not None else np.nan
            decl: float = star.decl if star.decl is not None else np.nan
            parallax: float = star.parallax if star.parallax is not None else np.nan
            dist: float = star.dist if star.dist is not None else np.nan
            mag_bv: float = star.color_bv if star.color_bv is not None else np.nan

            # Catalogue numbers for this star
            ybsn_num: int = star.names_bs_num if star.names_bs_num is not None else 0
            hd_num: int = star.names_hd_num if star.names_hd_num is not None else 0
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
            if star.names_catalogue_ref:
                # StarCharter using whitespace-sep columns
                name_4: str = re.sub(' ', '_', star.names_catalogue_ref[0])

            if star.names_const:
                if star.names_bayer_letter:
                    name_bayer_letter: str = star.names_bayer_letter
                    name_1: str = star_name_converter.greek_html_to_utf8(
                        in_str=star_name_converter.name_to_html(in_str=name_bayer_letter)
                    )
                    name_2: str = "{}-{}".format(name_1, star.names_const)
                if star.names_flamsteed_number:
                    name_5: str = "{}".format(star.names_flamsteed_number)

            output_star_charter.write("""
{:6d} {:6d} {:8d} {:17.12f} {:17.12f} {:17.12f} {:17.12f} {:17.12f} {:17.12f} {:s} {:s} {:s} {:s} {:s}
""".format(hd_num, ybsn_num, hip_num, ra, decl, star.mag_reference, mag_bv, parallax, dist,
           name_1, name_2, name_3, name_4, name_5).strip()
                                      )
            output_star_charter.write("\n")


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
    logging.info(__doc__.strip())

    # Perform merger operation
    merge_star_catalogues(magnitude_limit=args.magnitude_limit)
