#!/usr/bin/python3
# -*- coding: utf-8 -*-
# query_ned.py
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
Use NASA NED to request redshift-independent distance estimates for NGC and IC objects.
"""

import os
import re
import logging
from urllib.request import urlopen


def fetch_redshift(ned_data):
    """
    Search through the HTML for the line which has the redshift on it.

    :param ned_data:
        String containing the HTML web page returned by NED
    :return:
        Dictionary with the fields 'z' and 'z_err' set
    """

    for line in ned_data.split('\n'):
        test = re.search(r"Redshift\s*:\s*(\S*)\s*\+/-\s*(\S*)", line)
        if test is not None:
            z = test.group(1)
            zerr = test.group(2)
            return {
                'z': z,
                'z_err': zerr
            }

    return {
        'z': "---",
        'z_err': "---"
    }


def fetch_distance(ned_data):
    """
    Search through the HTML for the lines which have the redshift-independent distance estimates on them.

    :param ned_data:
        String containing the HTML web page returned by NED
    :return:
        Dictionary with information about the redshift-independent distance estimates
    """

    null_result = {
        'distances_count': "---",
        'distances_mean': "---",
        'distances_median': "---",
        'distances_std_dev': "---",
        'distances_min': "---",
        'distances_max': "---",
        'distances_unit': "---"
    }
    lines = ned_data.split('\n')

    line_count = len(lines)
    for line_count in range(len(lines)):
        test = re.search("REDSHIFT-INDEPENDENT DISTANCES", lines[line_count])
        if test is not None:
            break

    if line_count > len(lines) - 7:
        return null_result

    test = re.search("by NED from (.*) Distance\(s\)", lines[line_count + 1])

    test2 = re.search("Metric Distance<br>\((.*)\)", lines[line_count + 3])

    test3 = re.match(
        "<tr><td>Mean</td> <td>.*</td> <td>(.*)</td></tr> <tr><td>Std\. Dev\.</td> <td>.*</td> <td>(.*)</td></tr>",
        lines[line_count + 4])
    test4 = re.match(
        "<tr><td>Min\.</td> <td>.*</td> <td>(.*)</td></tr> <tr><td>Max\.</td> <td>.*</td> <td>(.*)</td></tr>",
        lines[line_count + 5])
    test5 = re.match(
        "<tr><td>Median</td> <td>(.*)</td> <td>(.*)</td></tr>", lines[line_count + 6])

    if test is None or test2 is None or test3 is None or test4 is None or test5 is None:
        return null_result

    return {
        'distances_count': "%4d" % (int(test.group(1))),
        'distances_mean': test3.group(1),
        'distances_median': test5.group(2),
        'distances_std_dev': test3.group(2),
        'distances_min': test4.group(1),
        'distances_max': test4.group(2),
        'distances_unit': test2.group(1)
    }


# Use process ID to uniquely identify this run
run_id = "%s" % os.getpid()

# The format that we use for the lines of output
line_format = """\
{object:8} {distances_count:11} {distances_mean:8} {distances_median:8} {distances_std_dev:8} \
{distances_min:8} {distances_max:8} {distances_unit:5} \
{z:8} {z_err:8}
"""

# The headings that we put at the top of each line of the output file
column_headings = {
    'object': "# Object",
    'distances_count': "NDistances",
    'distances_mean': "Mean",
    'distances_median': "Median",
    'distances_std_dev': "StdDev",
    'distances_min': "min",
    'distances_max': "max",
    'distances_unit': "unit",
    'z': "z",
    'z_err': "z_err"
}

# Create output directory
os.system("mkdir -p output")

# Create output file and write the column headings
with open("output/NED_distances.{}.dat".format(run_id), "w") as output:
    output.write(line_format.format(**column_headings))

    # Loop over all NGC and IC objects
    for catalogue, size in (("NGC", 7840), ("IC", 5387)):
        for cat_num in range(1, size + 1):

            # Query NED to find out the NED objid for each object
            query_url = "http://nedwww.ipac.caltech.edu/cgi-bin/nph-objsearch?objname={}{}".format(catalogue, cat_num)
            with urlopen(url=query_url) as query_page:
                query_page_str = query_page.read().decode('utf-8')

            # Search the page for each object for a URL with an objid parameter in it
            obj_id = re.search(r"objid=(\d+)&", query_page_str)

            # If we can't find one, we're stuck
            if obj_id is None:
                logging.info("Could not find data for {}{}".format(catalogue, cat_num))
                continue

            obj_id_int = int(obj_id.group(1))

            # Now that we have a NED object ID, use this to fetch a page with information about each object
            object_url = """
http://nedwww.ipac.caltech.edu/cgi-bin/objsearch?search_type=Obj_id&objid={}&img_stamp=YES\
&hconst=73.0&omegam=0.27&omegav=0.73&corr_z=1
""".format(obj_id_int).strip()

            with urlopen(url=object_url) as object_page:
                query_page_str = object_page.read().decode('utf-8')

            # Parse the HTML contents of the page to extract a distance and a redshift
            object_properties = {**fetch_distance(ned_data=query_page_str),
                                 **fetch_redshift(ned_data=query_page_str)
                                 }

            # Write a line to the output file describing each object
            output.write(line_format.format(
                object="{catalogue:3s} {cat_num:04}".format(catalogue=catalogue, cat_num=cat_num),
                **object_properties)
            )

            # Make sure that the output file is always up to date
            output.flush()
