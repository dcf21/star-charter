// ltStringProc.c
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

// Linked-list string processing functions

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "coreUtils/asciiDouble.h"

#include "ltList.h"

//! strSplit - Split up a string into a linked-list of words separated by whitespace
//! \param in - The string to split
//! \return - A linked list of the words within the input string

list *strSplit(char *in) {
    int pos, start, end;
    char *word;
    char *text_buffer;
    list *out;
    out = listInit();
    pos = 0;
    text_buffer = (char *) malloc((strlen(in) + 8) * sizeof(char));
    while (in[pos] != '\0') {
        // Scan along to find the next word
        while ((in[pos] <= ' ') && (in[pos] > '\0')) pos++;
        start = pos;

        // Scan along to find the end of this word
        while ((in[pos] > ' ') && (in[pos] > '\0')) pos++;
        end = pos;

        if (end > start) {
            word = str_slice(in, text_buffer, start, end);
            listAppendString(out, word);
        }
    }
    free(text_buffer);
    return out;
}

