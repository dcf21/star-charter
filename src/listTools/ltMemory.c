// ltMemory.c
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

// Functions for easy memory management. These routines store a linked list of pointers which are returned by a wrapper
// to malloc calls, which can then be freed all at once. The routines support multiple "contexts", whereby the user can
// start a new sub-group of malloced blocks of memory which can be freed independently of any blocks already created.
//
// The integer variable <lt_mem_context> keeps track of the current context, and can be incremented by a call to
// <lt_descendIntoNewContext> to create a new context.

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "coreUtils/strConstants.h"

#include "ltMemory.h"

// ---------------------------------------------------------
// ltMemory functions
// These provide simple wrapper for fastmalloc which keep track of the current memory allocation context
// ---------------------------------------------------------

//! current memory allocation context
int lt_mem_context = -1;

//! Maximum value of lt_mem_context
#define PPL_MAX_CONTEXTS 250

//! Storage buffer for error messages
char temp_merr_string[LSTR_LENGTH];

//! Handler for errors
void (*mem_error)(char *);

//! Handler for logging events
void (*mem_log)(char *);


//! lt_memoryInit - Call this before using any ltMemory, ltList or ltDist functions.
//! \param mem_error_handler - Function pointer to call in case error messages need to be reported
//! \param mem_log_handler - Function pointer to call to report logging messages

void lt_memoryInit(void(*mem_error_handler)(char *), void(*mem_log_handler)(char *)) {
    mem_error = mem_error_handler;
    mem_log = mem_log_handler;
    if (MEMDEBUG1) (*mem_log)("Initialising memory management system.");
    lt_mem_context = 0;
    fastmalloc_init();
    lt_freeAll(0);
    _lt_setMemContext(0);
}

//! lt_memoryStop - Call this when ltMemory, ltList or ltDist are all finished with and should be cleaned up

void lt_memoryStop() {
    if (MEMDEBUG1) (*mem_log)("Shutting down memory management system.");
    fastmalloc_close();
    lt_mem_context = -1;
}

//! lt_descendIntoNewContext - Create a new memory allocation context for future calls to lt_malloc
//! \return - the context number of the allocation context which has been assigned to future lt_malloc calls

int lt_descendIntoNewContext() {
    if (lt_mem_context < 0) {
        (*mem_error)("Call to lt_descendIntoNewContext() before call to lt_memoryInit().");
        return -1;
    }
    if (lt_mem_context >= (PPL_MAX_CONTEXTS + 1)) {
        (*mem_error)("Too many memory contexts.");
        return -1;
    }
    _lt_setMemContext(lt_mem_context + 1);
    if (MEMDEBUG1) {
        snprintf(temp_merr_string, 1024, "Descended into memory context %d.", lt_mem_context);
        (*mem_log)(temp_merr_string);
    }
    return lt_mem_context;
}

//! lt_ascendOutOfContext - Call when the current memory allocation context is finished with and can be freed.
//! \param context - the number of the allocation context which is to be freed
//! \return - the number of the current allocation context after the freeing operation

int lt_ascendOutOfContext(int context) {
    if (lt_mem_context < 0) {
        (*mem_error)("Call to lt_ascendOutOfContext() before call to lt_memoryInit().");
        return -1;
    }
    if (context > lt_mem_context) return lt_mem_context;
    if (context <= 0) {
        (*mem_error)("Call to lt_ascendOutOfContext() attempting to ascend out of lowest possible memory context.");
        return -1;
    }
    if (MEMDEBUG1) {
        snprintf(temp_merr_string, 1024, "Ascending out of memory context %d.", lt_mem_context);
        (*mem_log)(temp_merr_string);
    }
    lt_freeAll(context);
    _lt_setMemContext(context - 1);
    return lt_mem_context;
}

//! _lt_setMemContext - PRIVATE FUNCTION to update the current memory allocation context, with any free operation
//! \param context - the number of the new allocation context

void _lt_setMemContext(int context) {
    if ((context < 0) || (context >= PPL_MAX_CONTEXTS)) {
        snprintf(temp_merr_string, 1024, "lt_SetMemContext passed unrecognised context number %d.", context);
        (*mem_error)(temp_merr_string);
        return;
    }
    lt_mem_context = context;
}

//! lt_getMemContext - Return the number of the current memory allocation context
//! \return - the number of the current memory allocation context

int lt_getMemContext() {
    return lt_mem_context;
}

//! lt_freeAll - Free all memory which has been allocated in the specified allocation context, and in deeper levels
//! \param context - the number of the allocation context which is to be freed

void lt_freeAll(int context) {
    static int latch = 0;

    if (latch == 1) return; // Prevent recursive calls
    if (lt_mem_context < 0) return; // Memory management not initialised
    latch = 1;

    if ((context < 0) || (context >= PPL_MAX_CONTEXTS)) {
        snprintf(temp_merr_string, 1024, "lt_freeAll() passed unrecognised context %d.", context);
        (*mem_error)(temp_merr_string);
        return;
    }

    if (MEMDEBUG1) {
        snprintf(temp_merr_string, 1024, "Freeing all memory down to level %d.", context);
        (*mem_log)(temp_merr_string);
    }
    fastmalloc_freeall(context);
    latch = 0;
}

//! lt_free - Free all memory which has been allocated in the specified allocation context, but not in deeper levels
//! \param context - the number of the allocation context which is to be freed

void lt_free(int context) {
    static int latch = 0;

    if (latch == 1) return; // Prevent recursive calls
    latch = 1;

    if ((context < 0) || (context >= PPL_MAX_CONTEXTS)) {
        snprintf(temp_merr_string, 1024, "lt_free() passed unrecognised context %d.", context);
        (*mem_error)(temp_merr_string);
        return;
    }

    if (MEMDEBUG1) {
        snprintf(temp_merr_string, 1024, "Freeing all memory down in level %d.", context);
        (*mem_log)(temp_merr_string);
    }
    fastmalloc_free(context);
    latch = 0;
}

//! lt_malloc - Malloc some memory in the present allocation context
//! \param size - number of bytes required
//! \return - void pointer to the newly allocated memory

void *lt_malloc(int size) {
    void *out;

    if ((lt_mem_context < 0) || (lt_mem_context >= PPL_MAX_CONTEXTS)) {
        sprintf(temp_merr_string, "lt_malloc() using unrecognised context %d.", lt_mem_context);
        (*mem_error)(temp_merr_string);
        return NULL;
    }

    if (MEMDEBUG2) {
        snprintf(temp_merr_string, 1024, "Request to malloc %d bytes at memory level %d.", size, lt_mem_context);
        (*mem_log)(temp_merr_string);
    }
    out = fastmalloc(lt_mem_context, size);
    if (out == NULL) {
        (*mem_error)("Out of memory.");
        return NULL;
    }
    return out;
}

//! lt_malloc_incontext - Malloc some memory in the specified allocation context
//! \param size - number of bytes required
//! \param context - the allocation context into which to register the block of memory
//! \return - void pointer to the newly allocated memory

void *lt_malloc_incontext(int size, int context) {
    void *out;

    if ((context < 0) || (context >= PPL_MAX_CONTEXTS)) {
        snprintf(temp_merr_string, 1024, "lt_malloc_incontext() passed unrecognised context %d.", context);
        (*mem_error)(temp_merr_string);
        return NULL;
    }

    if (MEMDEBUG2) {
        snprintf(temp_merr_string, 1024, "Request to malloc %d bytes at memory level %d.", size, context);
        (*mem_log)(temp_merr_string);
    }
    out = fastmalloc(context, size);
    if (out == NULL) {
        (*mem_error)("Out of memory.");
        return NULL;
    }
    return out;
}

// ------------------------------------------
// ALL FUNCTIONS BELOW THIS POINT ARE PRIVATE
// ------------------------------------------

// Implementation of FASTMALLOC

//! For each allocation context, a pointer to the first chunk of memory which we have malloced
void **_fastmalloc_firstblocklist;

//! For each allocation context, a pointer to the chunk of memory which we are currently allocating from
void **_fastmalloc_currentblocklist;

//! For each allocation context, integers recording how many bytes have been allocated from the current block
long *_fastmalloc_currentblock_alloc_ptr;

//! Keep statistics on numbers of malloc calls
long long _fastmalloc_callcount;
long long _fastmalloc_bytecount;
long long _fastmalloc_malloccount;

//! Boolean flag indicated whether we have been initialised
static int _fastmalloc_initialised = 0;

//! fastmalloc_init - Initialise fastmalloc

void fastmalloc_init() {
    int i;
    if (_fastmalloc_initialised == 1) return;

    _fastmalloc_firstblocklist = (void **) malloc(PPL_MAX_CONTEXTS * sizeof(void *));
    _fastmalloc_currentblocklist = (void **) malloc(PPL_MAX_CONTEXTS * sizeof(void *));
    _fastmalloc_currentblock_alloc_ptr = (long *) malloc(PPL_MAX_CONTEXTS * sizeof(long));

    for (i = 0; i < PPL_MAX_CONTEXTS; i++) _fastmalloc_firstblocklist[i] = NULL;
    for (i = 0; i < PPL_MAX_CONTEXTS; i++) _fastmalloc_currentblocklist[i] = NULL;
    for (i = 0; i < PPL_MAX_CONTEXTS; i++) _fastmalloc_currentblock_alloc_ptr[i] = 0;

    _fastmalloc_callcount = 0;
    _fastmalloc_bytecount = 0;
    _fastmalloc_malloccount = 0;
    _fastmalloc_initialised = 1;
}

//! fastmalloc_close - Free up memory assigned by fastmalloc

void fastmalloc_close() {
    if (_fastmalloc_initialised == 0) return;
    if (DEBUG) {
        snprintf(temp_merr_string, 1024,
                "FastMalloc shutting down: Reduced %lld calls to fastmalloc, for a total of %lld bytes, to %lld calls to malloc.",
                _fastmalloc_callcount, _fastmalloc_bytecount, _fastmalloc_malloccount);
        (*mem_log)(temp_merr_string);
    }
    fastmalloc_freeall(0);
    free(_fastmalloc_firstblocklist);
    free(_fastmalloc_currentblocklist);
    free(_fastmalloc_currentblock_alloc_ptr);
    _fastmalloc_initialised = 0;
}


//! fastmalloc - Allocate a block of memory
//! \param context - The allocation context to assign the memory to
//! \param size - The number of bytes required
//! \return - A void pointer to the new block of memory

void *fastmalloc(int context, int size) {
    void *ptr, *out;

    _fastmalloc_callcount++;
    _fastmalloc_bytecount += size;

    if ((context < 0) || (context >= PPL_MAX_CONTEXTS)) {
        snprintf(temp_merr_string, 1024, "FastMalloc asked to malloc memory in an unrecognised context %d.", context);
        (*mem_error)(temp_merr_string);
        return NULL;
    }

    if ((_fastmalloc_currentblocklist[context] == NULL) ||
        (size > (FM_BLOCKSIZE - 2 - _fastmalloc_currentblock_alloc_ptr[context])))
    {
        // We need to malloc a new block
        _fastmalloc_malloccount++;
        if (size > FM_BLOCKSIZE - sizeof(void **)) {
            // This is a big malloc which needs to new block to itself

            if (MEMDEBUG1) {
                snprintf(temp_merr_string, 1024, "Fastmalloc creating block of size %d bytes at memory level %d.", size,
                        context);
                (*mem_log)(temp_merr_string);
            }

            if ((ptr = malloc(size + SYNCSTEP + sizeof(void **))) == NULL) {
                (*mem_error)("Out of memory.");
                return NULL;
            }
        } else {
            // Malloc a new standard sized block
            if (MEMDEBUG1) {
                snprintf(temp_merr_string, 1024, "Fastmalloc creating block of size %d bytes at memory level %d.",
                        FM_BLOCKSIZE, context);
                (*mem_log)(temp_merr_string);
            }

            if ((ptr = malloc(FM_BLOCKSIZE)) == NULL) {
                (*mem_error)("Out of memory.");
                return NULL;
            }
        }
        *((void **) ptr) = NULL; // Link to next block in chain
        if (_fastmalloc_currentblocklist[context] == NULL)
            _fastmalloc_firstblocklist[context] = ptr; // Insert link into previous block in chain
        else *((void **) _fastmalloc_currentblocklist[context]) = ptr;
        _fastmalloc_currentblocklist[context] = ptr;

        // Fast-forward over link to next block
        _fastmalloc_currentblock_alloc_ptr[context] = (sizeof(void **) + (SYNCSTEP - 1));
        _fastmalloc_currentblock_alloc_ptr[context] -= (_fastmalloc_currentblock_alloc_ptr[context] % SYNCSTEP);
        out = ptr + _fastmalloc_currentblock_alloc_ptr[context];

        // Fast-forward over block we have just allocated
        _fastmalloc_currentblock_alloc_ptr[context] += (size + (SYNCSTEP - 1));
        _fastmalloc_currentblock_alloc_ptr[context] -= (_fastmalloc_currentblock_alloc_ptr[context] % SYNCSTEP);
    } else {
        // There is room for this malloc in the old block
        out = _fastmalloc_currentblocklist[context] + _fastmalloc_currentblock_alloc_ptr[context];

        // Fast-forward over block we have just allocated
        _fastmalloc_currentblock_alloc_ptr[context] += (size + (SYNCSTEP - 1));
        _fastmalloc_currentblock_alloc_ptr[context] -= (_fastmalloc_currentblock_alloc_ptr[context] % SYNCSTEP);
    }
    return out;
}

//! fastmalloc_freeall - Free all memory assigned within an allocation context, and all deeper levels
//! \param context - The memory allocation context to free

void fastmalloc_freeall(int context) {
    int i;
    void *ptr, *ptr2;
    for (i = context; i < PPL_MAX_CONTEXTS; i++) {
        ptr = _fastmalloc_firstblocklist[i];
        while (ptr != NULL) {
            ptr2 = *((void **) ptr);
            free(ptr);
            ptr = ptr2;
        }
        _fastmalloc_firstblocklist[i] = NULL;
        _fastmalloc_currentblocklist[i] = NULL;
        _fastmalloc_currentblock_alloc_ptr[i] = 0;
    }
}

//! fastmalloc_freeall - Free all memory assigned within an allocation context
//! \param context - The memory allocation context to free

void fastmalloc_free(int context) {
    void *ptr, *ptr2;
    ptr = _fastmalloc_firstblocklist[context];
    while (ptr != NULL) {
        ptr2 = *((void **) ptr);
        free(ptr);
        ptr = ptr2;
    }
    _fastmalloc_firstblocklist[context] = NULL;
    _fastmalloc_currentblocklist[context] = NULL;
    _fastmalloc_currentblock_alloc_ptr[context] = 0;
}

