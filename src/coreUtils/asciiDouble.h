// asciiDouble.h
// 
// -------------------------------------------------
// Copyright 2015-2022 Dominic Ford
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

#ifndef ASCIIDOUBLE_H
#define ASCIIDOUBLE_H 1

#include <stdio.h>

int get_digit(const char c);

double get_float(const char *str, int *Nchars);

int valid_float(const char *str, int *end);

char *numeric_display(double in, int N, int sig_fig, int latex);

unsigned char double_equal(double a, double b);

void file_readline(FILE *file, char *output);

void get_word(char *out, const char *in, int max);

const char *next_word(const char *in);

char *friendly_time_string();

char *str_strip(const char *in, char *out);

char *str_upper(const char *in, char *out);

char *str_lower(const char *in, char *out);

char *str_underline(const char *in, char *out);

char *str_slice(const char *in, char *out, int start, int end);

char *str_comma_separated_list_scan(const char **inscan, char *out);

int str_cmp_no_case(const char *a, const char *b);

void readConfig_fetchKey(char *line, char *out);

void readConfig_fetchValue(char *line, char *out);

#endif

