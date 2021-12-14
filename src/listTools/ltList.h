// ltList.h
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

#ifndef LT_LIST_H
#define LT_LIST_H 1

#define DATATYPE_VOID   31000
#define DATATYPE_INT    31001
#define DATATYPE_FLOAT  31002
#define DATATYPE_STRING 31004
#define DATATYPE_LIST   31005
#define DATATYPE_DICT   31006

typedef struct listItemS {
    void *data;
    int dataType;
    int dataSize;
    int copyable;
    struct listItemS *next;
    struct listItemS *prev;
} listItem;


typedef struct listS {
    struct listItemS *first;
    struct listItemS *last;
    int length;
    int memoryContext;
} list;

typedef listItem listIterator;

#include "ltDict.h"

// Functions defined in lt_list.c
list *listInit();

list *listCopy(list *in, int deep);

int listLen(list *in);

void listAppendPtr(list *in, void *item, int size, int copyable, int dataType);

void listAppendPtrCpy(list *in, void *item, int size, int dataType);

void listAppendInt(list *in, int item);

void listAppendFloat(list *in, double item);

void listAppendString(list *in, char *item);

void listAppendList(list *in, list *item);

void listAppendDict(list *in, dict *item);

int listRemovePtr(list *in, void *item);

void listRemovePtrAll(list *in, void *item);

void *listGetItem(list *in, int N);

void *listPop(list *in);

void *listLast(list *in);

listIterator *listIterateInit(list *in);

listIterator *listIterate(listIterator *in, void **item);

char *listPrint(list *in, char *out, int size);

#endif

