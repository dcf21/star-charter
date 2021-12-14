#!/usr/bin/python3
# -*- coding: utf-8 -*-
# dataFetch.py
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
# along with StarCharter.  If not, see <https://www.gnu.org/licenses/>.
# -------------------------------------------------

"""
Automatically download all of the required data files from the internet.
"""

import argparse
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
    logging.info("Fetching <{}>".format(destination))

    # Check if the file already exists
    if os.path.exists(destination):
        if not force_refresh:
            logging.info("File already exists. Not downloading fresh copy.")
            return False
        else:
            logging.info("File already exists, but downloading fresh copy.")
            os.unlink(destination)

    # Check whether source URL is gzipped
    supplied_source_is_gzipped = web_address.endswith(".gz")
    target_is_gzipped = destination.endswith(".gz")

    # Try downloading file in both gzipped and uncompressed format, as CDS archive sometimes changes compression
    for source_is_gzipped in [supplied_source_is_gzipped, not supplied_source_is_gzipped]:
        # Make source source URL has the right suffix
        url = web_address
        if source_is_gzipped and not supplied_source_is_gzipped:
            url = web_address + ".gz"
        elif supplied_source_is_gzipped and not source_is_gzipped:
            url = web_address.strip(".gz")

        # Make sure file destination has the right suffix
        destination_download = destination
        if source_is_gzipped and not target_is_gzipped:
            destination_download = destination + ".gz"
        elif target_is_gzipped and not source_is_gzipped:
            destination_download = destination.strip(".gz")

        # Fetch the file with wget
        logging.info("Downloading <{}> to <{}>".format(url, destination_download))
        try:
            # It would be great to use Python's urllib here. But handling connection retries, catching 404 errors,
            # preserving file timestamps, etc, all has to be done manually. Oh, and if you want a progress bar...
            status = os.system("wget '{}' -O '{}'".format(url, destination_download))
            if status != 0:
                raise IOError("wget returned a non-zero status")
        except IOError:
            logging.info("wget returned a non-zero status")
            logging.info("Download failed; attempting alternative URLs")
            continue

        # Check that the file now exists
        if not (os.path.exists(destination_download) and os.path.getsize(destination_download) > 0):
            logging.info("Download failed; attempting alternative URLs")
            continue

        # Check that file is compressed or uncompressed, as required
        if source_is_gzipped and not target_is_gzipped:
            logging.info("Uncompressing to <{}>".format(destination))
            os.system("gunzip {}".format(destination_download))
        elif target_is_gzipped and not source_is_gzipped:
            logging.info("Compressing to <{}>".format(destination))
            os.system("gzip {}".format(destination_download))

        # Check that the file now exists
        if not os.path.exists(destination):
            raise IOError("Failed to create target file. Is gzip/gunzip installed?")

        # Success!
        return True

    # We didn't manage to download the file...
    raise IOError("Could not download file <{}>".format(web_address))


def fetch_required_files(refresh):
    """
    Fetch all of the files we require.

    :param refresh:
        Switch indicating whether to fetch fresh copies of any files we've already downloaded.
    :type refresh:
        bool
    :return:
    """
    # List of the files we require
    required_files = [
        # Definitions of constellation boundaries
        {
            'url': 'https://cdsarc.u-strasbg.fr/ftp/VI/49/bound_20.dat.gz',
            'destination': 'data/constellations/downloads/boundaries.dat',
            'force_refresh': refresh
        },
        {
            'url': 'https://cdsarc.u-strasbg.fr/ftp/VI/49/ReadMe',
            'destination': 'data/constellations/downloads/ReadMe',
            'force_refresh': refresh
        },

        # Yale Bright Star Catalog (copy for use in making constellation stick figures)
        {
            'url': 'https://cdsarc.u-strasbg.fr/ftp/V/50/catalog.gz',
            'destination': 'data/constellations/downloads/ybsc.gz',
            'force_refresh': refresh
        },

        # Hipparcos Catalog (copy for use in making constellation stick figures)
        {
            'url': 'https://cdsarc.u-strasbg.fr/ftp/I/239/hip_main.dat',
            'destination': 'data/constellations/downloads/hip_main.dat.gz',
            'force_refresh': refresh
        },

        # Axel Mellinger's Milky Way Panorama 2.0 (licensed for personal use only)
        {
            'url': 'https://galaxy.phy.cmich.edu/~axel/mwpan2/mwpan2_Merc_2000x1200.jpg',
            'destination': 'data/milkyWay/mwpan2_Merc_2000x1200.jpg',
            'force_refresh': refresh
        },

        # HD-DM-GC-HR-HIP-Bayer-Flamsteed Cross Index
        {
            'url': 'https://cdsarc.u-strasbg.fr/ftp/IV/27A/ReadMe',
            'destination': 'data/stars/bayerAndFlamsteed/ReadMe',
            'force_refresh': refresh
        },
        {
            'url': 'https://cdsarc.u-strasbg.fr/ftp/IV/27A/catalog.dat',
            'destination': 'data/stars/bayerAndFlamsteed/catalog.dat',
            'force_refresh': refresh
        },

        # Yale Bright Star Catalog
        {
            'url': 'https://cdsarc.u-strasbg.fr/ftp/V/50/ReadMe',
            'destination': 'data/stars/brightStars/ReadMe',
            'force_refresh': refresh
        },
        {
            'url': 'https://cdsarc.u-strasbg.fr/ftp/V/50/catalog.gz',
            'destination': 'data/stars/brightStars/catalog.gz',
            'force_refresh': refresh
        },

        # Gaia DR1
        {
            'url': 'https://cdsarc.u-strasbg.fr/ftp/I/337/ReadMe',
            'destination': 'data/stars/gaiaDR1/ReadMe',
            'force_refresh': refresh
        },
        {
            'url': 'https://cdsarc.u-strasbg.fr/ftp/I/337/tgas.dat.gz',
            'destination': 'data/stars/gaiaDR1/tgas.dat.gz',
            'force_refresh': refresh
        },

        # Tycho 1
        {
            'url': 'https://cdsarc.u-strasbg.fr/ftp/I/239/ReadMe',
            'destination': 'data/stars/tycho1/ReadMe',
            'force_refresh': refresh
        },
        {
            'url': 'https://cdsarc.u-strasbg.fr/ftp/I/239/tyc_main.dat',
            'destination': 'data/stars/tycho1/tyc_main.dat',
            'force_refresh': refresh
        },
        {
            'url': 'https://cdsarc.u-strasbg.fr/ftp/I/239/hip_main.dat',
            'destination': 'data/stars/tycho1/hip_main.dat.gz',
            'force_refresh': refresh
        },

        # Hipparcos catalogue new reduction
        {
            'url': 'https://cdsarc.u-strasbg.fr/ftp/I/311/ReadMe',
            'destination': 'data/stars/hipparcosNewReduction/ReadMe',
            'force_refresh': refresh
        },
        {
            'url': 'https://cdsarc.u-strasbg.fr/ftp/I/311/hip2.dat.gz',
            'destination': 'data/stars/hipparcosNewReduction/hip2.dat.gz',
            'force_refresh': refresh
        },

        # Tycho 2
        {
            'url': 'https://cdsarc.u-strasbg.fr/ftp/I/259/ReadMe',
            'destination': 'data/stars/tycho2/ReadMe',
            'force_refresh': refresh
        }
    ]

    # Fetch the various files which make up the Tycho 2 catalogue
    for file_number in range(20):
        required_files.append({
            'url': 'https://cdsarc.u-strasbg.fr/ftp/I/259/tyc2.dat.{:02d}.gz'.format(file_number),
            'destination': 'data/stars/tycho2/tyc2.dat.{:02d}.gz'.format(file_number),
            'force_refresh': refresh
        })

    # Fetch all the files
    for required_file in required_files:
        fetch_file(web_address=required_file['url'],
                   destination=required_file['destination'],
                   force_refresh=required_file['force_refresh']
                   )


if __name__ == "__main__":
    # Read command-line arguments
    parser = argparse.ArgumentParser(description=__doc__)

    # Add command-line options
    parser.add_argument('--refresh', dest='refresh', action='store_true', help='Download a fresh copy of all files.')
    parser.add_argument('--no-refresh', dest='refresh', action='store_false', help='Do not re-download existing files.')
    parser.set_defaults(refresh=False)
    args = parser.parse_args()

    # Set up logging
    logging.basicConfig(level=logging.INFO,
                        stream=sys.stdout,
                        format='[%(asctime)s] %(levelname)s:%(filename)s:%(message)s',
                        datefmt='%d/%m/%Y %H:%M:%S')
    logger = logging.getLogger(__name__)
    logger.info(__doc__.strip())

    # Fetch all the data
    fetch_required_files(refresh=args.refresh)
