// ltDict.h
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

#ifndef LT_DICT_H
#define LT_DICT_H 1

#define HASHSIZE_SMALL   128
#define HASHSIZE_LARGE 16384

typedef struct dictItemS {
    char *key;
    void *data;
    int dataType;
    int dataSize;
    unsigned char mallocedByUs;
    unsigned char copyable;
    struct dictItemS *next;
    struct dictItemS *prev;
} dictItem;


typedef struct dictS {
    struct dictItemS *first;
    struct dictItemS *last;
    int length;
    int hashSize;
    struct dictItemS **hashTable;
    int memoryContext;
} dict;

typedef dictItem dictIterator;

#include "ltList.h"

// Functions defined in ltDict.c
dict *dictInit(int hashSize);

dict *dictCopy(dict *in, int deep);

int dictLen(dict *in);

void dictAppendPtr(dict *in, char *key, void *item, int size, int copyable, int dataType);

void dictAppendPtrCpy(dict *in, char *key, void *item, int size, int dataType);

void dictAppendInt(dict *in, char *key, int item);

void dictAppendFloat(dict *in, char *key, double item);

void dictAppendString(dict *in, char *key, char *item);

void dictAppendList(dict *in, char *key, list *item);

void dictAppendDict(dict *in, char *key, dict *item);

void dictLookup(dict *in, char *key, int *dataTypeOut, void **ptrOut);

int dictContains(dict *in, char *key);

int dictRemoveKey(dict *in, char *key);

int dictRemovePtr(dict *in, void *item);

void dictRemovePtrAll(dict *in, void *item);

dictIterator *dictIterateInit(dict *in);

dictIterator *dictIterate(dictIterator *in, int *dataTypeOut, void **ptrOut);

char *dictPrint(dict *in, char *out, int size);

// Private
void _dictRemoveEngine(dict *in, dictItem *ptr);

#endif

