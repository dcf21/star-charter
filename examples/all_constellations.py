#!/usr/bin/python3
# all_constellations.py
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
Plot star charts showing all 88 constellations in turn
"""

import logging
import os
import re
import sys
import time

# Template configuration file that we use to make all the star charts
template_configuration = """
CHART
output_filename=output/{}.png
ra_central={}
dec_central={}
angular_width=77.0
width=25.0
aspect=0.63
ra_dec_lines=1
constellation_boundaries=1
constellation_sticks=1
coords=ra_dec
projection=gnomonic
star_names=1
star_flamsteed_labels=0
constellation_names=1
mag_min=6.5
"""


def draw_constellation_charts():
    # Loop through each constellation in turn
    with open("../data/constellations/name_places.dat") as f:
        for constellation_description in f:
            # Ignore blank lines and comments
            if len(constellation_description.strip()) == 0 or constellation_description[0] == "#":
                continue

            # Extract constellation name and position
            constellation_description = constellation_description.split()

            constellation_name = constellation_description[0]
            constellation_ra = constellation_description[1]
            constellation_dec = constellation_description[2]

            # Strip out any naughty characters from the constellation name
            constellation_name = re.sub('[\W_]', '', constellation_name)

            # Logging update
            logging.info("Working on <{}>".format(constellation_name))

            # Create configuration file
            config_filename = "/tmp/constellation_chart.sch"

            with open(config_filename, "w") as out:
                out.write(template_configuration.format(constellation_name, constellation_ra, constellation_dec))

            # Make star chart
            os.system("../bin/starchart.bin {}".format(config_filename))

            # Sleep means CTRL-C works...
            time.sleep(0.1)


# Do it right away if we're run as a script
if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO,
                        stream=sys.stdout,
                        format='[%(asctime)s] %(levelname)s:%(filename)s:%(message)s',
                        datefmt='%d/%m/%Y %H:%M:%S')
    logger = logging.getLogger(__name__)
    logger.info(__doc__.strip())

    draw_constellation_charts()
