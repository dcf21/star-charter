#!/usr/bin/python3
# -*- coding: utf-8 -*-
# dataFetch.py
#
# -------------------------------------------------
# Copyright 2015-2020 Dominic Ford
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
            'url': 'http://cdsarc.u-strasbg.fr/ftp/VI/49/bound_20.dat.gz',
            'destination': 'data/constellations/downloads/boundaries.dat.gz',
            'force_refresh': False
        },
        {
            'url': 'http://cdsarc.u-strasbg.fr/ftp/VI/49/ReadMe',
            'destination': 'data/constellations/downloads/ReadMe',
            'force_refresh': False
        },

        # Yale Bright Star Catalog (copy for use in making constellation stick figures)
        {
            'url': 'http://cdsarc.u-strasbg.fr/ftp/V/50/catalog.gz',
            'destination': 'data/constellations/downloads/ybsc.gz',
            'force_refresh': False
        },

        # Hipparcos Catalog (copy for use in making constellation stick figures)
        {
            'url': 'ftp://cdsarc.u-strasbg.fr/pub/cats/I/239/hip_main.dat.gz',
            'destination': 'data/constellations/downloads/hip_main.dat.gz',
            'force_refresh': False
        },

        # Axel Mellinger's Milky Way Panorama 2.0 (licensed for personal use only)
        {
            'url': 'http://galaxy.phy.cmich.edu/~axel/mwpan2/mwpan2_Merc_2000x1200.jpg',
            'destination': 'data/milkyWay/mwpan2_Merc_2000x1200.jpg',
            'force_refresh': False
        },

        # HD-DM-GC-HR-HIP-Bayer-Flamsteed Cross Index
        {
            'url': 'http://cdsarc.u-strasbg.fr/ftp/IV/27A/ReadMe',
            'destination': 'data/stars/bayerAndFlamsteed/ReadMe',
            'force_refresh': False
        },
        {
            'url': 'http://cdsarc.u-strasbg.fr/ftp/IV/27A/catalog.dat',
            'destination': 'data/stars/bayerAndFlamsteed/catalog.dat',
            'force_refresh': False
        },

        # Yale Bright Star Catalog
        {
            'url': 'http://cdsarc.u-strasbg.fr/ftp/V/50/ReadMe',
            'destination': 'data/stars/brightStars/ReadMe',
            'force_refresh': False
        },
        {
            'url': 'http://cdsarc.u-strasbg.fr/ftp/V/50/catalog.gz',
            'destination': 'data/stars/brightStars/catalog.gz',
            'force_refresh': False
        },

        # Gaia DR1
        {
            'url': 'http://cdsarc.u-strasbg.fr/ftp/I/337/ReadMe',
            'destination': 'data/stars/gaiaDR1/ReadMe',
            'force_refresh': False
        },
        {
            'url': 'http://cdsarc.u-strasbg.fr/ftp/I/337/tgas.dat.gz',
            'destination': 'data/stars/gaiaDR1/tgas.dat.gz',
            'force_refresh': False
        },

        # Tycho 1
        {
            'url': 'http://cdsarc.u-strasbg.fr/ftp/I/239/ReadMe',
            'destination': 'data/stars/tycho1/ReadMe',
            'force_refresh': False
        },
        {
            'url': 'ftp://cdsarc.u-strasbg.fr/pub/cats/I/239/tyc_main.dat',
            'destination': 'data/stars/tycho1/tyc_main.dat',
            'force_refresh': False
        },
        {
            'url': 'ftp://cdsarc.u-strasbg.fr/pub/cats/I/239/hip_main.dat.gz',
            'destination': 'data/stars/tycho1/hip_main.dat.gz',
            'force_refresh': False
        },

        # Hipparcos catalogue new reduction
        {
            'url': 'http://cdsarc.u-strasbg.fr/ftp/I/311/ReadMe',
            'destination': 'data/stars/hipparcosNewReduction/ReadMe',
            'force_refresh': False
        },
        {
            'url': 'http://cdsarc.u-strasbg.fr/ftp/I/311/hip2.dat.gz',
            'destination': 'data/stars/hipparcosNewReduction/hip2.dat.gz',
            'force_refresh': False
        },


        # Tycho 2
        {
            'url': 'http://cdsarc.u-strasbg.fr/ftp/I/259/ReadMe',
            'destination': 'data/stars/tycho2/ReadMe',
            'force_refresh': False
        }
    ]

    # Fetch the various files which make up the Tycho 2 catalogue
    for file_number in range(20):
        required_files.append({
            'url': 'ftp://cdsarc.u-strasbg.fr/pub/cats/I/259/tyc2.dat.{:02d}.gz'.format(file_number),
            'destination': 'data/stars/tycho2/tyc2.dat.{:02d}.gz'.format(file_number),
            'force_refresh': False
        })

    # Fetch all the files
    for required_file in required_files:
        fetch_file(web_address=required_file['url'],
                   destination=required_file['destination'],
                   force_refresh=required_file['force_refresh']
                   )

    # Unzip the constellation boundaries
    os.system("gunzip -f data/constellations/downloads/boundaries.dat.gz")



if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO,
                        stream=sys.stdout,
                        format='[%(asctime)s] %(levelname)s:%(filename)s:%(message)s',
                        datefmt='%d/%m/%Y %H:%M:%S')
    logger = logging.getLogger(__name__)
    logger.info(__doc__.strip())

    fetch_required_files()
