/**
 *  \file tape.h
 *  \brief This file contains some auxiliary routines.
 *  \author Daniel Bershatsky
 *  \date 2018
 *  \copyright GNU General Public License v3.0
 *
 *  \defgroup tape tape
 *  \ingroup matfile
 *  \brief This module contains some auxiliary routines.
 *
 *  @{
 */

#pragma once

#include <stdlib.h>

/**
 *  Tape object is similar to std::vector<char> in C++. It is higly convenient
 *  for stream processing and parsing data structures of unknown size.
 */
typedef struct _tape_t tape_t;

/**
 *  Allocate memory only for tape_t data structure and use given buffer as base
 *  tape buffer.
 *
 *  \param[in] buffer
 *  \param[in] length
 *  \return If buffer binded successfuly return pointer to tape, otherwise null
 *  pointer.
 */
tape_t *tape_bind(void *buffer, size_t length);

/**
 *  Creates new tape object.
 *
 *  \param[in] length Maximal length of tape buffer.
 *  \return Pointer to new tape object if successful otherwise null.
 */
tape_t *tape_create(size_t length);

/**
 *  Get pointer to the beginning of inner bytes tape without destruction of
 *  tape object.
 *
 *  \param[in] tape Pointer into tape object.
 *  \return Pointer to the beginning of bytes sequence.
 */
void *tape_deref(tape_t *tape);

/**
 *  Destroy tape object and its inner state.
 *
 *  \param[in] tape Pointer into tape object.
 */
void tape_destroy(tape_t *tape);

/**
 *  Pop some bytes from tape and roll back pointer to the end of tape.
 *
 *  \param[in] tape Pointer into tape object.
 *  \param[in] size Size of block of data in bytes.
 */
void tape_pop(tape_t *tape, size_t size);

/**
 *  Adjust tape memory and return pointer to beginning of it but lose state of
 *  tape object itself in order to destroy it. This function is similar to
 *  runState function of ST monad in functional programming.
 *  safety.
 *
 *  \param[in] tape Pointer into tape object.
 *  \return Beginning of bytes block in the inner state of tape.
 */
void *tape_purge(tape_t *tape);

/**
 *  Reserve block elements(bytes) on tape and return pointer to the beginning
 *  of the block. The routine retuens pointer to part of tape before bytes were
 *  added. In other words this function reserves memory.
 *
 *  \param[in] tape Pointer into tape object.
 *  \param[in] size Size of block of data in bytes.
 *  \return Pointer to beginning of block if allocation successful, otherwise
 *  null.
 */
void *tape_push(tape_t *tape, size_t size);

/** @} */
