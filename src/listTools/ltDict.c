// ltDict.c
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

// Functions for creating, querying, and manipulating associative arrays (similar to Python dictionaries)

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "ltDict.h"
#include "ltList.h"
#include "ltMemory.h"

#include "coreUtils/asciiDouble.h"

//! dictInit - Initialise a new associative array.
//! \param hashSize - The size of the hash table used to look up entries in the array
//! \return A pointer to the created <dict> item

dict *dictInit(int hashSize) {
    dict *out;
    out = (dict *) lt_malloc(sizeof(dict));
    if (out == NULL) return NULL;
    out->first = NULL;
    out->last = NULL;
    out->length = 0;
    out->hashSize = hashSize;
    out->hashTable = (dictItem **) lt_malloc(hashSize * sizeof(dictItem *));
    memset(out->hashTable, 0, hashSize * sizeof(dictItem *));
    if (out->hashTable == NULL) return NULL;
    out->memoryContext = lt_getMemContext();
    return out;
}

//! dictHash - Calculate the integer hash of a string
//! \param str - The string to hash
//! \param HashSize - The size of the hash table in the associative array
//! \return The integer hash

static int dictHash(const char *str, int HashSize) {
    unsigned int hash = 5381;
    int c;
    while ((c = *str++)) hash = ((hash << 5) + hash) + c;
    return hash % HashSize;
}

//! dictCopy - Create a copy of an associative array. New copies of string values are created. Dict and List values are
//! copied only if deep is true.
//! \param in - The associative array to copy
//! \param deep - Flag indicating whether to make a deep copy or a shallow copy
//! \return A copy of the associative array

dict *dictCopy(dict *in, int deep) {
    int hash;
    dictItem *item, *outitem;
    dict *out;
    out = dictInit(in->hashSize);
    if (out == NULL) return NULL;
    item = in->first;
    while (item != NULL) {
        outitem = (dictItem *) lt_malloc_incontext(sizeof(dictItem), out->memoryContext);
        if (outitem == NULL) return NULL;
        outitem->prev = out->last;
        outitem->next = NULL;
        outitem->key = (char *) lt_malloc_incontext((strlen(item->key) + 1) * sizeof(char), out->memoryContext);
        if (outitem->key == NULL) return NULL;
        strcpy(outitem->key, item->key);
        outitem->dataSize = item->dataSize;
        if (item->copyable != 0) {
            outitem->data = (void *) lt_malloc_incontext(outitem->dataSize, out->memoryContext);
            if (outitem->data == NULL) return NULL;
            memcpy(outitem->data, item->data, outitem->dataSize);
            outitem->mallocedByUs = 1;
        } else {
            if ((deep != 0) && (item->dataType == DATATYPE_LIST)) outitem->data = listCopy((list *) item->data, 1);
            else if ((deep != 0) && (item->dataType == DATATYPE_DICT)) outitem->data = dictCopy((dict *) item->data, 1);
            else outitem->data = item->data;
            outitem->mallocedByUs = 0;
        }
        outitem->copyable = item->copyable;
        outitem->dataType = item->dataType;
        if (out->first == NULL) out->first = outitem;
        if (out->last != NULL) out->last->next = outitem;
        out->last = outitem;
        out->length++;
        hash = dictHash(item->key, in->hashSize);
        out->hashTable[hash] = outitem;
        item = item->next;
    }
    return out;
}

//! dictLen - Return the number of items stored in an associative array.
//! \param in - The associative array to query
//! \return The number of items within the array

int dictLen(dict *in) {
    return in->length;
}

//! dictAppendPtr - Append an item to an associative array, without copying the block of memory which contains it.
//! \param in - The associative array to add the item to
//! \param key - The key associated with this item in the associative array
//! \param item - A void pointer to the block of memory containing this item (which must be persistent; it is not
//! copied)
//! \param size - The number of bytes the item takes up
//! \param copyable - Boolean flag indicating whether this item can be copied / moved into a new block of memory
//! \param dataType - The data type to indicate for this item

void dictAppendPtr(dict *in, char *key, void *item, int size, int copyable, int dataType) {
    dictItem *ptr = NULL, *ptrnew = NULL, *prev = NULL;
    int cmp = -1, hash;

    ptr = in->first;
    while (ptr != NULL) {
        if (((cmp = str_cmp_no_case(ptr->key, key)) > 0) || ((cmp = strcmp(ptr->key, key)) == 0)) break;
        prev = ptr;
        ptr = ptr->next;
    }
    if (cmp == 0) // Overwrite an existing entry in dictionary
    {
        ptr->data = item;
        ptr->dataSize = size;
        ptr->dataType = dataType;
        ptr->copyable = copyable;
        ptr->mallocedByUs = 0;
    } else {
        ptrnew = (dictItem *) lt_malloc_incontext(sizeof(dictItem), in->memoryContext);
        if (ptrnew == NULL) return;
        ptrnew->prev = prev;
        ptrnew->next = ptr;
        ptrnew->key = (char *) lt_malloc_incontext((strlen(key) + 1) * sizeof(char), in->memoryContext);
        if (ptrnew->key == NULL) return;
        strcpy(ptrnew->key, key);
        ptrnew->data = item;
        ptrnew->mallocedByUs = 0;
        ptrnew->dataType = dataType;
        ptrnew->copyable = copyable;
        ptrnew->dataSize = size;
        if (prev == NULL) in->first = ptrnew; else prev->next = ptrnew;
        if (ptr == NULL) in->last = ptrnew; else ptr->prev = ptrnew;
        in->length++;

        hash = dictHash(key, in->hashSize);
        in->hashTable[hash] = ptrnew;
    }
}

//! dictAppendPtrCpy - Append an item to an associative array, copying the block of memory which contains it.
//! \param in - The associative array to add the item to
//! \param key - The key associated with this item in the associative array
//! \param item - A void pointer to the block of memory containing this item
//! \param size - The number of bytes the item takes up
//! \param copyable - Boolean flag indicating whether this item can be copied / moved into a new block of memory
//! \param dataType - The data type to indicate for this item

void dictAppendPtrCpy(dict *in, char *key, void *item, int size, int dataType) {
    dictItem *ptr = NULL, *ptrnew = NULL, *prev = NULL;
    int cmp = -1, hash;

    ptr = in->first;
    while (ptr != NULL) {
        if (((cmp = str_cmp_no_case(ptr->key, key)) > 0) || ((cmp = strcmp(ptr->key, key)) == 0)) break;
        prev = ptr;
        ptr = ptr->next;
    }
    if (cmp == 0) // Overwrite an existing entry in dictionary
    {
        if ((size != ptr->dataSize) || (ptr->mallocedByUs == 0)) {
            ptr->data = (void *) lt_malloc_incontext(size, in->memoryContext);
            ptr->dataSize = size;
            ptr->mallocedByUs = 1;
        }
        memcpy(ptr->data, item, size);
        ptr->dataType = dataType;
        ptrnew = ptr;
    } else {
        ptrnew = (dictItem *) lt_malloc_incontext(sizeof(dictItem), in->memoryContext);
        if (ptrnew == NULL) return;
        ptrnew->prev = prev;
        ptrnew->next = ptr;
        ptrnew->key = (char *) lt_malloc_incontext((strlen(key) + 1) * sizeof(char), in->memoryContext);
        if (ptrnew->key == NULL) return;
        strcpy(ptrnew->key, key);
        ptrnew->data = (void *) lt_malloc_incontext(size, in->memoryContext);
        if (ptrnew->data == NULL) return;
        memcpy(ptrnew->data, item, size);
        ptrnew->mallocedByUs = 1;
        ptrnew->copyable = 1;
        ptrnew->dataType = dataType;
        ptrnew->dataSize = size;
        if (prev == NULL) in->first = ptrnew; else prev->next = ptrnew;
        if (ptr == NULL) in->last = ptrnew; else ptr->prev = ptrnew;
        in->length++;

        hash = dictHash(key, in->hashSize);
        in->hashTable[hash] = ptrnew;
    }
}

//! dictAppendInt - Add an integer value to an associative array
//! \param in - The associative array to add the item to
//! \param key - The key associated with this item in the associative array
//! \param item - The value to insert

void dictAppendInt(dict *in, char *key, int item) {
    dictAppendPtrCpy(in, key, (void *) &item, sizeof(int), DATATYPE_INT);
}

//! dictAppendFloat - Add a double value to an associative array
//! \param in - The associative array to add the item to
//! \param key - The key associated with this item in the associative array
//! \param item - The value to insert

void dictAppendFloat(dict *in, char *key, double item) {
    dictAppendPtrCpy(in, key, (void *) &item, sizeof(double), DATATYPE_FLOAT);
}

//! dictAppendString - Add a string value to an associative array
//! \param in - The associative array to add the item to
//! \param key - The key associated with this item in the associative array
//! \param item - The character string value to insert

void dictAppendString(dict *in, char *key, char *item) {
    int length = strlen(item);
    dictAppendPtrCpy(in, key, (void *) item, (length + 1) * sizeof(char), DATATYPE_STRING);
}

//! dictAppendList - Add a List value to an associative array
//! \param in - The associative array to add the item to
//! \param key - The key associated with this item in the associative array
//! \param item - The value to insert

void dictAppendList(dict *in, char *key, list *item) {
    dictAppendPtr(in, key, (void *) item, sizeof(list), 0, DATATYPE_LIST);
}

//! dictAppendDict - Add a Dict value to an associative array
//! \param in - The associative array to add the item to
//! \param key - The key associated with this item in the associative array
//! \param item - The value to insert

void dictAppendDict(dict *in, char *key, dict *item) {
    dictAppendPtr(in, key, (void *) item, sizeof(dict), 0, DATATYPE_DICT);
}

//! dictLookup - Fetch the value associated with a key in an associative array
//! \param [in] in - The associative array to query
//! \param [in] key - The key associated with item to query
//! \param [out] dataTypeOut - The data type of the item returned
//! \param [out] ptrOut - A pointer to the value associated with this key. Set to NULL if the key is not defined.

void dictLookup(dict *in, char *key, int *dataTypeOut, void **ptrOut) {
    int hash;
    dictItem *ptr;

    if (in == NULL) {
        *ptrOut = NULL;
        return;
    }

#define DICTLOOKUP_TEST \
   if (strcmp(ptr->key, key) == 0) \
    { \
     if (dataTypeOut != NULL) *dataTypeOut = ptr->dataType; \
     *ptrOut = ptr->data; \
     return; \
    }

    // Check hash table
    hash = dictHash(key, in->hashSize);
    ptr = in->hashTable[hash];
    if (ptr == NULL) {
        *ptrOut = NULL;
        return;
    }
    DICTLOOKUP_TEST;

    // Hash table clash; need to exhaustively search dictionary
    ptr = in->first;
    while (ptr != NULL) {
        DICTLOOKUP_TEST
        else if (str_cmp_no_case(ptr->key, key) > 0) break;
        ptr = ptr->next;
    }
    *ptrOut = NULL;
}

//! dictContains - Test whether an associative array has a particular key defined.
//! \param in - The associative array to query
//! \param key - The key to query
//! \return - Boolean indicating whether this key is defined

int dictContains(dict *in, char *key) {
    int hash;
    dictItem *ptr;
    if (in == NULL) return 0;

    // Check hash table
    hash = dictHash(key, in->hashSize);
    ptr = in->hashTable[hash];
    if (ptr == NULL) return 0;
    if (strcmp(ptr->key, key) == 0) return 1;

    // Hash table clash; need to exhaustively search dictionary
    ptr = in->first;
    while (ptr != NULL) {
        if (strcmp(ptr->key, key) == 0) return 1;
        ptr = ptr->next;
    }
    return 0;
}

//! dictRemoveKey - Remove a particular key from an associative array, if it is defined.
//! \param in - The associative array to update
//! \param key - The key to remove
//! \return - Zero on success; -1 if the key was not defined

int dictRemoveKey(dict *in, char *key) {
    int hash;
    dictItem *ptr;

    if (in == NULL) return -1;

    // Check hash table
    hash = dictHash(key, in->hashSize);
    ptr = in->hashTable[hash];
    if (ptr == NULL) return -1;
    if (strcmp(ptr->key, key) == 0) {
        _dictRemoveEngine(in, ptr);
        return 0;
    }

    // Hash table clash; need to exhaustively search dictionary
    ptr = in->first;
    while (ptr != NULL) {
        if (strcmp(ptr->key, key) == 0) {
            _dictRemoveEngine(in, ptr);
            return 0;
        } else if (str_cmp_no_case(ptr->key, key) > 0) break;
        ptr = ptr->next;
    }
    return -1;
}

//! dictRemovePtr - Remove the first instance of a particular pointer from an associative array, if it is a value
//! anywhere in the array
//! \param in - The associative array to update
//! \return - Zero on success; -1 if the key was not defined

int dictRemovePtr(dict *in, void *item) {
    dictItem *ptr;
    if (in == NULL) return -1;
    ptr = in->first;
    while (ptr != NULL) {
        if (ptr->data == item) {
            _dictRemoveEngine(in, ptr);
            return 0;
        }
        ptr = ptr->next;
    }
    return -1;
}

//! _dictRemoveEngine - Internal method to remove an item from an associative array
//! \param in - The associative array to update
//! \param ptr - The pointer to remove

void _dictRemoveEngine(dict *in, dictItem *ptr) {
    int hash;
    dictItem *ptrnext;

    if (in == NULL) return;
    if (ptr == NULL) return;

    // Remove hash table entry
    hash = dictHash(ptr->key, in->hashSize);
    if (in->hashTable[hash] == ptr) in->hashTable[hash] = NULL;

    if (ptr->next != NULL) // We are not the last item in the list
    {
        ptrnext = ptr->next;
        ptr->key = ptrnext->key;
        ptr->dataType = ptrnext->dataType;
        ptr->dataSize = ptrnext->dataSize;
        ptr->copyable = ptrnext->copyable;
        ptr->mallocedByUs = ptrnext->mallocedByUs;
        ptr->data = ptrnext->data;
        ptr->next = ptrnext->next;
        if (in->last == ptrnext) in->last = ptr;
        else ptr->next->prev = ptr;
    } else if (ptr->prev != NULL) // We are the last item in the list, but not the first item
    {
        ptrnext = ptr->prev;
        ptr->key = ptrnext->key;
        ptr->dataType = ptrnext->dataType;
        ptr->dataSize = ptrnext->dataSize;
        ptr->copyable = ptrnext->copyable;
        ptr->mallocedByUs = ptrnext->mallocedByUs;
        ptr->data = ptrnext->data;
        ptr->prev = ptrnext->prev;
        if (in->first == ptrnext) in->first = ptr;
        else ptr->prev->next = ptr;
    } else // We are the only item in the list
    {
        in->first = NULL;
        in->last = NULL;
    }
    in->length--;

    // Fix up hash table in case another dictionary entry shared our hash entry
    if (in->hashTable[hash] == NULL) {
        dictItem *iter = in->first;
        while (iter != NULL) {
            if (dictHash(iter->key, in->hashSize) == hash) {
                in->hashTable[hash] = iter;
                break;
            }
            iter = iter->next;
        }
    }
}

//! dictRemovePtrAll - Remove all instances of a particular pointer from an associative array, if it is a value
//! anywhere in the array
//! \param in - The associative array to update
//! \return - Zero on success; -1 if the key was not defined

void dictRemovePtrAll(dict *in, void *item) {
    while (dictRemovePtr(in, item) != -1);
}

//! dictIterateInit - Begin iterating over all items in an associative array
//! \param in - The associative array to iterate over
//! \return - An iterator handle representing the iteration

dictIterator *dictIterateInit(dict *in) {
    if (in == NULL) return NULL;
    return in->first;
}

//! dictIterate - Fetch the next entry in an iteration over all items in an associative array
//! \param in - An iterator handle representing the iteration
//! \param dataTypeOut - The data type of the next item
//! \param ptrOut - A pointer to the next item, or NULL if there is no next item
//! \return - The updated iterator handle

dictIterator *dictIterate(dictIterator *in, int *dataTypeOut, void **ptrOut) {
    if (in == NULL) {
        if (ptrOut != NULL) *ptrOut = NULL;
        return NULL;
    }
    if (dataTypeOut != NULL) *dataTypeOut = in->dataType;
    if (ptrOut != NULL) *ptrOut = in->data;
    in = in->next;
    return in;
}

//! dictPrint - Produce a string representation of an associative array.
//! \param in - The associative array to print
//! \param out - A character buffer into which to render the string representation
//! \param size - The size of the character buffer <out>.
//! \return A pointer to <out>.

char *dictPrint(dict *in, char *out, int size) {
    dictIterator *iter;
    int pos, first;
    iter = dictIterateInit(in);
    pos = 0;
    first = 1;
    strcpy(out + pos, "{");
    pos += strlen(out + pos);
    while (iter != NULL) {
        if (pos > (size - 30)) {
            strcpy(out + pos, ", ... }");
            return out;
        } // Truncate string as we're getting close to the end of the buffer
        if (first != 1) strcpy(out + (pos++), ",");
        if (iter->dataType == DATATYPE_VOID) {
            snprintf(out + pos, 1024, "'%s':void", iter->key);
        } else if (iter->dataType == DATATYPE_INT) {
            snprintf(out + pos, 1024, "'%s':%d", iter->key, *((int *) iter->data));
        } else if (iter->dataType == DATATYPE_FLOAT) {
            snprintf(out + pos, 1024, "'%s':%e", iter->key, *((double *) iter->data));
        } else if (iter->dataType == DATATYPE_STRING) {
            snprintf(out + pos, 1024, "'%s':'%s'", iter->key, ((char *) iter->data));
        } else if (iter->dataType == DATATYPE_LIST) {
            snprintf(out + pos, 1024, "'%s':", iter->key);
            pos += strlen(out + pos);
            listPrint(((list *) iter->data), out + pos, size - pos);
        } else if (iter->dataType == DATATYPE_DICT) {
            snprintf(out + pos, 1024, "'%s':", iter->key);
            pos += strlen(out + pos);
            dictPrint(((dict *) iter->data), out + pos, size - pos);
        }
        pos += strlen(out + pos);
        first = 0;
        iter = dictIterate(iter, NULL, NULL);
    }
    strcpy(out + pos, "}");
    //pos += strlen(out + pos);
    return out;
}
