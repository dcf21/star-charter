# -*- coding: utf-8 -*-
# star_name_conversion.py
#
# -------------------------------------------------
# Copyright 2015-2025 Dominic Ford
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
Functions to convert star names between string formats.
"""

import re

from typing import Dict, List, Tuple


class StarNameConversion:
    # Convert Greek letters (i.e. Bayer designations of stars) into TeX
    greek_alphabet_tex_lookup: Dict[str, str] = {
        # Abbreviations used in Bright Star catalog
        'Alp': r'\alpha', 'Bet': r'\beta', 'Gam': r'\gamma', 'Del': r'\delta', 'Eps': r'\epsilon',
        'Zet': r'\zeta', 'Eta': r'\eta', 'The': r'\theta', 'Iot': r'\iota', 'Kap': r'\kappa',
        'Lam': r'\lambda', 'Mu': r'\mu', 'Nu': r'\nu', 'Xi': r'\xi', 'Omi': 'O', 'Pi': r'\pi',
        'Rho': r'\rho', 'Sig': r'\sigma', 'Tau': r'\tau', 'Ups': r'\upsilon', 'Phi': r'\phi',
        'Chi': r'\chi', 'Psi': r'\psi', 'Ome': r'\omega',

        # Abbreviations used in cross-index catalog
        'alf': r'\alpha', 'bet': r'\beta', 'gam': r'\gamma', 'del': r'\delta', 'eps': r'\epsilon',
        'zet': r'\zeta', 'eta': r'\eta', 'the': r'\theta', 'iot': r'\iota', 'kap': r'\kappa',
        'lam': r'\lambda', 'mu.': r'\mu', 'nu.': r'\nu', 'ksi': r'\xi', 'omi': 'O', 'pi.': r'\pi',
        'rho': r'\rho', 'sig': r'\sigma', 'tau': r'\tau', 'ups': r'\upsilon', 'phi': r'\phi',
        'chi': r'\chi', 'psi': r'\psi', 'ome': r'\omega'
    }

    # Convert Greek letters (i.e. Bayer designations of stars) into HTML
    greek_alphabet_html_lookup: List[Tuple[str, str]] = [
        (r"\\_", "_"), (r"\^1", "&#x00B9;"), (r"\^2", "&#x00B2;"), (r"\^3", "&#x00B3;"),
        (r"\^4", "&#x2074;"), (r"\^5", "&#x2075;"), (r"\^6", "&#x2076;"), (r"\^7", "&#x2077;"),
        (r"\^8", "&#x2078;"), (r"\^9", "&#x2079;"),
        (r"\\alpha", "&alpha;"), (r"\\beta", "&beta;"), (r"\\gamma", "&gamma;"),
        (r"\\delta", "&delta;"), (r"\\epsilon", "&epsilon;"), (r"\\zeta", "&zeta;"),
        (r"\\eta", "&eta;"), (r"\\theta", "&theta;"), (r"\\iota", "&iota;"), (r"\\kappa", "&kappa;"),
        (r"\\lambda", "&lambda;"), (r"\\mu", "&mu;"), (r"\\nu", "&nu;"), (r"\\xi", "&xi;"),
        (r"\\pi", "&pi;"), (r"\\rho", "&rho;"), (r"\\sigma", "&sigma;"), (r"\\tau", "&tau;"),
        (r"\\upsilon", "&upsilon;"), (r"\\phi", "&phi;"), (r"\\chi", "&chi;"), (r"\\psi", "&psi;"),
        (r"\\omega", "&omega;"), (r"\$", "")
    ]

    # Convert Greek letters (i.e. Bayer designations of stars) into ASCII
    greek_alphabet_ascii_lookup: List[Tuple[str, str]] = [
        (r"\\_", "_"), (r"\^1", "1"), (r"\^2", "2"), (r"\^3", "3"), (r"\^4", "4"),
        (r"\^5", "5"), (r"\^6", "6"), (r"\^7", "7"), (r"\^8", "8"), (r"\^9", "9"), (r"\\alpha", "Alpha"),
        (r"\\beta", "Beta"), (r"\\gamma", "Gamma"), (r"\\delta", "Delta"), (r"\\epsilon", "Epsilon"),
        (r"\\zeta", "Zeta"), (r"\\eta", "Eta"), (r"\\theta", "Theta"), (r"\\iota", "Iota"),
        (r"\\kappa", "Kappa"), (r"\\lambda", "Lambda"), (r"\\mu", "Mu"), (r"\\nu", "Nu"),
        (r"\\xi", "Xi"), (r"\\pi", "Pi"), (r"\\rho", "Rho"), (r"\\sigma", "Sigma"), (r"\\tau", "Tau"),
        (r"\\upsilon", "Upsilon"), (r"\\phi", "Phi"), (r"\\chi", "Chi"), (r"\\psi", "Psi"),
        (r"\\omega", "Omega"), (r"\$", ""),
        ("^O$", "Omicron"), ("^O1$", "Omicron1"), ("^O2$", "Omicron2")
    ]

    # Convert Greek letters from HTML entities into UTF8 characters
    greek_html_to_utf8_lookup: List[Tuple[str, str]] = [
        (r"&#x00B9;", "\u00B9"), (r"&#x00B2;", "\u00B2"), (r"&#x00B3;", "\u00B3"),
        (r"&#x2074;", "\u2074"), (r"&#x2075;", "\u2075"), (r"&#x2076;", "\u2076"),
        (r"&#x2077;", "\u2077"), (r"&#x2078;", "\u2078"), (r"&#x2079;", "\u2079"),
        (r"&alpha;", "\u03B1"), (r"&beta;", "\u03B2"), (r"&gamma;", "\u03B3"),
        (r"&delta;", "\u03B4"), (r"&epsilon;", "\u03B5"), (r"&zeta;", "\u03B6"), (r"&eta;", "\u03B7"),
        (r"&theta;", "\u03B8"), (r"&iota;", "\u03B9"), (r"&kappa;", "\u03BA"), (r"&lambda;", "\u03BB"),
        (r"&mu;", "\u03BC"), (r"&nu;", "\u03BD"), (r"&xi;", "\u03BE"), (r"&pi;", "\u03C0"),
        (r"&rho;", "\u03C1"), (r"&sigma;", "\u03C3"), (r"&tau;", "\u03C4"), (r"&upsilon;", "\u03C5"),
        (r"&phi;", "\u03C6"), (r"&chi;", "\u03C7"), (r"&psi;", "\u03C8"), (r"&omega;", "\u03C9")
    ]

    def greek_html_to_utf8(self, in_str: str) -> str:
        """
        Convert HTML entities for Greek letters into UTF8 characters

        :param in_str:
            The name of an object, with Greek letters encoded as HTML entities
        :return:
            UTF8 encoded string
        """
        for letter in self.greek_html_to_utf8_lookup:
            in_str = re.sub(letter[0], letter[1], in_str)
        return in_str.split("-")[0]

    def name_to_html(self, in_str: str) -> str:
        """
        Convert the name of an object into HTML, by substituting any greek letters with HTML entities.

        :param in_str:
            Name of object as reported in catalogue
        :return:
            HTML name of object
        """

        if in_str == "":
            return ""
        for subst in self.greek_alphabet_html_lookup:
            in_str = re.sub(subst[0], subst[1], in_str)
        return in_str

    def name_to_ascii(self, in_str: str) -> str:
        """
        Convert the name of an object into ASCII, by substituting any abbreviated greek letters with full names.

        :param in_str:
            Name of object as reported in catalogue
        :return:
            ASCII name of object
        """

        if in_str == "":
            return ""
        for subst in self.greek_alphabet_ascii_lookup:
            in_str = re.sub(subst[0], subst[1], in_str)
        return in_str
