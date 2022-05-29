// ltList.c
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

// Functions for creating, querying, and manipulating linked lists

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ltDict.h"
#include "ltList.h"
#include "ltMemory.h"

//! listInit - Initialise a new linked list
//! \return A pointer to the created <list> item

list *listInit() {
    list *out;
    out = (list *) lt_malloc(sizeof(list));
    out->first = NULL;
    out->last = NULL;
    out->length = 0;
    out->memoryContext = lt_getMemContext();
    return out;
}

//! listCopy - Create a copy of a linked list. New copies of string values are created. Dict and List values are
//! copied only if deep is true.
//! \param in - The linked list to copy
//! \param deep - Flag indicating whether to make a deep copy or a shallow copy
//! \return A copy of the linked list

list *listCopy(list *in, int deep) {
    listItem *item, *outitem;
    list *out;
    out = listInit();
    item = in->first;
    while (item != NULL) {
        outitem = (listItem *) lt_malloc_incontext(sizeof(listItem), out->memoryContext);
        outitem->prev = out->last;
        outitem->next = NULL;
        outitem->dataSize = item->dataSize;
        if (item->copyable != 0) {
            outitem->data = (void *) lt_malloc_incontext(outitem->dataSize, out->memoryContext);
            memcpy(outitem->data, item->data, outitem->dataSize);
        } else {
            if ((deep != 0) && (item->dataType == DATATYPE_LIST)) outitem->data = listCopy((list *) item->data, 1);
            else if ((deep != 0) && (item->dataType == DATATYPE_DICT)) outitem->data = dictCopy((dict *) item->data, 1);
            else outitem->data = item->data;
        }
        outitem->copyable = item->copyable;
        outitem->dataType = item->dataType;
        if (out->first == NULL) out->first = outitem;
        if (out->last != NULL) out->last->next = outitem;
        out->last = outitem;
        out->length++;
        item = item->next;
    }
    return out;
}

//! listLen - Return the number of items stored in a linked list.
//! \param in - The linked list to query
//! \return The number of items within the list

int listLen(list *in) {
    if (in == NULL) return 0;
    return in->length;
}

//! listAppendPtr - Append an item to a linked list, without copying the block of memory which contains it.
//! \param in - The linked list to add the item to
//! \param item - A void pointer to the block of memory containing this item (which must be persistent; it is not
//! copied)
//! \param size - The number of bytes the item takes up
//! \param copyable - Boolean flag indicating whether this item can be copied / moved into a new block of memory
//! \param dataType - The data type to indicate for this item

void listAppendPtr(list *in, void *item, int size, int copyable, int dataType) {
    listItem *ptrnew;
    ptrnew = (listItem *) lt_malloc_incontext(sizeof(listItem), in->memoryContext);
    ptrnew->prev = in->last;
    ptrnew->next = NULL;
    ptrnew->data = item;
    ptrnew->dataSize = size;
    ptrnew->dataType = dataType;
    ptrnew->copyable = copyable;
    if (in->first == NULL) in->first = ptrnew;
    if (in->last != NULL) in->last->next = ptrnew;
    in->last = ptrnew;
    in->length++;
}

//! listAppendPtrCpy - Append an item to a linked list, copying the block of memory which contains it.
//! \param in - The linked list to add the item to
//! \param item - A void pointer to the block of memory containing this item
//! \param size - The number of bytes the item takes up
//! \param copyable - Boolean flag indicating whether this item can be copied / moved into a new block of memory
//! \param dataType - The data type to indicate for this item

void listAppendPtrCpy(list *in, void *item, int size, int dataType) {
    listItem *ptrnew;

    ptrnew = (listItem *) lt_malloc_incontext(sizeof(listItem), in->memoryContext);
    ptrnew->prev = in->last;
    ptrnew->next = NULL;
    ptrnew->data = (void *) lt_malloc_incontext(size, in->memoryContext);
    memcpy(ptrnew->data, item, size);
    ptrnew->dataSize = size;
    ptrnew->dataType = dataType;
    ptrnew->copyable = 1;
    if (in->first == NULL) in->first = ptrnew;
    if (in->last != NULL) in->last->next = ptrnew;
    in->last = ptrnew;
    in->length++;
}

//! listAppendInt - Add an integer value to a linked list
//! \param in - The linked list to add the item to
//! \param item - The value to insert

void listAppendInt(list *in, int item) {
    listAppendPtrCpy(in, (void *) &item, sizeof(int), DATATYPE_INT);
}

//! listAppendFloat - Add a double value to a linked list
//! \param in - The linked list to add the item to
//! \param item - The value to insert

void listAppendFloat(list *in, double item) {
    listAppendPtrCpy(in, (void *) &item, sizeof(double), DATATYPE_FLOAT);
}

//! listAppendString - Add a string value to a linked list
//! \param in - The linked list to add the item to
//! \param item - The character string value to insert

void listAppendString(list *in, char *item) {
    listAppendPtrCpy(in, (void *) item, (strlen(item) + 1) * sizeof(char), DATATYPE_STRING);
}

//! listAppendList - Add a List value to a linked list
//! \param in - The linked list to add the item to
//! \param item - The value to insert

void listAppendList(list *in, list *item) {
    listAppendPtr(in, (void *) item, sizeof(list), 0, DATATYPE_LIST);
}

//! listAppendDict - Add a Dict value to a linked list
//! \param in - The linked list to add the item to
//! \param item - The value to insert

void listAppendDict(list *in, dict *item) {
    listAppendPtr(in, (void *) item, sizeof(dict), 0, DATATYPE_DICT);
}

//! listRemovePtr - Remove the first instance of a particular pointer from a linked list, if it is a value
//! anywhere in the array
//! \param in - The linked list to update
//! \return - Zero on success; -1 if the key was not defined

int listRemovePtr(list *in, void *item) {
    listItem *ptr, *ptrnext;
    if (in == NULL) return -1;
    ptr = in->first;
    while (ptr != NULL) {
        if (ptr->data == item) {
            if (ptr->next != NULL) // We are not the last item in the list
            {
                ptrnext = ptr->next;
                ptr->dataType = ptrnext->dataType;
                ptr->data = ptrnext->data;
                ptr->next = ptrnext->next;
                if (in->last == ptrnext) in->last = ptr;
                else ptr->next->prev = ptr;
            } else if (ptr->prev != NULL) // We are the last item in the list, but not the first item
            {
                ptrnext = ptr->prev;
                ptr->dataType = ptrnext->dataType;
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
            return 0;
        }
        ptr = ptr->next;
    }
    return -1;
}

//! dictRemovePtrAll - Remove all instances of a particular pointer from a linked list, if it is a value
//! anywhere in the array
//! \param in - The linked list to update
//! \return - Zero on success; -1 if the key was not defined

void listRemovePtrAll(list *in, void *item) {
    while (listRemovePtr(in, item) != -1);
}

//! listGetItem - Return a pointer to the Nth item in a linked list
//! \param in - The linked list to query
//! \return - A pointer to the Nth item

void *listGetItem(list *in, int N) {
    listItem *ptr;
    int i;
    if (in == NULL) return NULL;
    ptr = in->first;
    for (i = 0; ((i < N) && (ptr != NULL)); i++, ptr = ptr->next);
    if (ptr == NULL) return NULL;
    return ptr->data;
}

//! listPop - Return a pointer to the last item in a linked list, and remove it from the list
//! \param in - The linked list to query
//! \return - A pointer to the last item

void *listPop(list *in) {
    void *out;
    if (in->last == NULL) return NULL;
    out = in->last->data;
    if (in->first == in->last) {
        in->first = in->last = NULL;
    } else {
        in->last = in->last->prev;
        in->last->next = NULL;
    }
    in->length--;
    return out;
}

//! listLast - Return a pointer to the last item in a linked list
//! \param in - The linked list to query
//! \return - A pointer to the last item

void *listLast(list *in) {
    if (in->last == NULL) return NULL;
    return in->last->data;
}

//! listIterateInit - Begin iterating over all items in a linked list
//! \param in - The linked list to iterate over
//! \return - An iterator handle representing the iteration

listIterator *listIterateInit(list *in) {
    if (in == NULL) return NULL;
    return in->first;
}

//! listIterate - Fetch the next entry in an iteration over all items in a linked list
//! \param in - An iterator handle representing the iteration
//! \param dataTypeOut - The data type of the next item
//! \param ptrOut - A pointer to the next item, or NULL if there is no next item
//! \return - The updated iterator handle

listIterator *listIterate(listIterator *in, void **item) {
    if (in == NULL) {
        if (item != NULL) *item = NULL;
        return NULL;
    }
    if (item != NULL) *item = in->data;
    in = in->next;
    return in;
}

//! listPrint - Produce a string representation of a linked list.
//! \param in - The linked list to print
//! \param out - A character buffer into which to render the string representation
//! \param size - The size of the character buffer <out>.
//! \return A pointer to <out>.

char *listPrint(list *in, char *out, int size) {
    listIterator *iter;
    int pos, first;
    iter = listIterateInit(in);
    pos = 0;
    first = 1;
    strcpy(out + pos, "[");
    pos += strlen(out + pos);
    while (iter != NULL) {
        if (pos > (size - 30)) {
            strcpy(out + pos, ", ... ]");
            return out;
        }// Truncate string as we're getting close to the end of the buffer
        if (first != 1) strcpy(out + (pos++), ",");
        if (iter->dataType == DATATYPE_VOID) {
            strcpy(out + pos, "void");
        } else if (iter->dataType == DATATYPE_INT) {
            snprintf(out + pos, 1024, "%d", *((int *) iter->data));
        } else if (iter->dataType == DATATYPE_FLOAT) {
            snprintf(out + pos, 1024, "%e", *((double *) iter->data));
        } else if (iter->dataType == DATATYPE_STRING) {
            snprintf(out + pos, 1024, "'%s'", ((char *) iter->data));
        } else if (iter->dataType == DATATYPE_LIST) {
            listPrint(((list *) iter->data), out + pos, size - pos);
        } else if (iter->dataType == DATATYPE_DICT) {
            dictPrint(((dict *) iter->data), out + pos, size - pos);
        }
        pos += strlen(out + pos);
        first = 0;
        iter = listIterate(iter, NULL);
    }
    strcpy(out + pos, "]");
    pos += strlen(out + pos);
    return out;
}

