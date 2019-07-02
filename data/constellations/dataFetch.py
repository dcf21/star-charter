#!/usr/bin/python3
# -*- coding: utf-8 -*-
# dataFetch.py
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
Automatically download all of the required data files from the internet.
"""

import os
import sys
import logging


def fetch_file(web_address, destination, force_refresh=False):
    """
    Download a file that we need, using wget.

    :param web_address:
        The URL that we should use to fetch the file
    :type web_address:
        str
    :param destination:
        The path we should download the file to
    :type destination:
        str
    :param force_refresh:
        Boolean flag indicating whether we should download a new copy if the file already exists.
    :type force_refresh:
        bool
    :return:
        Boolean flag indicating whether the file was downloaded. Raises IOError if the download fails.
    """
    logging.info("Fetching file <{}>".format(destination))

    # Check if the file already exists
    if os.path.exists(destination):
        if not force_refresh:
            logging.info("File already exists. Not downloading fresh copy.")
            return False
        else:
            logging.info("File already exists, but downloading fresh copy.")
            os.unlink(destination)

    # Fetch the file with wget
    os.system("wget -q '{}' -O {}".format(web_address, destination))

    # Check that the file now exists
    if not os.path.exists(destination):
        raise IOError("Could not download file <{}>".format(web_address))

    return True


def fetch_required_files():
    # List of the files we require
    required_files = [
        # Definitions of constellation boundaries
        {
            'url': 'http://cdsarc.u-strasbg.fr/ftp/VI/49/bound_20.dat',
            'destination': 'downloads/boundaries.dat',
            'force_refresh': False
        },
        {
            'url': 'http://cdsarc.u-strasbg.fr/ftp/VI/49/ReadMe',
            'destination': 'downloads/ReadMe',
            'force_refresh': False
        },

        # Yale Bright Star Catalog
        {
            'url': 'http://cdsarc.u-strasbg.fr/ftp/V/50/catalog.gz',
            'destination': 'downloads/ybsc.gz',
            'force_refresh': False
        },

        # Hipparcos Catalog
        {
            'url': 'ftp://cdsarc.u-strasbg.fr/pub/cats/I/239/hip_main.dat.gz',
            'destination': 'downloads/hip_main.dat.gz',
            'force_refresh': False
        }
    ]

    # Fetch all the files
    for required_file in required_files:
        fetch_file(web_address=required_file['url'],
                   destination=required_file['destination'],
                   force_refresh=required_file['force_refresh']
                   )


if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO,
                        stream=sys.stdout,
                        format='[%(asctime)s] %(levelname)s:%(filename)s:%(message)s',
                        datefmt='%d/%m/%Y %H:%M:%S')
    logger = logging.getLogger(__name__)
    logger.info(__doc__.strip())

    fetch_required_files()
