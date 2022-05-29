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
import sys
import time

# List of all constellations, and details of the plot to produce for each one
# Abbreviated name, full name, central RA (hr), central Dec (deg), angular width of field of view (deg)
constellations_list = (
    ('AND', 'Andromeda', 0.5, 37.0, 70),
    ('ANT', 'Antlia', 10.2, -33.0, 50),
    ('APS', 'Apus', 16.0, -75.0, 40),
    ('AQR', 'Aquarius', 22.1, -10, 67),
    ('AQL', 'Aquila', 19.5, 3.0, 60),
    ('ARA', 'Ara', 17.4, -57.0, 47),
    ('ARI', 'Aries', 2.5, 20.0, 50),
    ('AUR', 'Auriga', 5.9, 43.0, 60),
    ('BOO', 'Bootes', 15.0, 32.0, 77),
    ('CAE', 'Caelum', 4.75, -38.0, 45),
    ('CAM', 'Camelopardalis', 6.0, 76.0, 77),
    ('CNC', 'Cancer', 8.8, 21.0, 60),
    ('CVN', 'Canes Venatici', 13.0, 41.0, 60),
    ('CMA', 'Canis Major', 6.9, -23.0, 50),
    ('CMI', 'Canis Minor', 7.75, 9, 40),
    ('CAP', 'Capricornus', 21.0, -20.0, 55),
    ('CAR', 'Carina', 8.3, -63.0, 65),
    ('CAS', 'Cassiopeia', 1.0, 62.0, 65),
    ('CEN', 'Centaurus', 13.0, -49.0, 65),
    ('CEP', 'Cepheus', 22.0, 75.0, 70),
    ('CET', 'Cetus', 1.5, -5.0, 70),
    ('CHA', 'Chamaeleon', 10.2, -80.0, 40),
    ('CIR', 'Circinus', 14.4, -64.0, 40),
    ('COL', 'Columba', 6.0, -35.5, 40),
    ('COM', 'Coma Berenices', 12.75, 23.0, 50),
    ('CRA', 'Corona Australis', 18.4, -39.0, 40),
    ('CRB', 'Corona Borealis', 15.95, 34.0, 40),
    ('CRV', 'Corvus', 12.5, -18.0, 45),
    ('CRT', 'Crater', 11.4, -15.0, 45),
    ('CRU', 'Crux', 12.5, -60.0, 30),
    ('CYG', 'Cygnus', 20.5, 43.0, 60),
    ('DEL', 'Delphinus', 20.6, 12.0, 45),
    ('DOR', 'Dorado', 5.0, -60.0, 40),
    ('DRA', 'Draco', 15.2, 71.0, 77),
    ('EQU', 'Equuleus', 21.2, 8.0, 40),
    ('ERI', 'Eridanus', 3.8, -32.0, 92),
    ('FOR', 'Fornax', 2.9, -31.0, 45),
    ('GEM', 'Gemini', 7.0, 22.0, 55),
    ('GRU', 'Grus', 22.4, -46.0, 55),
    ('HER', 'Hercules', 17.3, 28.0, 77),
    ('HOR', 'Horologium', 3.1, -53, 60),
    ('HYA', 'Hydra', 11.4, -16.0, 105),
    ('HYI', 'Hydrus', 2.5, -70.0, 50),
    ('IND', 'Indus', 21.3, -60.0, 55),
    ('LAC', 'Lacerta', 22.5, 47.0, 55),
    ('LEO', 'Leo', 10.8, 15.0, 77),
    ('LMI', 'Leo Minor', 10.2, 32.0, 55),
    ('LEP', 'Lepus', 5.6, -16.0, 60),
    ('LIB', 'Libra', 15.2, -16.0, 60),
    ('LUP', 'Lupus', 15.2, -42, 58),
    ('LYN', 'Lynx', 7.8, 48.0, 60),
    ('LYR', 'Lyra', 18.8, 37.0, 55),
    ('MEN', 'Mensa', 5.5, -78.0, 50),
    ('MIC', 'Microscopium', 21.0, -36.0, 60),
    ('MON', 'Monoceros', 6.9, -2.0, 52),
    ('MUS', 'Musca', 12.6, -70.0, 45),
    ('NOR', 'Norma', 16.1, -50.0, 45),
    ('OCT', 'Octans', 21.5, -87.0, 60),
    ('OPH', 'Ophiuchus', 17.2, -7.0, 77),
    ('ORI', 'Orion', 5.5, 6.0, 60),
    ('PAV', 'Pavo', 19.3, -64.0, 55),
    ('PEG', 'Pegasus', 22.7, 19.0, 70),
    ('PER', 'Perseus', 3.2, 48.0, 60),
    ('PHE', 'Phoenix', 0.75, -50.0, 60),
    ('PIC', 'Pictor', 5.3, -54.0, 55),
    ('PSC', 'Pisces', 0.5, 15.0, 70),
    ('PSA', 'Piscis Austrinus', 22.3, -29.0, 50),
    ('PUP', 'Puppis', 7.2, -33.0, 77),
    ('PYX', 'Pyxis', 8.9, -27.0, 50),
    ('RET', 'Reticulum', 3.9, -60.0, 45),
    ('SGE', 'Sagitta', 19.6, 19.5, 40),
    ('SGR', 'Sagittarius', 18.8, -27.0, 72),
    ('SCO', 'Scorpius', 16.8, -28.0, 70),
    ('SCL', 'Sculptor', 0.4, -32.0, 55),
    ('SCT', 'Scutum', 18.8, -11.0, 38),
    ('SER1', 'Serpens Caput', 15.58, 11.0, 60),
    ('SER2', 'Serpens Cauda', 17.8, -7.0, 55),
    ('SEX', 'Sextans', 10.25, -2.0, 55),
    ('TAU', 'Taurus', 4.7, 16.0, 58),
    ('TEL', 'Telescopium', 19.3, -50.0, 45),
    ('TRI', 'Triangulum', 2.2, 34.0, 47),
    ('TRA', 'Triangulum Australe', 16.1, -64.0, 45),
    ('TUC', 'Tucana', 0.0, -65.0, 48),
    ('UMA', 'Ursa Major', 11.1, 52.0, 77),
    ('UMI', 'Ursa Minor', 15.0, 80.0, 55),
    ('VEL', 'Vela', 9.6, -47.0, 50),
    ('VIR', 'Virgo', 13.5, -5.0, 77),
    ('VOL', 'Volans', 7.8, -68.0, 40),
    ('VUL', 'Vulpecula', 20.2, 25.0, 52)
)

# Template configuration file that we use to make all the star charts
template_configuration = """
DEFAULTS
ra_central={ra}
dec_central={dec}
angular_width={angular_width}
width=25.0
aspect=0.63
ra_dec_lines=1
constellation_boundaries=1
constellation_sticks=1
constellation_highlight={constellation_abbrev}
coords=ra_dec
projection=gnomonic
star_names=1
star_flamsteed_labels=0
constellation_names=1
mag_min=6.5
mag_step=1
mag_alpha=1.3754439
dso_mag_min=12

CHART
output_filename={output_dir}/{output_filename}.png
"""


def draw_constellation_charts():
    os.system("mkdir -p output")

    # Loop through each constellation in turn
    for item in constellations_list:
        # Unpack constellation information tuple
        abbrev, name, ra, dec, fov = item

        # Logging update
        logging.info("Working on <{}>".format(name))

        # Create configuration file
        config_filename = "/tmp/constellation_chart.sch"

        with open(config_filename, "w") as out:
            out.write(template_configuration.format(
                output_dir="output",
                output_filename=name,
                constellation_abbrev=abbrev,
                ra=ra,
                dec=dec,
                angular_width=fov))

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
