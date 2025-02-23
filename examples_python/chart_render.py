# -*- coding: utf-8 -*-
# chart_render.py

"""
Class to render charts using StarCharter.
"""

import io
import logging
import os
import subprocess
import sys

from typing import Dict, List, Tuple, Union


class ChartRender:
    """
    Class to render charts using StarCharter.
    """

    def __init__(self):
        # Configure sensible logging defaults
        logging.basicConfig(level=logging.INFO,
                            stream=sys.stdout,
                            format='[%(asctime)s] %(levelname)s:%(filename)s:%(message)s',
                            datefmt='%d/%m/%Y %H:%M:%S')

        # Path to the StarCharter binary
        this_path: str = os.path.split(os.path.abspath(__file__))[0]
        self.starchart_binary: str = os.path.join(this_path, "../bin/starchart.bin")

        # Dictionary of chart settings
        self.settings: Dict[str, Union[str, float, int]] = {}

        # List of chart settings which will have multiple values
        self.settings_multivalued: List[Tuple[str, Union[str, float, int]]] = []

        # List of filenames for this chart (which may be in a variety of graphical formats)
        self.output_filenames: List[str] = []

    def configure(self, settings: Dict[str, Union[str, float, int]]) -> None:
        """
        Apply single-valued settings to this star chart

        :param settings:
            Dictionary of single-valued settings to apply to this star chart.
        :return:
            None
        """

        self.settings = {
            **self.settings, **settings
        }

    def configure_multivalued(self, settings: Dict[str, Union[str, float, int]]) -> None:
        """
        Apply multivalued settings to this star chart

        :param settings:
            Dictionary of multivalued settings to apply to this star chart.
        :return:
            None
        """

        for key in sorted(settings.keys()):
            value = settings[key]
            self.settings_multivalued.append((key, value))

    def configure_multivalued_from_list(self, settings: List[Tuple[str, Union[str, float, int]]]) -> None:
        """
        Apply multivalued settings to this star chart

        :param settings:
            List of multivalued settings to apply to this star chart.
        :return:
            None
        """

        self.settings_multivalued.extend(settings)

    def set_output_filename(self, output_filename: str) -> None:
        """
        Set the filename of graphical output we are to produce.

        :param output_filename:
            The filename of a chart we should produce.
        :return:
            None
        """

        self.output_filenames.append(output_filename)

    def write_config(self, output_filename: str) -> str:
        """
        Write a configuration file for StarCharter.

        :param output_filename:
            Filename of the chart we are to produce
        :return:
            Configuration file string
        """

        with io.StringIO() as output:
            # Start list of CHART settings
            output.write("CHART\n")

            # Output single-valued settings
            for key in sorted(self.settings.keys()):
                value = self.settings[key]
                output.write("{key}={value}\n".format(key=key, value=value))

            # Output multi-valued settings
            for row in self.settings_multivalued:
                key, value = row
                output.write("{key}={value}\n".format(key=key, value=value))

            # Set output filename
            output.write("output_filename={value}\n".format(value=output_filename))

            # Return configuration text
            return output.getvalue()

    def dump_config(self, output_filename: str = "debug.sch") -> None:
        """
        Dump a copy of the StarCharter configuration file to a file on disk.

        :param output_filename:
            The filename for the output configuration dump.
        :return:
            None
        """

        with open(output_filename, "wt") as output:
            output.write(self.write_config(output_filename))

    def render_file(self, output_filename: str) -> None:
        """
        Run StarCharter to render the requested charts.

        :return:
            None
        """

        # Generate star chart
        my_env = os.environ.copy()
        my_env["OMP_NUM_THREADS"] = "1"
        process = subprocess.Popen([self.starchart_binary],
                                   env=my_env,
                                   stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        sc_output: tuple = process.communicate(
            input=bytes(self.write_config(output_filename=output_filename).encode('utf-8')))
        sc_return_code: int = process.returncode
        sc_stdout: str = sc_output[0].decode("utf-8").strip()
        sc_stderr: str = sc_output[1].decode("utf-8").strip()

        # Return output messages from StarCharter
        if sc_return_code != 0:
            logging.error("StarCharter returned exit code: <{:d}>".format(sc_return_code))
        if sc_stdout:
            logging.info("StarCharter returned stdout: <{}>".format(sc_stdout))
        if sc_stderr:
            logging.info("StarCharter returned stderr: <{}>".format(sc_stderr))

    def render_all(self) -> None:
        """
        Run StarCharter to render all requested charts.

        :return:
            None
        """

        for output_filename in self.output_filenames:
            self.render_file(output_filename=output_filename)
