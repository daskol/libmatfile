/**
 *  \file matfile.h
 *  \brief This file defines common data structures that represents mat-file
 *  itself and API routines.
 *  \author Daniel Bershatsky
 *  \date 2018
 *  \copyright GNU General Public License v3.0
 *
 *  \defgroup matfile matfile
 *  \brief This module defines common data structures that represents mat-file
 *  itself and API routines.
 *
 *  @{
 */

#pragma once

#include <stdlib.h>
#include <stdint.h>

#define MATFILE_VERSION     "0.1.0"

#define MF_ALIGNMENT        8u  ///<Alignment of data in mat-file.
#define MATFILE_ALIGNMENT   MF_ALIGNMENT

/**
 *  Identify differences between endianess on encoder and on decoder sides.
 */
typedef enum _matfile_endianness_t {
    MFEND_SAME = 0, ///<Do not swith byte order.
    MFEND_SWITCH,   ///<Switch byte order.
    MFEND_COUNT,    ///<Number of options.
} matfile_endianness_t;

/**
 *  Identify the MATLAB array type (class) represented by the data element.
 */
typedef enum _matfile_array_type_t {
    MFMX_CELL_CLASS = 1,    ///<Cell array
    MFMX_STRUCT_CLASS,      ///<Structure
    MFMX_OBJECT_CLASS,      ///<Object
    MFMX_CHAR_CLASS,        ///<Character array
    MFMX_SPARSE_CLASS,      ///<Sparse array
    MFMX_DOUBLE_CLASS,      ///<Double precision array
    MFMX_SINGLE_CLASS,      ///<Single precision array
    MFMX_INT8_CLASS,        ///<8-bit, signed integer
    MFMX_UINT8_CLASS,       ///<8-bit, unsigned integer
    MFMX_INT16_CLASS,       ///<16-bit, signed integer
    MFMX_UINT16_CLASS,      ///<16-bit, unsigned integer
    MFMX_INT32_CLASS,       ///<32-bit, signed integer
    MFMX_UINT32_CLASS,      ///<32-bit, unsigned integer
    MFMX_INT64_CLASS,       ///<64-bit, signed integer
    MFMX_UINT64_CLASS,      ///<64-bit, unsigned integer
    MFMX_COUNT,             ///<Number of array types
} matfile_array_type_t;

/**
 *  The Data Type field specifies how the data in the element should be
 *  interpreted, that is, its size and format.
 */
typedef enum _matfile_data_type_t {
    MFDT_INT8 = 1,      ///<8 bit, signed
    MFDT_UINT8,         ///<8 bit, unsigned
    MFDT_INT16,         ///<16-bit, signed
    MFDT_UINT16,        ///<16-bit, unsigned
    MFDT_INT32,         ///<32-bit, signed
    MFDT_UINT32,        ///<32-bit, unsigned
    MFDT_SINGLE,        ///<IEEE® 754 single format
    MFDT_DOUBLE = 9,    ///<IEEE 754 double format
    MFDT_INT64 = 12,    ///<64-bit, signed
    MFDT_UINT64,        ///<64-bit, unsigned
    MFDT_MATRIX,        ///<MATLAB array
    MFDT_COMPRESSED,    ///<Compressed Data
    MFDT_UTF8,          ///<Unicode UTF-8 Encoded Character Data
    MFDT_UTF16,         ///<Unicode UTF-16 Encoded Character Data
    MFDT_UTF32,         ///<Unicode UTF-32 Encoded Character Data
    MFDT_COUNT,         ///<Number of data element types
} matfile_data_type_t;

//  Forward type definitions.

typedef const char * matfile_varname_t;
typedef matfile_varname_t * matfile_varnames_t;

typedef struct _matfile_data_element_small_t matfile_data_element_tag_t;
typedef struct _matfile_data_element_small_t matfile_data_element_small_t;
typedef struct _matfile_data_element_large_t matfile_data_element_large_t;

typedef union _matfile_data_element_t matfile_data_element_t;

#pragma pack(push, 1)

/**
 * \note Programs that create MAT-files always write data in their native
 * machine format. Programs that read MAT-files are responsible for
 * byte-swapping.
 */
typedef struct _matfile_header_t {
    /**
     *  Text data in human-readable form. This text typically provides
     *  information that describes how the MAT-file was created. For example,
     *      MATLAB 5.0 MAT-file, Platform: SOL2, Created on: Thu Nov 13
     *      10:10:27 1997
     */
    char     description[116];

    /**
     *  An offset to subsystem-specific data in the MAT-file. All zeros or all
     *  spaces in this field indicate that there is no subsystem-specific data
     *  stored in the file.
     */
    uint64_t subsys_data_offset;

    /**
     *  When creating a MAT-file, set this field to 0x0100(Level 5).
     */
    uint16_t version;

    /**
     *  Contains the two characters, M and I, written to the MAT-file in this
     *  order, as a 16-bit value. If, when read from the MAT-file as a 16-bit
     *  value, the characters appear in reversed order (IM rather than MI), it
     *  indicates that the program reading the MAT-file must perform
     *  byteswapping to interpret the data in the MAT-file correctly.
     */
    uint16_t endianness;
} matfile_header_t;

#pragma pack(pop)

/**
 *  Union that is used to represent real and imaginary part of numerical array.
 */
typedef union {
    void       *data;

    int8_t     *mx_int8;
    int16_t    *mx_int16;
    int32_t    *mx_int32;
    int64_t    *mx_int64;

    uint8_t    *mx_uint8;
    uint16_t   *mx_uint16;
    uint32_t   *mx_uint32;
    uint64_t   *mx_uint64;

    float      *mx_single;
    double     *mx_double;
} matfile_numerical_part_t;

/**
 *  Data structure represents abstract array that is stored into mat-file.
 */
typedef struct _matfile_array_t {
    /**
     *  Array flags field.
     */
    uint64_t    flags;

    /**
     *  Array that contains shape of mat-file array.
     */
    int32_t    *dims;

    /**
     *  Symbolic name of variable.
     */
    char       *name;

    /**
     *  Number of array dimension and length of dims array.
     */
    size_t      nodims;

    /**
     *  Length of variable name.
     */
    size_t      length;

    /**
     *  Real part of any numeric data type.
     */
    matfile_numerical_part_t pr;

    /**
     *  Imagimary part of any numeric data type.
     */
    matfile_numerical_part_t pi;
} matfile_array_t;

/**
 *  If a data element takes up only 1 to 4 bytes, MATLAB saves storage space by
 *  storing the data in an 8-byte format. In this format, the Data Type and
 *  Number of Bytes fields are stored as 16-bit values, freeing 4 bytes in the
 *  tag in which to store the data.
 */
typedef struct _matfile_data_element_small_t {
    uint16_t size;
    uint16_t type;
    uint32_t data;
} matfile_data_element_small_t;

/**
 *  Each data element begins with an 8-byte tag followed immediately by the
 *  data in the element.
 */
typedef struct _matfile_data_element_large_t {
    /**
     *  The Data Type field specifies how the data in the element should be
     *  interpreted, that is, its size and format.
     */
    uint32_t type;

    /**
     *  The Number of Bytes field is a 32-bit value that specifies the number
     *  of bytes of data in the element.
     */
    uint32_t size;

    /**
     *  Pointer of different type to data arrays.
     */
    union {
        void       *data;

        int8_t     *mi_int8;
        int16_t    *mi_int16;
        int32_t    *mi_int32;
        int64_t    *mi_int64;

        uint8_t    *mi_uint8;
        uint16_t   *mi_uint16;
        uint32_t   *mi_uint32;
        uint64_t   *mi_uint64;

        float      *mi_single;
        double     *mi_double;

        /**
         *  Contains data element in case of matrix or compresed data for
         *  example.
         */
        matfile_data_element_t *element;

        /**
         *  Contains array data element.
         */
        matfile_array_t *array;
    };

    /**
     *  Number of elements if this data element contains structured data.
     */
    size_t noelements;
} matfile_data_element_large_t;

/**
 *  Data element is generalization for small and large data element which could
 *  be placed into mat-file.
 */
typedef union _matfile_data_element_t {
    matfile_data_element_small_t small;
    matfile_data_element_large_t large;
} matfile_data_element_t;

/**
 *  Data structure that represents whole mat-file.
 */
typedef struct _matfile_t {
    matfile_header_t        header;
    matfile_data_element_t *elements;
    size_t                  noelements;
} matfile_t;

/**
 *  \brief Destroy mat-file data structure and free all accuired resources.
 *
 *  \param[in] mat Structure which represents mat-file.
 */
void matfile_destroy(matfile_t *mat);

/**
 *  Get abstract array by its name.
 *
 *  \param[in] mat  Pointer to mat-file object.
 *  \param[in] name Name of array.
 *  \return Pointer to array if there is array with the same name otherwise
 *  null.
 */
matfile_array_t *matfile_get_array(const matfile_t *mat, const char *name);

/**
 *  \brief Get textual description of data type code.
 *
 *  \param type Data type code.
 *  \return C-string that describes data type code.
 */
const char *matfile_get_type_string(matfile_data_type_t type);

/**
 *  This function parses raw bytes into array of data element i.e. there is not
 *  header block that contains description and version info.
 *
 *  \todo Support big endian encoders.
 *
 *  \param[in]  data   Not parsed data elements in memory.
 *  \param[in]  length Size of not parsed data buffer.
 *  \param[in]  endianness The endiannes indicator. It is MFEND_SAME if
 *  mat-file was encoded on platform which has the same endianness as the
 *  current one, otherwise MFEND_SWITCH.
 *  \param[out] noelements Number of parsed data elements.
 *  \return Pointer to data elements array if parsing were successful,
 *  otherwise null.
 */
matfile_data_element_t *matfile_parse(const void *data,
                                      size_t length,
                                      matfile_endianness_t endianness,
                                      size_t *noelements);

/**
 *  \brief Deserialize mat-file into specific data structure.
 *
 *  \param filename Name of mat-file to read.
 *  \return If it reads and parses file successfully then it returns pointer to
 *  data, otherwise it returns null.
 */
matfile_t *matfile_read(const char *filename);

/**
 *  Checks the current data element is large.
 *
 *  \param[in] element Data element of mat-file.
 *  \return If it is large data element returns 1, otherwise 0.
 */
int matfile_is_large(const matfile_data_element_t *element);

/**
 *  Checks if the data element type is numerical type.
 *
 *  \param[in] element Data element of mat-file.
 *  \return If it is numerical type then returns 1, otherwise 0.
 */
int matfile_is_numerical(const matfile_data_element_t *element);

/**
 *  Checks the current data element is small.
 *
 *  \param[in] element Data element of mat-file.
 *  \return If it is small data element returns 1, otherwise 0.
 */
int matfile_is_small(const matfile_data_element_t *element);

/**
 *  \brief Serialize data strucuture that represents content of mat-file into
 *  file.
 *
 *  \param[in] filename Name of target mat-file.
 *  \param[in] mat Data structure that represent a content of mat-file.
 *  \return Returns 0 if it writes mat-file successully.
 */
int matfile_write(const char *filename, const matfile_t *mat);

/**
 *  Destroy list of variable names.
 *
 *  \see matfile_who
 *
 *  \param[in] varnames List of variabel names.
 */
void matfile_varnames_destroy(matfile_varnames_t varnames);

/**
 *  Get list avaliable variable(array) names into mat-file. The list is
 *  allocated into the function and it should be destroyed with corresponding
 *  routine.
 *
 *  \see matfile_varnames_destroy
 *
 *  \bug There is no routine that destroy list of variable names.
 *
 *  \param[in] mat  Pointer to mat-file object.
 *  \return Dynamic array of C-strings of variable names. It should be
 *  destroyed by caller.
 */
matfile_varnames_t matfile_who(const matfile_t *mat);

/** @} */
