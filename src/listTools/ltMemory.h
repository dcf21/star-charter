// ltMemory.h
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

#ifndef LT_MEMORY_H
#define LT_MEMORY_H 1

void lt_memoryInit(void(*mem_error_handler)(char *), void(*mem_log_handler)(char *));

void lt_memoryStop();

int lt_descendIntoNewContext();

int lt_ascendOutOfContext(int context);

void _lt_setMemContext(int context);

int lt_getMemContext();

void lt_freeAll(int context);

void lt_free(int context);

void *lt_malloc(int size);

void *lt_malloc_incontext(int size, int context);

// Fastmalloc functions

// Allocate memory in 128kb blocks (131072 bytes)
#define FM_BLOCKSIZE  131072

// Always align mallocs to 8-byte boundaries; 64-bit processors do double arithmetic twice as fast when word-aligned
#define SYNCSTEP      8

void fastmalloc_init();

void fastmalloc_close();

void *fastmalloc(int context, int size);

void fastmalloc_freeall(int context);

void fastmalloc_free(int context);

#endif

