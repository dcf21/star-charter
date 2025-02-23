#!/usr/bin/python3
# -*- coding: utf-8 -*-
# do_planets.py

"""
Render planet charts.
"""

import os

from typing import Dict, List, Tuple, Union

from chart_render import ChartRender
import chart_templates
from dcf_ast import julian_day

settings: Dict[str, Union[str, float, int]] = {
    **chart_templates.chart_template,
    "coords": "ra_dec",
    "width": 14.0,
    "show_horizon": 0,
    "constellation_sticks": 1,
    "star_names": 1,
    "maximum_star_label_count": 25,
    "star_catalogue_numbers": 0,
    "constellation_names": 1,
    "shade_twilight": 0,
    "shade_not_observable": 0,
    "horizon_latitude": 49,
    "horizon_longitude": 0,
    "plot_equator": 0,
    "plot_ecliptic": 0,
    "plot_dso": 0,
    "plot_galaxy_map": 1,
    "show_solar_system": 0,
    "show_zenith": 0,
    "ephemeris_autoscale": 1,
    "must_show_all_ephemeris_labels": 1,
    "ephemeris_style": "side_by_side_with_arrow"
}

# Charts for objects in the outer solar can show more stars
settings_outer_objects: Dict[str, Union[str, float, int]] = {
    **settings,
    "width": 18.0,
    "font_size": 0.9,
    "constellation_boundaries": 1,
    "star_bayer_labels": 1,
    "star_flamsteed_labels": 1,
    "maximum_star_label_count": 40,
}

# Time span of ephemeris
epoch_start: float = julian_day(2025, 1, 1, 0, 0, 0)
epoch_end: float = julian_day(2026, 1, 1, 0, 0, 0)

# Create a list of epochs to mark along the ephemeris tracks
settings_epochs: List[Tuple[str, Union[str, float]]] = []
chart_templates.add_ephemeris_epochs(
    settings=settings_epochs,
    epoch_list=[
        julian_day(year, 1, 1, 0, 0, 0) for year in (2025, 2026)
    ],
    label_format="01/{y:04d}"
)
chart_templates.add_ephemeris_epochs(
    settings=settings_epochs,
    epoch_list=[
        julian_day(2025, month, 1, 0, 0, 0) for month in range(2, 13)
    ],
    label_format="{m:02d}"
)

# Chart for Mars
settings_mars: Dict[str, Union[str, float, int]] = {
    **settings,
    "font_size": 0.9,
    "projection": "flat",
    "copyright": "The path of Mars in 2025",
    "text": "page,0.52,0.36,0,0,1.5,1,0,1,0.7,0.6,MARS",
    "draw_ephemeris": "P4,{start:.6f},{end:.6f}".format(start=epoch_start, end=epoch_end),
    "aspect": 0.4,
    "mag_size_norm": 0.75,
    "mag_min": 4.9
}

settings_mars_epochs: List[Tuple[str, Union[str, float]]] = settings_epochs.copy()
chart_templates.add_ephemeris_epochs(
    settings=settings_mars_epochs,
    epoch_list=[julian_day(2025, 1, 16, 12, 0, 0)],
    label_format="16 Jan opposition"
)

# Chart for Jupiter
settings_jupiter: Dict[str, Union[str, float, int]] = {
    **settings_outer_objects,
    "font_size": 0.95,
    "copyright": "The path of Jupiter in 2025",
    "text": "page,0.52,0.33,0,0,1.5,1,0,1,0.7,1,JUPITER",
    "draw_ephemeris": "P5,{start:.6f},{end:.6f}".format(start=epoch_start, end=epoch_end),
    "aspect": 0.4,
    "mag_size_norm": 0.5,
    "mag_min": 6.8
}

settings_jupiter_epochs: List[Tuple[str, Union[str, float]]] = settings_epochs.copy()

# Chart for Saturn
settings_saturn: Dict[str, Union[str, float, int]] = {
    **settings_outer_objects,
    "font_size": 1.1,
    "copyright": "The path of Saturn in 2025",
    "text": "page,0.52,0.6,0,0,1.5,1,0,1,1,0.6,SATURN",
    "draw_ephemeris": "P6,{start:.6f},{end:.6f}".format(start=epoch_start, end=epoch_end),
    "aspect": 0.75,
    "mag_min": 7.6,
    "mag_size_norm": 0.75
}

settings_saturn_epochs: List[Tuple[str, Union[str, float]]] = settings_epochs.copy()
chart_templates.add_ephemeris_epochs(
    settings=settings_saturn_epochs,
    epoch_list=[julian_day(2025, 9, 21, 12, 0, 0)],
    label_format="21 Sept opposition"
)

# Chart for Uranus
settings_uranus: Dict[str, Union[str, float, int]] = {
    **settings_outer_objects,
    "copyright": "The path of Uranus in 2025",
    "font_size": 1.2,
    "text": "page,0.52,0.36,0,0,1.5,1,0,0.9,0.9,1,URANUS",
    "draw_ephemeris": "P7,{start:.6f},{end:.6f}".format(start=epoch_start, end=epoch_end),
    "aspect": 0.8,
    "mag_min": 9.0,
    "mag_size_norm": 0.5
}

settings_uranus_epochs: List[Tuple[str, Union[str, float]]] = settings_epochs.copy()
chart_templates.add_ephemeris_epochs(
    settings=settings_uranus_epochs,
    epoch_list=[julian_day(2025, 11, 21, 12, 0, 0)],
    label_format="21 Nov opposition"
)

# Chart for Neptune
settings_neptune: Dict[str, Union[str, float, int]] = {
    **settings_outer_objects,
    "copyright": "The path of Neptune in 2025",
    "font_size": 1.2,
    "text": "page,0.52,0.36,0,0,1.5,1,0,0.9,0.9,1,NEPTUNE",
    "draw_ephemeris": "P8,{start:.6f},{end:.6f}".format(start=epoch_start, end=epoch_end),
    "aspect": 0.8,
    "mag_min": 9.4,
    "mag_size_norm": 0.75,
    "star_flamsteed_labels": 1
}

settings_neptune_epochs: List[Tuple[str, Union[str, float]]] = settings_epochs.copy()
chart_templates.add_ephemeris_epochs(
    settings=settings_neptune_epochs,
    epoch_list=[julian_day(2025, 9, 23, 12, 0, 0)],
    label_format="23 Sept opposition"
)

# Create output directory
os.system("mkdir -p output output_config")

# Render charts
colour_scheme: str
for colour_scheme in chart_templates.colour_schemes.keys():
    object_name: str
    chart_settings: dict
    chart_settings_epochs: list
    for object_name, chart_settings, chart_settings_epochs in (
            ("Mars", settings_mars, settings_mars_epochs),
            ("Jupiter", settings_jupiter, settings_jupiter_epochs),
            ("Saturn", settings_saturn, settings_saturn_epochs),
            ("Uranus", settings_uranus, settings_uranus_epochs),
            ("Neptune", settings_neptune, settings_neptune_epochs)
    ):
        renderer: ChartRender = ChartRender()
        renderer.configure(settings=chart_templates.colour_schemes[colour_scheme]['global_settings'])
        renderer.configure(settings=chart_templates.colour_schemes[colour_scheme]['flat_background'])
        renderer.configure(settings=chart_settings)
        renderer.configure_multivalued_from_list(settings=chart_settings_epochs)
        for suffix in ('png', 'svg', 'pdf'):
            renderer.set_output_filename(
                output_filename='output/planet_{object_name}_2025_{colours}.{suffix}'.format(
                    object_name=object_name, suffix=suffix, colours=colour_scheme,
                ))
        renderer.dump_config("output_config/planet_{object_name}_2025_{colours}.sch".format(
            object_name=object_name, colours=colour_scheme,
        ))
        renderer.render_all()
