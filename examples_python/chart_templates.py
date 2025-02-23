# -*- coding: utf-8 -*-
# chart_templates.py

"""
StarCharter template settings.
"""

from typing import Any, Dict, Final, List, Optional, Sequence, Tuple, Union

from dcf_ast import inv_julian_day, month_name, month_name_full

colour_scheme: Final[str] = 'pastel'

colour_schemes: Final[Dict[str, Dict[str, dict]]] = {
    'dark': {
        'global_settings': {
            'constellation_stick_col': '0,0.4,0',
            'constellation_boundary_col': '0.4,0.4,0',
            'constellation_label_col': '0.6,0.6,0.6',
            'dso_cluster_col': '0.6,0.6,0.1875',
            'dso_nebula_col': '0.1875,0.5,0.1875',
            'dso_label_col': '0.8,0.8,0.8',
            'galaxy_col': '0,0,0.5',
            'galaxy_col0': '0,0,0.25',
            "twilight_zenith_col": "0,0,0.25",
            "twilight_horizon_col": "0,0,0.25",
            'star_col': '0.8,0.8,0.8',
            'star_label_col': '0.6,0.6,0.6',
            'grid_col': '0.3,0.3,0.3',
            'equator_col': '0.65,0,0.65',
            'galactic_plane_col': '0,0.5,0.25',
            'ecliptic_col': '0.8,0.65,0',
            'ephemeris_col': '1,1,0.7',
            'ephemeris_label_col': '1,1,0.7',
            'ephemeris_arrow_col': '1,1,0.7',
            "horizon_cardinal_points_marker_col": "1,1,0.7",
            "horizon_cardinal_points_labels_col": "1,1,0.7",
            "horizon_cardinal_points_marker_elevate": "1",
            "constellations_capitalise": "1",
            "constellations_label_shadow": "0",
        },
        'flat_background': {
            "galaxy_col": "0,0,0",
            "galaxy_col0": "0,0,0",
        },
        'chart_bright_horizon': {
            "twilight_horizon_col": "0.9,0.9,0.7"
        },
        'solar_system_colouring': {
            "solar_system_col": "1,1,1",
            "solar_system_label_col": "1,1,1"
        },
        'moon_zoom_labelling': {
            "solar_system_label_col": "1,1,1"
        }
    },
    'light': {
        'global_settings': {
            'constellation_stick_col': '0,0.6,0',
            'constellation_boundary_col': '0.8,0.8,0.8',
            'constellation_label_col': '0.4,0.4,0.4',
            'galaxy_col': '0.68,0.76,1',
            'galaxy_col0': '1,1,1',
            "twilight_zenith_col": "1,1,1",
            "twilight_horizon_col": "1,1,1",
            'star_col': '0.2,0.2,0.2',
            'star_label_col': '0.4,0.4,0.4',
            'grid_col': '0.7,0.7,0.7',
            'equator_col': '0.65,0,0.65',
            'galactic_plane_col': '0,0,0.75',
            'ecliptic_col': '0.8,0.65,0',
            'ephemeris_col': '0,0,0',
            'ephemeris_label_col': '0,0,0',
            'ephemeris_arrow_col': '0,0,0',
            "horizon_cardinal_points_marker_col": "1,1,0.7",
            "horizon_cardinal_points_labels_col": "1,1,0.7",
            "horizon_cardinal_points_marker_elevate": "1",
            "constellations_capitalise": "1",
            "constellations_label_shadow": "0",
        },
        'flat_background': {
            "galaxy_col": "1,1,1",
            "galaxy_col0": "1,1,1",
        },
        'chart_bright_horizon': {
            "twilight_horizon_col": "0.506,0.765,0.929"
        },
        'solar_system_colouring': {
        },
        'moon_zoom_labelling': {
            "solar_system_label_col": "0,0,0"
        }
    },
    'pastel': {
        'global_settings': {
            "galaxy_col": "0.7,0.8,1",
            "galaxy_col0": "0.337,0.547,0.820",
            'star_col': '1,1,1',
            'grid_col': '0.5,0.5,0.5',
            'ephemeris_col': '1,1,0.7',
            "constellation_label_col": "0,0,0",
            "dso_label_col": "0,0,0",
            "twilight_zenith_col": "0.337,0.547,0.820",
            "twilight_horizon_col": "0.506,0.765,0.929",
            "star_label_col": "0,0,0",
            "meteor_radiant_colour": "0.65,0.25,0.65",
            "horizon_zenith_col": "1,1,0.4",
            "dso_display_style": "fuzzy",
            "horizon_cardinal_points_marker_col": "1,1,0.7",
            "horizon_cardinal_points_labels_col": "1,1,0.7",
            "horizon_cardinal_points_marker_elevate": "1",
            "constellation_stick_col": "0.9,0.9,0.9",
            "constellation_boundary_col": "0.9,0.9,0.9",
            "dso_cluster_col": "0.9,0.9,0.9",
            "dso_nebula_col": "0.9,0.9,0.9",
            "dso_galaxy_col": "1,0.78,1",
            "equator_col": "0.8,0.3,0.8",
            "galactic_plane_col": "0,0.8,0.25",
            "ecliptic_col": "1,1,0.7",
            "ephemeris_label_col": "1,0.9,0.75",
            "ephemeris_arrow_col": "1,0.9,0.75",
            "constellations_capitalise": "1",
            "constellations_label_shadow": "0",
            "constellation_sticks_line_width": "0.7",
        },
        'flat_background': {
            "galaxy_col": "0.337,0.547,0.820",
            "galaxy_col0": "0.337,0.547,0.820",
        },
        'chart_bright_horizon': {
            "twilight_horizon_col": "0.506,0.765,0.929"
        },
        'solar_system_colouring': {
        },
        'moon_zoom_labelling': {
            "solar_system_label_col": "0,0,0"
        }
    }
}

chart_template: Final[Dict[str, str]] = {
    # Insert further default settings here
}

meteor_showers_by_month: Final[Dict[str, Dict[int, List[Tuple[str, str]]]]] = {
    'north': {
        1: [
            ("meteor_radiant", "Quadrantids,230.0000,49.0000")
        ],
        2: [
        ],
        3: [
        ],
        4: [
            ("meteor_radiant", "Lyrids,271.0000,34.0000")
        ],
        5: [
        ],
        6: [
        ],
        7: [
            ("meteor_radiant", "α-Capricornids,307.0000,-10.0000"),
            ("meteor_radiant", "Southern δ-Aquariids,340.0000,-16.0000"),
            ("meteor_radiant", "Perseids,48.0000,57.0000")
        ],
        8: [
            ("meteor_radiant", "Perseids,48.0000,57.0000")
        ],
        9: [
            ("meteor_radiant", "Aurigids,91.0000,39.0000")
        ],
        10: [
            ("meteor_radiant", "Orionids,95.0000,16.0000"),
            ("meteor_radiant", "Southern Taurids,32.0000,9.0000")
        ],
        11: [
            ("meteor_radiant", "Northern Taurids,58.0000,22.0000"),
            ("meteor_radiant", "Leonids,152.0000,22.0000")
        ],
        12: [
            ("meteor_radiant", "Geminids,112.0000,33.0000"),
            ("meteor_radiant", "Ursids,217.0000,76.0000")
        ]
    },
    'south': {
        1: [
        ],
        2: [
            ("meteor_radiant", "α-Centaurids,210.0000,-59.0000"),
        ],
        3: [
            ("meteor_radiant", "γ-Normids,239.0000,-50.0000"),
        ],
        4: [
            ("meteor_radiant", "π-Puppids,110.0000,-45.0000")
        ],
        5: [
        ],
        6: [
        ],
        7: [
            ("meteor_radiant", "α-Capricornids,307.0000,-10.0000"),
            ("meteor_radiant", "Southern δ-Aquariids,340.0000,-16.0000"),
            ("meteor_radiant", "Piscis Austrinids,341.0000,-30.0000")
        ],
        8: [
            ("meteor_radiant", "α-Capricornids,307.0000,-10.0000"),
            ("meteor_radiant", "Southern δ-Aquariids,340.0000,-16.0000")
        ],
        9: [
        ],
        10: [
            ("meteor_radiant", "Northern & Southern Taurids,32.0000,9.0000")
        ],
        11: [
            ("meteor_radiant", "Northern Taurids,58.0000,22.0000"),
        ],
        12: [
            ("meteor_radiant", "Geminids,112.0000,33.0000"),
            ("meteor_radiant", "Phoenicids,18.0000,-53.0000"),
            ("meteor_radiant", "Puppid-Velids,123.0000,-45.0000")
        ]
    }
}

object_info: Final[Dict[str, Tuple[str, Tuple[float, float, float]]]] = {
    "Sun": ("sun", (1, 1, 0.7)),
    "Moon": ("P301", (1, 1, 0.7)),
    "Mercury": ("P1", (1, 1, 0.7)),
    "Venus": ("P2", (1, 1, 0.7)),
    "Mars": ("P4", (1, 1, 0.7)),
    "Jupiter": ("P5", (1, 1, 0.7)),
    "Saturn": ("P6", (1, 1, 0.7)),
    "Uranus": ("P7", (1, 1, 0.7)),
    "Neptune": ("P8", (1, 1, 0.7)),
    "default": ("", (1, 1, 0.7))
}

asterisms: Final[Sequence[Dict[str, Any]]] = [
    {
        'title': "Circlet",
        'ra': 23.4949972222,
        'decl': 3.9860833333,
    },
    {
        'title': "Great Square of Pegasus",
        'ra': 23.62225,
        'decl': 22.2423888889,
    },
    {
        'title': "Water Jar",
        'ra': 22.230556,
        'decl': 1.6088889,
    },
    {
        'title': "The Kids",
        'ra': 4.8666667,
        'decl': 41,
    },
    {
        'title': "Mira",
        'ra': 2.4166667,
        'decl': -2.7783333,
    },
    {
        'title': "The Plough",
        'ra': 13.016667,
        'decl': 52.538333,
    },
    {
        'title': "Keystone",
        'ra': 17.408333,
        'decl': 32.666667,
    },
    {
        'title': "False Cross",
        'ra': 9.0516667,
        'decl': -53.05,
    },
    {
        'title': "Teapot",
        'ra': 18.616667,
        'decl': -30,
    },
    {
        'title': "Cat's Eyes",
        'ra': 17.383333,
        'decl': -37.22,
    }
]


def select_solar_system_objects(static_objects: Optional[Sequence[str]] = None,
                                ephemeris_objects: Optional[Sequence[str]] = None,
                                start_epoch: float = 0, end_epoch: float = 0,
                                colour_objects: bool = True, colour_labels: bool = False
                                ) -> List[Tuple[str, Union[str, float, int]]]:
    """
    Create the settings needed to add solar system objects to the chart.

    :param static_objects:
        A list of the names of the solar system objects to represent with single-epoch positions.
    :param ephemeris_objects:
        A list of the names of the solar system objects to represent with multi-epoch tracks.
    :param start_epoch:
        The JD at which to start multi-epoch tracks.
    :param end_epoch:
        The JD at which to end multi-epoch tracks.
    :param colour_objects:
        Boolean indicating whether solar system objects should be colour-coded.
    :param colour_labels:
        Boolean indicating whether ephemeris labels should share the colour of the track we draw.
    :return:
        A list of multivalued settings to apply to the chart
    """

    # Start building output list
    settings_out: List[Tuple[str, Union[str, float, int]]] = []

    # Create settings for static representations of objects
    object_name: str
    if static_objects is not None:
        for object_name in static_objects:
            de430_id: str = object_name
            object_colour: Tuple[float, float, float] = object_info["default"][1]
            if object_name in object_info:
                de430_id, object_colour = object_info[object_name]
            object_colour_str: str = ",".join([str(i) for i in object_colour])

            settings_out.append(("solar_system_ids", de430_id))
            settings_out.append(("solar_system_labels", object_name))
            if colour_objects:
                settings_out.append(("solar_system_col", object_colour_str))
            if colour_labels:
                settings_out.append(("solar_system_label_col", object_colour_str))

    # Create settings for tracks representing objects
    if ephemeris_objects is not None:
        for object_name in ephemeris_objects:
            de430_id = object_name
            object_colour = object_info["default"][1]
            if object_name in object_info:
                de430_id, object_colour = object_info[object_name]
            object_colour_str: str = ",".join([str(i) for i in object_colour])

            settings_out.append(
                ("draw_ephemeris", "{:s},{:.6f},{:.6f}".format(de430_id, start_epoch, end_epoch))
            )
            if colour_objects:
                settings_out.append(("ephemeris_col", object_colour_str))
                settings_out.append(("ephemeris_arrow_col", object_colour_str))
            if colour_labels:
                settings_out.append(("ephemeris_label_col", object_colour_str))

    # Return the settings we compiled
    return settings_out


def add_ephemeris_epochs(settings: List[Tuple[str, Union[str, float, int]]],
                         epoch_list: Optional[Sequence[float]] = None,
                         epoch_intervals: Optional[Sequence[float]] = None,
                         label_format: str = "", timezone: float = 0) -> None:
    """
    Add some epochs at which to explicitly label the ephemeris tracks.

    :param settings:
        The list of multivalued settings into which we should insert the epochs.
    :param epoch_list:
        The list of Julian Dates at which to label all the tracks (all tracks have same labels).
    :param epoch_intervals:
        The list of Julian Date intervals (days) at which to label each track (tracks have different labels).
    :param label_format:
        The format string to use to generate the label strings
    :param timezone:
        The timezone to use when rendering dates, hours ahead to GMT
    :return:
        None
    """

    epoch: float
    if epoch_list is not None:
        for epoch_jd in epoch_list:
            settings.append(("ephemeris_epochs", epoch_jd))
            date = inv_julian_day(jd=epoch_jd + timezone / 24. + 1e-6)
            tokens = {
                'y': date[0], 'm': date[1], 'd': date[2], 'h': date[3], 'i': date[4], 's': date[5],
                'month_name_full': month_name_full[date[1] - 1],
                'month_name_abbrev': month_name[date[1] - 1],
            }
            settings.append(("ephemeris_epoch_labels", label_format.format(**tokens)))

    if epoch_intervals is not None:
        for epoch_interval in epoch_intervals:
            settings.append(("ephemeris_label_interval", epoch_interval))


def text_item(text: str, x_pos: float, y_pos: float, coords: str = "page",
              font_size: float = 1, h_align: int = 0, v_align: int = 0,
              bold: bool = False, italic: bool = False, r: float = 0, g: float = 0, b: float = 0) -> Tuple[str, str]:
    """
    Create a star-charter configuration string describing a text label.

    :param text:
        The text to be displayed.
    :param x_pos:
        The horizontal position of the label.
    :param y_pos:
        The vertical position of the label.
    :param coords:
        The coordinate system of (x_pos, y_pos).
    :param font_size:
        The font size of the label.
    :param h_align:
        The horizontal alignment of the label.
    :param v_align:
        The vertical alignment of the label.
    :param bold:
        Boolean indicating whether the label is bold.
    :param italic:
        Boolean indicating whether the label is italic.
    :param r:
        The red component of the label colour.
    :param g:
        The green component of the label colour.
    :param b:
        The blue component of the label colour.
    :return:
        Tuple describing the text label
    """

    # Check all inputs are within range
    assert coords in ("page", "ra_dec", "alt_az")
    assert h_align in (-1, 0, 1)
    assert v_align in (-1, 0, 1)
    assert 0 <= r <= 1
    assert 0 <= g <= 1
    assert 0 <= b <= 1

    # Create the label setting
    return (
        'text',
        '{},{},{},{},{},{},{},{},{},{},{},{}'.format(
            coords, x_pos, y_pos, h_align, v_align, font_size, int(bold), int(italic), r, g, b, text),
    )


def arrow_item(x_pos_0: float, y_pos_0: float, x_pos_1: float, y_pos_1: float,
               coords_0: str = "page", coords_1: str = "page",
               head_start: bool = False, head_end: bool = True,
               r: float = 0, g: float = 0, b: float = 0, lw: float = 1) -> Tuple[str, str]:
    """
    Create a star-charter configuration string describing an arrow or line.

    :param x_pos_0:
        The horizontal position of the start of the arrow.
    :param y_pos_0:
        The vertical position of the start of the arrow.
    :param coords_0:
        The coordinate system of (x_pos_0, y_pos_0).
    :param x_pos_1:
        The horizontal position of the end of the arrow.
    :param y_pos_1:
        The vertical position of the end of the arrow.
    :param coords_1:
        The coordinate system of (x_pos_1, y_pos_1).
    :param head_start:
        Boolean indicating whether the start of the line has an arrow-head.
    :param head_end:
        Boolean indicating whether the end of the line has an arrow-head.
    :param r:
        The red component of the arrow colour.
    :param g:
        The green component of the arrow colour.
    :param b:
        The blue component of the arrow colour.
    :param lw:
        The line width of the arrow.
    :return:
        Tuple describing the arrow / line
    """

    # Check all inputs are within range
    assert coords_0 in ("page", "ra_dec", "alt_az")
    assert coords_1 in ("page", "ra_dec", "alt_az")
    assert 0 <= r <= 1
    assert 0 <= g <= 1
    assert 0 <= b <= 1

    # Create the label setting
    return (
        'arrow',
        '{},{},{},{},{},{},{},{},{},{},{},{}'.format(
            coords_0, x_pos_0, y_pos_0, coords_1, x_pos_1, y_pos_1,
            int(head_start), int(head_end), r, g, b, lw),
    )
