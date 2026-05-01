#!/usr/bin/python3
# -*- coding: utf-8 -*-
# dataFetch.py
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
Automatically download all the required data files from the internet.
"""

import argparse
import glob
import os
import sys
import logging

from typing import Dict, List, Tuple, Union

# Data file numbering for each ASCII JPL ephemeris
de_ephemeris_file_specs: Dict[int, Tuple[int, int, int, int, str]] = {
    405: (1600, 2200, 20, 4, ""),
    430: (1550, 2550, 100, 4, "_572"),
    431: (1000, 16000, 1000, 5, "_572"),
    440: (1550, 2550, 100, 5, ""),
    441: (1000, 16000, 1000, 5, "")
}


def fetch_file(index: int, index_total: int, web_address: str, destination: str, force_refresh: bool = False) -> bool:
    """
    Download a file that we need, using wget.

    :param index:
        Counter of the files we have downloaded so far.
    :param index_total:
        Total number of files we will need to download.
    :param web_address:
        The URL that we should use to fetch the file
    :param destination:
        The path we should download the file to
    :param force_refresh:
        Boolean flag indicating whether we should download a new copy if the file already exists.
    :return:
        Boolean flag indicating whether the file was downloaded. Raises IOError if the download fails.
    """
    logging.info("({:3d}/{:3d}) Fetching <{}>".format(index, index_total, destination))

    # Check whether source URL is gzipped
    supplied_source_is_gzipped: bool = web_address.endswith(".gz")
    target_is_gzipped: bool = destination.endswith(".gz")

    # Check if the file already exists
    if os.path.isfile(destination) and os.path.getsize(destination) > 0:
        if not force_refresh:
            logging.info("File already exists. Not downloading fresh copy.")
            return False
        else:
            logging.info("File already exists, but downloading fresh copy.")

    # Clean out old copies of this file
    if os.path.isfile(destination):
        os.unlink(destination)
    if not target_is_gzipped:
        target_2: str = "{}.gz".format(destination)
        if os.path.isfile(target_2):
            os.unlink(target_2)
    else:
        target_2: str = destination[:-3]
        if os.path.isfile(target_2):
            os.unlink(target_2)

    # Try downloading file in both gzipped and uncompressed format, as CDS archive sometimes changes compression
    for source_is_gzipped in [supplied_source_is_gzipped, not supplied_source_is_gzipped]:
        # Make sure source URL has the right suffix
        url: str = web_address
        if source_is_gzipped and not supplied_source_is_gzipped:
            url = web_address + ".gz"
        elif supplied_source_is_gzipped and not source_is_gzipped:
            url = web_address.strip(".gz")

        # Make sure file destination has the right suffix
        destination_download: str = destination
        if source_is_gzipped and not target_is_gzipped:
            destination_download = destination + ".gz"
        elif target_is_gzipped and not source_is_gzipped:
            destination_download = destination.strip(".gz")

        # Ensure parent directory exists
        parent_dir: str = os.path.split(destination_download)[0]
        os.system("mkdir -p '{}'".format(parent_dir))

        # Fetch the file with wget
        logging.info("Downloading <{}> to <{}>".format(url, destination_download))
        try:
            # It would be great to use Python's urllib here. But handling connection retries, catching 404 errors,
            # preserving file timestamps, etc., all has to be done manually. Oh, and if you want a progress bar...
            status: int = os.system("wget '{}' -q -O '{}'".format(url, destination_download))
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


def list_de4xx_files(de_number: int, refresh: bool) -> List[Dict[str, str | bool]]:
    """
    Create a list of all the files we need to download for a single NASA JPL DE4xx ephemeris.

    :param de_number:
        The number of the ephemeris we are to fetch, e.g. 440 for DE440
    :param refresh:
        If true, we re-download files even if they are already present on disk.
    :return:
        A list of the required file downloads.
    """
    # List of the files we require
    required_files: List[Dict[str, str | bool]] = []

    # Select requested ephemeris
    assert de_number in de_ephemeris_file_specs, "Unknown JPL ephemeris <{}>".format(de_number)
    file_number_min: int = de_ephemeris_file_specs[de_number][0]
    file_number_max: int = de_ephemeris_file_specs[de_number][1]
    file_number_step: int = de_ephemeris_file_specs[de_number][2]
    file_number_width: int = de_ephemeris_file_specs[de_number][3]
    header_suffix: str = de_ephemeris_file_specs[de_number][4]

    # Fetch the JPL DE4xx ephemeris header files
    f: Dict[str, int | str] = {'de': de_number, 'suffix': header_suffix}
    required_files.append({
        'url': 'https://ssd.jpl.nasa.gov/ftp/eph/planets/ascii/de{de:3d}/header.{de:3d}{suffix:s}'.format(**f),
        'destination': 'data/de{de:3d}/header.{de:3d}'.format(**f),
        'force_refresh': refresh
    })

    # Fetch the JPL DE4xx ephemeris data files
    for file_number in range(file_number_min, file_number_max + 1, file_number_step):
        f: Dict[str, int] = {'de': de_number, 'num': file_number, 'width': file_number_width}
        required_files.append({
            'url': 'https://ssd.jpl.nasa.gov/ftp/eph/planets/ascii/de{de:3d}/ascp{num:0{width}d}.{de:3d}'.format(**f),
            'destination': 'data/de{de:3d}/ascp{num:0{width}d}.{de:3d}'.format(**f),
            'force_refresh': refresh
        })

    # Return list of files we need to fetch
    return required_files


def fetch_required_files(refresh: bool = False, never_refresh: bool = False,
                         selected_ephemeris: int = 440, all_ephemerides: bool = False) -> None:
    """
    Fetch all the files we require.

    :param refresh:
        Boolean switch indicating whether to fetch fresh copies of any files we've already downloaded.
    :param never_refresh:
        Boolean switch indicating we should never fetch fresh copies of any files we've already downloaded, even if
        newer versions exist online.
    :param selected_ephemeris:
        The number of the JPL DE4xx ephemeris to download, e.g. 440 for DE440
    :param all_ephemerides:
        Boolean switch to download the data files for all the JPL ephemerides
    :return:
        None
    """

    # Path to installation directory
    our_path: str = os.path.split(os.path.abspath(__file__))[0]

    # List of the files we require
    required_files: List[Dict[str, str | bool]] = [
        {
            'url': 'https://ftp.lowell.edu/pub/elgb/astorb.dat.gz',
            'destination': 'data/astorb.dat',
            'force_refresh': True
        },
        {
            'url': 'https://www.minorplanetcenter.net/iau/MPCORB/AllCometEls.txt',
            'destination': 'data/Soft00Cmt.txt',
            'force_refresh': True
        },

        # Definitions of constellation boundaries
        #       {
        #           'url': 'https://cdsarc.u-strasbg.fr/ftp/VI/49/bound_20.dat.gz',
        #           'destination': 'data/constellations/downloads/boundaries.dat',
        #           'force_refresh': refresh
        #       },
        #       {
        #           'url': 'https://cdsarc.u-strasbg.fr/ftp/VI/49/ReadMe',
        #           'destination': 'data/constellations/downloads/ReadMe',
        #           'force_refresh': refresh
        #       },

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

        # HD-DM-GC-HR-HIP-Bayer-Flamsteed Cross-Index
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
    file_number: int
    for file_number in range(20):
        required_files.append({
            'url': 'https://cdsarc.u-strasbg.fr/ftp/I/259/tyc2.dat.{:02d}.gz'.format(file_number),
            'destination': 'data/stars/tycho2/tyc2.dat.{:02d}.gz'.format(file_number),
            'force_refresh': refresh
        })

    # Fetch the JPL ephemerides
    for de_number in de_ephemeris_file_specs.keys():
        if de_number == selected_ephemeris or all_ephemerides:
            required_files.extend(list_de4xx_files(de_number=de_number, refresh=refresh))

    # Fetch all the files
    for index, required_file in enumerate(required_files):
        force_refresh: bool = required_file['force_refresh'] if not never_refresh else False
        fetch_file(index=index + 1, index_total=len(required_files),
                   web_address=required_file['url'],
                   destination=os.path.join(our_path, required_file['destination']),
                   force_refresh=force_refresh
                   )

    # Create a list of all the files containing orbital elements for planets
    planet_elements: List[str] = glob.glob("data/planets*.dat")
    with open("data/list_planet_files.txt", "wt") as f_out:
        f_out.write("# List of files containing orbital elements for planets\n")
        for item in sorted(planet_elements):
            f_out.write("{}\n".format(os.path.abspath(item)))

    # Create a list of all the files containing orbital elements for planets
    asteroid_elements: List[str] = (["data/astorb.dat"] +
                                    glob.glob(os.path.expanduser("~/astorb_archive/astorb_*.dat")))
    with open("data/list_astorb_files.txt", "wt") as f_out:
        f_out.write("# List of files containing orbital elements for asteroids\n")
        for item in sorted(asteroid_elements):
            f_out.write("{}\n".format(os.path.abspath(item)))

    # Create a list of all the files containing orbital elements for planets
    comet_elements: List[str] = (["data/Soft00Cmt.txt"] +
                                 glob.glob(os.path.expanduser("~/astorb_archive/Soft00Cmt_*.dat*")))
    with open("data/list_comet_files.txt", "wt") as f_out:
        f_out.write("# List of files containing orbital elements for comets\n")
        for item in sorted(comet_elements):
            f_out.write("{}\n".format(os.path.abspath(item)))


if __name__ == "__main__":
    # Read command-line arguments
    parser = argparse.ArgumentParser(description=__doc__)

    # Add command-line options
    parser.add_argument('--refresh', dest='refresh', action='store_true',
                        help='Download a fresh copy of all files.')
    parser.add_argument('--no-refresh', dest='refresh', action='store_false',
                        help='Do not re-download all existing files.')
    parser.add_argument('--never-refresh', dest='never_refresh', action='store_true',
                        help='Never re-download existing files, even if newer versions exist online.')
    parser.add_argument('--ephemeris', dest='ephemeris', type=int, default=440,
                        choices=de_ephemeris_file_specs.keys(),
                        help='Select which NASA JPL DE4xx ephemeris should be downloaded (440 by default).')
    parser.add_argument('--all-ephemerides', dest='fetch_all', action='store_true',
                        help='Download all NASA JPL DE4xx ephemerides.')
    parser.set_defaults(refresh=False)
    parser.set_defaults(never_refresh=False)
    parser.set_defaults(fetch_all=False)
    args = parser.parse_args()

    # Set up logging
    logging.basicConfig(level=logging.INFO,
                        stream=sys.stdout,
                        format='[%(asctime)s] %(levelname)s:%(filename)s:%(message)s',
                        datefmt='%d/%m/%Y %H:%M:%S')
    logging.info(__doc__.strip())

    # Fetch all the data
    fetch_required_files(refresh=args.refresh, never_refresh=args.never_refresh,
                         selected_ephemeris=args.ephemeris, all_ephemerides=args.fetch_all)
