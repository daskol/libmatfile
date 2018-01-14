/**
 *  \file tape.c
 *  \author Daniel Bershatsky
 *  \date 2018
 *  \copyright GNU General Public License v3.0
 */

#include "matfile/tape.h"
#include <memory.h>
#include <stdlib.h>

typedef struct _tape_t {
    void   *elems;      ///<Pointer to byte buffer.
    size_t  cur_length; ///<Current used number of bytes.
    size_t  max_length; ///<Maximal number of bytes.
} tape_t;

tape_t *tape_create(size_t length) {
    tape_t * tape = malloc(sizeof(tape_t));

    if (!tape) {
        return NULL;
    }

    tape->elems = malloc(length);
    tape->cur_length = 0;
    tape->max_length = length;

    if (!tape->elems) {
        tape_destroy(tape);
        return NULL;
    }

    return tape;
}

void tape_destroy(tape_t *tape) {
    if (!tape) {
        return;
    }

    if (tape->elems) {
        free((void *)tape->elems);
    }

    free((void *)tape);
}

void *tape_deref(tape_t *tape) {
    return tape->elems;
}

void tape_pop(tape_t *tape, size_t size) {
    if (tape->cur_length > size) {
        tape->cur_length -= size;
    }
    else {
        tape->cur_length = 0;
    }
}

void * tape_push(tape_t *tape, size_t size) {
    if (tape->cur_length + size > tape->max_length) {
        tape->max_length *= 2;
        tape->elems = realloc((void *)tape->elems, tape->max_length);

        if (!elems) {
            return NULL;
        }
    }

    void *current = (char *)tape->elems + tape->cur_length;
    tape->cur_length += size;
    return current;
}

void * tape_purge(tape_t *tape) {
    void * elements = tape->elems;

    if (tape->cur_length < tape->max_length) {
        elements = realloc(tape->elems, tape->cur_length);
        if (!elements) {
            tape_destroy(tape);
            return NULL;
        }
    }

    tape->elems = NULL;
    tape->cur_length = 0;
    tape->max_length = 0;

    tape_destroy(tape);
    return elements;
}
