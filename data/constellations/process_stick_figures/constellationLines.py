#!/usr/bin/python3
# -*- coding: utf-8 -*-
# constellationLines.py
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
This script takes the designs for stick figures to represent the 88 astronomical constellations, which are defined in
the files in the <input> directory, and we convert these designs into alternative data formats. These input design files
list sequences of stars to connect, referenced by their Hipparcos numbers. For ease of use, we now output these designs
in a range of alternative formats, such as listing the RA and Dec of the stars, rather than their Hipparcos numbers.
"""

import os
import sys
import copy
import logging
import json
import gzip

# A list of the input files we are to process, containing various designs of stick figures
input_files = [
    {
        'filename': "../constellation_lines_simplified.dat",
        'output_stub': "output/constellation_lines_simplified"
    },
    {
        'filename': "../constellation_lines_rey.dat",
        'output_stub': "output/constellation_lines_rey"
    }
]

# A list of the output formats we are to copy each set of stick figures into
output_formats = [
    {
        'name': "by_HR",
        'fields': ['hr_number'],
        'mark_stars_in_neighbouring_constellations': True,
        'wrap_at_ra_0': False,
        'description': """
###
## Each line of this file lists a pair of stars, referenced by their catalogue
## numbers within the Yale Bright Star catalogue. To construct stick figures,
## draw a line between each of the pairs of stars defined on the lines of this
## file.  
        """
    },
    {
        'name': "by_RA_Dec",
        'fields': ['ra', 'dec'],
        'mark_stars_in_neighbouring_constellations': False,
        'wrap_at_ra_0': False,
        'description': """
###
## Each line of this file lists a pair of stars, referenced by their right
## ascensions (col 2 and 4) and declinations (col 3 and 5). To construct stick
## figures, draw a line between each of the pairs of stars defined on the lines
## of this file.
        """
    },
    {
        'name': "by_RA_Dec_wrapped",
        'fields': ['ra', 'dec'],
        'mark_stars_in_neighbouring_constellations': False,
        'wrap_at_ra_0': True,
        'description': """
###
## Each line of this file lists a pair of stars, referenced by their right
## ascensions (col 2 and 4) and declinations (col 3 and 5). To construct stick
## figures, draw a line between each of the pairs of stars defined on the lines
## of this file.
##
## To aid with drawing all-sky charts with RA=23h59m on the far left and RA=0h
## on the far right, sticks which cross this line are repeated twice, since they
## need to be drawn on both sides of the chart. The first time, the lines are
## written with one end expressed as being at RA<0h. The second time, the same
## line is written with the other end being at RA>24h.
"""
    }
]


def read_bright_star_catalogue():
    stars_by_hd = {}
    for hr_number, line_str in enumerate(gzip.open("../downloads/ybsc.gz", "rt")):
        try:
            hd_number = int(line_str[25:31])
            ra_hrs = float(line_str[75:77])
            ra_min = float(line_str[77:79])
            ra_sec = float(line_str[79:82])
            dec_neg = (line_str[83] == '-')
            dec_deg = float(line_str[84:86])
            dec_min = float(line_str[86:88])
            dec_sec = float(line_str[88:90])
            mag = float(line_str[102:107])
        except ValueError:
            continue

        # Output RA and Dec in degrees
        ra = (ra_hrs + (ra_min + (ra_sec / 60)) / 60) / 12 * 180.0
        dec = (dec_deg + (dec_min + (dec_sec / 60)) / 60) / 180 * 180.0

        # Fix the sign of the declination
        if dec_neg:
            dec = -dec

        # Create an information data structure describing this star
        stars_by_hd[hd_number] = {
            "hr_number": hr_number,
            "mag": mag,
            "ra": ra,
            "dec": dec
        }

    logging.info("Read the Bright Star Catalogue. Found {:d} entries.".format(len(stars_by_hd)))
    return stars_by_hd


def read_hipparcos_catalogue(stars_by_hd):
    stars_by_hip = {}
    for line_str in gzip.open("../downloads/hip_main.dat.gz", "rt"):
        try:
            hip_number = int(line_str[2: 14])
            hd_number = int(line_str[390:396])
        except ValueError:
            continue
        if hd_number in stars_by_hd:
            stars_by_hip[hip_number] = {
                **stars_by_hd[hd_number],
                "hd_number": hd_number
            }

    logging.info("Read the Hipparcos Catalogue. Found {:d} entries.".format(len(stars_by_hip)))
    return stars_by_hip


def fetch_star_coordinates(hip_number_str, stars_by_hip):
    star_in_neighbouring_constellation = hip_number_str.endswith('*')

    if star_in_neighbouring_constellation:
        hip_number_str = hip_number_str[:-1]

    hip_number_int = int(hip_number_str)

    try:
        star_description = stars_by_hip[hip_number_int]
    except KeyError:
        logging.info("No match to star HIP {:d}".format(hip_number_int))
        raise
    return {
        "hip_number_str": hip_number_str,
        "wrong_constellation": star_in_neighbouring_constellation,
        "hr_number": star_description['hr_number'],
        "ra": star_description['ra'],
        "dec": star_description['dec']
    }


def read_input_file(input_filename, stars_by_hip):
    # We preserve all the comment lines at the top of the input file, which we repeat at the top of the output files
    output_file_header = ""
    sticks = []
    current_constellation = "???"

    # Loop over the lines in the input file, which defines stick figures as a list of Hipparcos catalogue numbers
    with open(input_filename) as f:
        for line in f:
            line = line.strip()
            words = line.split()

            # Ignore blank lines
            if line == "":
                continue

            # The magic symbol ### tells us to insert a description of the specific format of the output file
            elif line == "###":
                output_file_header += "{format_specific_instructions}\n"

            # Ignore lines which start ##, which contain specific format of the input file
            elif line.startswith("##"):
                continue

            # Copy comment lines which start # into the output file(s)
            elif line.startswith("#"):
                output_file_header += "{}\n".format(line)

            # Lines which start * tell us we're starting a new constellation
            elif line.startswith("*"):
                current_constellation = words[1]

            # Lines which start [ contain JSON-formatted lists of the Hipparcos numbers of stars to connect
            elif line.startswith('['):
                star_hipparcos_numbers = json.loads(line)

                # Loop over the list of star numbers, adding a line between each consecutive pair of stars
                for index in range(1, len(star_hipparcos_numbers)):
                    sticks.append({
                        'constellation': current_constellation,
                        'line_start': fetch_star_coordinates(hip_number_str=star_hipparcos_numbers[index - 1],
                                                             stars_by_hip=stars_by_hip),
                        'line_end': fetch_star_coordinates(hip_number_str=star_hipparcos_numbers[index],
                                                           stars_by_hip=stars_by_hip),
                    })

            # We could not parse this line!
            else:
                raise ValueError("Could not read the line\n{}".format(line))

    return {
        'output_file_header': output_file_header,
        'sticks': sticks
    }


def convert_stick_figures():
    # Read the input catalogues we need
    stars_by_hd = read_bright_star_catalogue()
    stars_by_hip = read_hipparcos_catalogue(stars_by_hd=stars_by_hd)

    # Make sure that output directory exists
    os.system("mkdir -p output")
    os.system("rm -f output/*")

    # Process each input file in turn
    for input_file in input_files:
        input_data = read_input_file(input_filename=input_file['filename'],
                                     stars_by_hip=stars_by_hip)

        # Write each output file into each output format
        for output_format in output_formats:

            # Work out filename for the processed version of this file
            output_filename = "{}_{}.dat".format(input_file['output_stub'], output_format['name'])

            # Create output file
            with open(output_filename, "w") as f:

                # Write comment lines at the top of the file
                f.write("""
## {output_filename}

{comments}
""".format(output_filename=output_filename,
           comments=input_data['output_file_header'].format(
               format_specific_instructions=output_format['description'].strip()
           )
           ))

                # Add each stick to the file in turn, one stick on each line
                current_constellation = "???"
                for stick in input_data['sticks']:

                    # Leave a blank line each time we start a new constellation
                    if stick['constellation'] != current_constellation:
                        current_constellation = stick['constellation']
                        f.write("\n")

                    # If we're wrapping at RA=0, we may need to output this stick twice
                    stick_split = (stick,)

                    if output_format['wrap_at_ra_0']:
                        wrap_this_stick = (((stick['line_start']['ra'] > 270) and (stick['line_end']['ra'] < 90)) or
                                           ((stick['line_end']['ra'] > 270) and (stick['line_start']['ra'] < 90)))

                        if wrap_this_stick:
                            stick_split = [copy.deepcopy(stick) for i in range(2)]

                            while stick_split[0]['line_start']['ra'] > 270:
                                stick_split[0]['line_start']['ra'] -= 360
                            while stick_split[0]['line_end']['ra'] > 270:
                                stick_split[0]['line_end']['ra'] -= 360

                            while stick_split[0]['line_start']['ra'] < 90:
                                stick_split[0]['line_start']['ra'] += 360
                            while stick_split[0]['line_end']['ra'] < 90:
                                stick_split[0]['line_end']['ra'] += 360

                    # Write this stick to the output file
                    for item in stick_split:
                        f.write("{:18} ".format(current_constellation))
                        for star in ('line_start', 'line_end'):
                            for field in output_format['fields']:
                                value = item[star][field]
                                if isinstance(value, int):
                                    f.write("{:12d} ".format(value))
                                else:
                                    f.write("{:12f} ".format(value))
                        f.write("\n")


if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO,
                        stream=sys.stdout,
                        format='[%(asctime)s] %(levelname)s:%(filename)s:%(message)s',
                        datefmt='%d/%m/%Y %H:%M:%S')
    logger = logging.getLogger(__name__)
    logger.info(__doc__.strip())

    convert_stick_figures()
