#!/usr/bin/python3
# -*- coding: utf-8 -*-
# runExamples.py
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
# along with StarCharter.  If not, see <https://www.gnu.org/licenses/>.
# -------------------------------------------------

"""
Generate a set of sample charts. This is a useful thing to do immediately post-installation, as it triggers
StarCharter to convert its ASCII input ephemeris files into binary format, which can quickly be accessed for subsequent
queries.
"""

import argparse
import os
import sys
import logging

from typing import Dict, Final, List, Tuple

# Data file numbering for each ASCII JPL ephemeris
de_ephemeris_file_specs: Final[Dict[int, Tuple[int, int, int, int, str]]] = {
    405: (1600, 2200, 20, 4, ""),
    430: (1550, 2550, 100, 4, "_572"),
    431: (1000, 16000, 1000, 5, "_572"),
    440: (1550, 2550, 100, 5, ""),
    441: (1000, 16000, 1000, 5, "")
}

# Example StarCharter configuration files to run as part of the demo.
script_list: List[str] = ['ephemerides.sch']


def run_demos(selected_ephemeris: int = 440, all_ephemerides: bool = False) -> None:
    """
    Generate a set of sample charts. This is a useful thing to do immediately post-installation, as it triggers
    StarCharter to convert its ASCII input ephemeris files into binary format, which can quickly be accessed for
    subsequent queries.

    :param selected_ephemeris:
        The number of the JPL DE4xx ephemeris to download, e.g. 440 for DE440
    :param all_ephemerides:
        Boolean switch to generate demos using all the JPL ephemerides
    :return:
        None
    """

    # Path to EphemerisCompute binary
    our_path: Final[str] = os.path.split(os.path.abspath(__file__))[0]
    binary_path: Final[str] = os.path.join(our_path, "bin", "starchart.bin")
    examples_path: Final[str] = os.path.join(our_path, "examples")
    output_path: Final[str] = os.path.join(examples_path, "output")

    # List of the JPL DE4xx ephemerides we are to use
    ephemeris_list: List[int] = [
        de_number
        for de_number in de_ephemeris_file_specs.keys()
        if de_number == selected_ephemeris or all_ephemerides
    ]

    # Run the demos
    de_number: int
    script_name: str
    for de_number in ephemeris_list:
        for script_name in script_list:
            command: str = """
cd '{}' ; mkdir -p '{}' ; {} -d {:d} '{:s}'
""".format(examples_path, output_path, binary_path, de_number, script_name).strip()
            logging.info("Running <{}>".format(command))
            os.system(command)


if __name__ == "__main__":
    # Read command-line arguments
    parser = argparse.ArgumentParser(description=__doc__)

    # Add command-line options
    parser.add_argument('--ephemeris', dest='ephemeris', type=int, default=440,
                        choices=de_ephemeris_file_specs.keys(),
                        help='Select which NASA JPL DE4xx ephemeris should be used (440 by default).')
    parser.add_argument('--all-ephemerides', dest='fetch_all', action='store_true',
                        help='Generate sample charts from all NASA JPL DE4xx ephemerides.')
    parser.set_defaults(fetch_all=False)
    args = parser.parse_args()

    # Set up logging
    logging.basicConfig(level=logging.INFO,
                        stream=sys.stdout,
                        format='[%(asctime)s] %(levelname)s:%(filename)s:%(message)s',
                        datefmt='%d/%m/%Y %H:%M:%S')
    logging.info(__doc__.strip())

    # Run the demo ephemerides
    run_demos(selected_ephemeris=args.ephemeris, all_ephemerides=args.fetch_all)
