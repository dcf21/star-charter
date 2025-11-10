#!/usr/bin/python3
# -*- coding: utf-8 -*-
# do_moon_closeup.py

"""
Render close-up chart of the Moon's progress
"""

import os

from typing import Dict, List, Tuple, Union

from chart_render import ChartRender
import chart_templates
from dcf_ast import julian_day

# Epoch of chart
epoch_start: float = julian_day(2025, 1, 21, 6, 30, 0)
epoch_drawn: float = julian_day(2025, 1, 24, 6, 30, 0)
epoch_end: float = julian_day(2025, 1, 25, 6, 30, 0)

# Settings for the chart
settings: Dict[str, Union[str, float, int]] = {
    **chart_templates.colour_schemes[chart_templates.colour_scheme]['global_settings'],
    **chart_templates.colour_schemes[chart_templates.colour_scheme]['moon_zoom_labelling'],
    **chart_templates.chart_template,
    "title": "The path of the Moon, 22-25 Jan 2025",
    "projection": "stereographic",
    "julian_date": epoch_drawn,
    "az_central": 168,
    "alt_central": 20,
    "angular_width": 50,
    "width": 10.0,
    "aspect": 1,
    "coords": "alt_az",
    "grid_coords": "alt_az",
    "constellation_boundaries": 0,
    "star_names": 1,
    "maximum_star_label_count": 20,
    "dso_names": 1,
    "shade_twilight": 1,
    "show_grid_lines": 0,
    "plot_equator": 0,
    "plot_galactic_plane": 0,
    "show_solar_system": 1,
    "show_horizon": 1,
    "solar_system_topocentric_correction": 1,
    "horizon_latitude": 52,
    "horizon_longitude": 0,
    "horizon_cardinal_points_marker_elevate": 1,
    "constellations_capitalise": 1,
    "constellations_label_shadow": 0,
    "show_zenith": 1,
    "show_poles": 1,
    "magnitude_key": 0,
    "great_circle_key": 0,
    "great_circle_line_width": 0.5,
    "great_circle_dotted": 1,
    "dso_symbol_key": 0,
    "plot_ecliptic": 0,
    "plot_dso": 0,
    "plot_galaxy_map": 0,
    "constellation_sticks": 0,
    "constellation_names": 0,
    "mag_min": 2.7,
    "mag_size_norm": 0.5,
    "horizon_cardinal_points_marker_count": 16,
    "scale_bar_col": "1,1,1",
    "ephemeris_style": "side_by_side",
    "solar_system_sun_actual_size_scaling": 2,
    "must_show_all_ephemeris_labels": 1,
    "scale_bar": "0.05,0.3,0,10"
}

# Create epochs for displaying solar system objects
settings_epochs: List[Tuple[str, Union[str, float]]] = chart_templates.select_solar_system_objects(
    ephemeris_objects=("Moon",),
    start_epoch=epoch_start,
    end_epoch=epoch_end,
    colour_labels=False
)

# Create a list of text labels to display
labels: List[Tuple[str, Union[str, float]]] = [
    chart_templates.text_item(coords="page", x_pos=0.95, y_pos=0.05, h_align=1, v_align=0, font_size=1.75, bold=True,
                              italic=False, r=1, g=1, b=1, text="The path of the Moon"),
    chart_templates.text_item(coords="page", x_pos=0.95, y_pos=0.10, h_align=1, v_align=0, font_size=1.75, bold=True,
                              italic=False, r=1, g=1, b=1, text="22-25 January"),
    chart_templates.text_item(coords="page", x_pos=0.95, y_pos=0.15, h_align=1, v_align=0, font_size=1.4, bold=True,
                              italic=False, r=1, g=1, b=1, text="Morning, 06:30"),
]

# Create output directory
os.system("mkdir -p output output_config")

# Render chart
renderer: ChartRender = ChartRender()
renderer.configure(settings=settings)
renderer.configure_multivalued_from_list(settings=settings_epochs)
renderer.configure_multivalued_from_list(settings=labels)
for suffix in ('png', 'svg', 'pdf'):
    renderer.set_output_filename(
        output_filename='output/moon_zoom_example.{suffix}'.format(suffix=suffix))
renderer.dump_config("output_config/moon_zoom_example.sch")
renderer.render_all()
