// read_config.h
// 
// -------------------------------------------------
// Copyright 2015-2024 Dominic Ford
//
// This file is part of StarCharter.
//
// StarCharter is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// StarCharter is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with StarCharter.  If not, see <http://www.gnu.org/licenses/>.
// -------------------------------------------------

#ifndef READ_CONFIG_H
#define READ_CONFIG_H 1

#include "settings/chart_config.h"

colour colour_from_string(const char *input);

int process_configuration_file_line(char *line, const char *filename, int iteration_depth, int file_line_number,
                                    int *got_chart, chart_config *chart_defaults,
                                    chart_config *this_chart_config, chart_config **settings_destination);

int read_configuration_file(const char *filename, int iteration_depth,
                            int *got_chart, chart_config *chart_defaults,
                            chart_config *this_chart_config, chart_config **settings_destination);

#endif
