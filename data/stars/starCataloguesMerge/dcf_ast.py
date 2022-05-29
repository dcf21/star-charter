# -*- coding: utf-8 -*-
# dcf_ast.py
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
Various astronomical helper functions
"""

from math import floor, fmod, pi, sin, cos, tan

# The day of the year on which each month begins
month_day = [0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 999]

# The three-letter names of each month of the year
month_name = ["Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"]

# The full names of each month of the year
month_name_full = ["January", "February", "March", "April", "May", "June",
                   "July", "August", "September", "October", "November", "December"]


def julian_day(year, month, day, hour=0, minute=0, sec=0):
    """
    Convert a calendar date into a Julian Date.

    :param year:
        Integer year number.

    :type year:
        int

    :param month:
        Integer month number (1-12)

    :type month:
        int

    :param day:
        Integer day of month (1-31)

    :type day:
        int

    :param hour:
        Integer hour of day (0-23)

    :type hour:
        int

    :param minute:
        Integer minutes past the hour (0-59)

    :type minute:
        int

    :param sec:
        Floating point seconds past minute (0-60)

    :type sec:
        float

    :return:
        float Julian date
    """
    last_julian = 15821209.0
    first_gregorian = 15821220.0
    req_date = 10000.0 * year + 100 * month + day

    if month <= 2:
        month += 12
        year -= 1

    if req_date <= last_julian:
        b = -2 + floor((year + 4716) / 4) - 1179  # Julian calendar
    elif req_date >= first_gregorian:
        b = floor(year / 400) - floor(year / 100) + floor(year / 4)  # Gregorian calendar
    else:
        raise IndexError("The requested date never happened")

    jd = 365.0 * year - 679004.0 + 2400000.5 + b + floor(30.6001 * (month + 1)) + day
    day_fraction = (int(hour) + int(minute) / 60.0 + sec / 3600.0) / 24.0
    return jd + day_fraction


def inv_julian_day(jd):
    """
    Convert a Julian date into a calendar date.

    :param jd:
        Julian date

    :type jd:
        float

    :return:
        Calendar date
    """
    day_fraction = (jd + 0.5) - floor(jd + 0.5)
    hour = int(floor(24 * day_fraction))
    minute = int(floor(fmod(1440 * day_fraction, 60)))
    sec = fmod(86400 * day_fraction, 60)

    # Number of whole Julian days. b = Number of centuries since the Council of Nicaea.
    # c = Julian Day number as if century leap years happened.
    a = int(jd + 0.5)
    if a < 2361222.0:
        c = int(a + 1524)  # Julian calendar
    else:
        b = int((a - 1867216.25) / 36524.25)
        c = int(a + b - (b // 4) + 1525)  # Gregorian calendar
    d = int((c - 122.1) / 365.25)  # Number of 365.25 periods, starting the year at the end of February
    e_ = int(365 * d + d // 4)  # Number of days accounted for by these
    f = int((c - e_) / 30.6001)  # Number of 30.6001 days periods (a.k.a. months) in remainder
    day = int(floor(c - e_ - int(30.6001 * f)))
    month = int(floor(f - 1 - 12 * (f >= 14)))
    year = int(floor(d - 4715 - int(month >= 3)))
    return [year, month, day, hour, minute, sec]


def date_string(utc):
    """
    Create a human-readable date from a unix time.

    :param utc:
        Unix time

    :type utc:
        float

    :return:
        Human-readable string
    """
    jd = jd_from_unix(utc)
    x = inv_julian_day(jd)
    return "{:02d}/{:02d}/{:04d} {:02d}:{:02d}".format(x[2], x[1], x[0], x[3], x[4])


# Returns a Unix timestamp from a Julian Day number
def unix_from_jd(jd):
    """
    Convert a Julian date into a unix time.

    :param jd:
        Julian date

    :type jd:
        float

    :return:
        Float unix time
    """
    return 86400.0 * (jd - 2440587.5)


def jd_from_unix(utc):
    """
    Convert a unix time into a Julian date.

    :param utc:
        Unix time

    :type utc:
        float

    :return:
        Float Julian date
    """
    return (utc / 86400.0) + 2440587.5


def sidereal_time(utc):
    """
    Turns a unix time into a sidereal time (in hours, at Greenwich)

    :param utc:
        Unix time

    :type utc:
        float

    :return:
        float, sidereal time in hours
    """
    u = utc
    j = 40587.5 + u / 86400.0  # Julian date - 2400000
    t = (j - 51545.0) / 36525.0  # Julian century (no centuries since 2000.0)
    st = ((
                  280.46061837 +
                  360.98564736629 * (j - 51545.0) +  # See pages 87-88 of Astronomical Algorithms, by Jean Meeus
                  0.000387933 * t * t +
                  t * t * t / 38710000.0
          ) % 360) * 12 / 180
    return st  # sidereal time, in hours. RA at zenith in Greenwich.


def ra_dec_from_j2000(ra0, dec0, utc_new):
    """
    Convert celestial coordinates from J2000 into a new epoch. See Green's Spherical Astronomy, pp 222-225

    :param ra0:
        Right ascension, in hours, J2000

    :type ra0:
        float

    :param dec0:
        Declination, in degrees, J2000

    :type dec0:
        float

    :param utc_new:
        Unix time of the epoch we are to transform celestial coordinates into

    :type utc_new:
        float

    :return:
        List of [RA, Dec] in hours and degrees, new epoch
    """
    ra0 *= pi / 12
    dec0 *= pi / 180

    u = utc_new
    j = 40587.5 + u / 86400.0  # Julian date - 2400000
    t = (j - 51545.0) / 36525.0  # Julian century (no centuries since 2000.0)

    deg = pi / 180
    m = (1.281232 * t + 0.000388 * t * t) * deg
    n = (0.556753 * t + 0.000119 * t * t) * deg

    ra_m = ra0 + 0.5 * (m + n * sin(ra0) * tan(dec0))
    dec_m = dec0 + 0.5 * n * cos(ra_m)

    ra_new = ra0 + m + n * sin(ra_m) * tan(dec_m)
    dec_new = dec0 + n * cos(ra_m)

    return [ra_new * 12 / pi, dec_new * 180 / pi]


def ra_dec_to_j2000(ra1, dec1, utc_old):
    """
    Convert celestial coordinates to J2000 from another epoch. See Green's Spherical Astronomy, pp 222-225

    :param ra1:
        Right ascension, in hours, original epoch

    :type ra1:
        float

    :param dec1:
        Declination, in degrees, original epoch

    :type dec1:
        float

    :param utc_old:
        Unix time of the epoch we are to transform celestial coordinates from

    :type utc_old:
        float

    :return:
        List of [RA, Dec] in hours and degrees, J2000
    """
    ra1 *= pi / 12
    dec1 *= pi / 180

    u = utc_old
    j = 40587.5 + u / 86400.0  # Julian date - 2400000
    t = (j - 51545.0) / 36525.0  # Julian century (no centuries since 2000.0)

    deg = pi / 180
    m = (1.281232 * t + 0.000388 * t * t) * deg
    n = (0.556753 * t + 0.000119 * t * t) * deg

    ra_m = ra1 - 0.5 * (m + n * sin(ra1) * tan(dec1))
    dec_m = dec1 - 0.5 * n * cos(ra_m)

    ra_new = ra1 - m - n * sin(ra_m) * tan(dec_m)
    dec_new = dec1 - n * cos(ra_m)

    return [ra_new * 12 / pi, dec_new * 180 / pi]


def ra_dec_switch_epoch(ra0, dec0, utc_old, utc_new):
    """
    Convert celestial coordinates from one epoch into a new epoch. See Green's Spherical Astronomy, pp 222-225

    :param ra0:
        Right ascension, in hours, original epoch

    :type ra0:
        float

    :param dec0:
        Declination, in degrees, original epoch

    :type dec0:
        float

    :param utc_old:
        Unix time of the epoch we are to transform celestial coordinates from

    :type utc_old:
        float

    :param utc_new:
        Unix time of the epoch we are to transform celestial coordinates into

    :type utc_new:
        float

    :return:
        List of [RA, Dec] in hours and degrees, new epoch
    """
    [ra_j2000, dec_j2000] = ra_dec_to_j2000(ra0, dec0, utc_old)
    return ra_dec_from_j2000(ra_j2000, dec_j2000, utc_new)


def ra_dec_j2000_from_b1950(ra0, dec0):
    """
    Convert celestial coordinates from B1950 into J2000.

    :param ra0:
        Right ascension, in hours, B1950

    :type ra0:
        float

    :param dec0:
        Declination, in degrees, B1950

    :type dec0:
        float

    :return:
        List of [RA, Dec] in hours and degrees, J2000
    """
    return ra_dec_to_j2000(ra0, dec0, -631158660)


def ra_dec_b1950_from_j2000(ra0, dec0):
    """
    Convert celestial coordinates from J2000 into B1950.

    :param ra0:
        Right ascension, in hours, J2000

    :type ra0:
        float

    :param dec0:
        Declination, in degrees, J2000

    :type dec0:
        float

    :return:
        List of [RA, Dec] in hours and degrees, B1950
    """
    return ra_dec_from_j2000(ra0, dec0, -631158660)
