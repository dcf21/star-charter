// errorReport.h
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

// Functions for returning messages to the user

#ifndef ERRORREPORT_H
#define ERRORREPORT_H 1

extern char temp_err_string[];

void stch_error(char *msg);

void stch_fatal(char *file, int line, char *msg);

void stch_warning(char *msg);

void stch_report(char *msg);

void stch_log(char *msg);

void dcf_fread(void *ptr, size_t size, size_t nmemb, FILE *stream);

#endif
