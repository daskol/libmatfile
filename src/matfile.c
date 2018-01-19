/**
 *  \file matfile.c
 *  \brief The file contains main business logic and public API implementation.
 *  \author Daniel Bershatsky
 *  \date 2018
 *  \copyright GNU General Public License v3.0
 */

#include <matfile/matfile.h>
#include <matfile/tape.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <zlib.h>

//! Shortcut for memory freeing.
#define SAFE_RELEASE(p)     if (p) {free((void *)p); p = NULL;}

static const char *array_type_string[] = {
    "mxCELL_CLASS",
    "mxSTRUCT_CLASS",
    "mxOBJECT_CLASS",
    "mxCHAR_CLASS",
    "mxSPARSE_CLASS",
    "mxDOUBLE_CLASS",
    "mxSINGLE_CLASS",
    "mxINT8_CLASS",
    "mxUINT8_CLASS",
    "mxINT16_CLASS",
    "mxUINT16_CLASS",
    "mxINT32_CLASS",
    "mxUINT32_CLASS",
    "mxINT64_CLASS",
    "mxUINT64_CLASS",
};

static const char *data_type_strings[] = {
    "miINT8",
    "miUINT8",
    "miINT16",
    "miUINT16",
    "miINT32",
    "miUINT32",
    "miSINGLE",
    "unknown",
    "miDOUBLE",
    "unknown",
    "unknown",
    "miINT64",
    "miUINT64",
    "miMATRIX",
    "miCOMPRESSED",
    "miUTF8",
    "miUTF16",
    "miUTF32",
};

static size_t data_type_size[] = {
    sizeof(int8_t),
    sizeof(uint8_t),
    sizeof(int16_t),
    sizeof(uint16_t),
    sizeof(int32_t),
    sizeof(uint32_t),
    sizeof(float),
    0,                  //  reserved
    sizeof(double),
    0,                  //  reserved
    0,                  //  reserved
    sizeof(int64_t),
    sizeof(uint64_t),
    0,                  //  not numerical type
    0,                  //  not numerical type
    0,                  //  not numerical type
    0,                  //  not numerical type
    0,                  //  not numerical type
};

static int swap_bytes = 0;

/**
 *  Decompress compressed data element with zlib. It accepts data element of
 *  miCOMPRESSED type. After decomporession the routine modifies data element
 *  in way to store correct inflated content.
 *
 *  \note In order to prevent data leak though zlib stream it would be better
 *  to initialize and destroy stream in this function but decompression logic
 *  move to subroutine.
 *
 *  \param[in,out] element Compressed byte array. It should be of miCOMPRESSED
 *  type before invocation, and it should contains compressed with correct size
 *  field.
 *  \return Return zero if data element decompression was successful, otherwise
 *  result is not zero.
 */
int decompress_data_element(matfile_data_element_t *element);

/**
 *  Parse raw data in suggesstion it contains mat-file matrix data type.
 *
 *  \todo Cast (type) compressed data.
 *
 *  \bug There is no type restoration due to values downcasting.
 *  \bug It only supports numerical arrays by now.
 *
 *  \param[in] data   Raw array of bytes.
 *  \param[in] length Length of raw data array.
 *  \return If parsing was successful it returns data structures that
 *  represents array in mat-file, otherwise null.
 */
matfile_array_t *parse_array(const void *data, size_t length);

/**
 *  Parse arbitrary data that is expected to contain correct data element
 *  array. Routines parses data elements recursively so depth parameters
 *  determine stop codition of recursion.
 *
 *  \see matfile_parse
 *
 *  \bug Do not work with buffer properly in case of matrix type.
 *
 *  \param[in]  data
 *  \param[in]  length
 *  \param[in]  endianness The endiannes indicator.
 *  \param[out] noelements
 *  \return If parsing was successful it returns array of data elements.
 */
matfile_data_element_t *parse_data_elements(const void *data,
                                            size_t length,
                                            matfile_endianness_t endianness,
                                            size_t *noelements);

/**
 *  Parse data element from raw bytes that is typed as mxMATRIX.
 *
 *  \param[in,out] array  Data structure that describes numerical array.
 *  \param[in]     data   Pointer to buffer of raw data.
 *  \param[in]     length Length of buffer.
 *  \return Return zero if numerical array parsed successfully, otherwise not
 *  zero value.
 */
int parse_numerical_array(matfile_array_t *array,
                          const void *data,
                          size_t length);

/**
 *  Parse real or imaginary part of numerical array since there is not
 *  difference between them for parsing. Both of part types are represented
 *  with data element of the same structure.
 *
 *  \todo Cast array elements to origin data type.
 *
 *  \param[in,out] array
 *  \param[out] part
 *  \param[in]  data
 *  \param[in,out] length
 *  \return Return pointer to rest of data if parsing was successful, otherwise
 *  null.
 */
const void *parse_numerical_part(matfile_array_t *array,
                                 matfile_numerical_part_t *part,
                                 const void *data,
                                 size_t *length);

/**
 *  This function swaps 2 bytes i.e. change byte order.
 *
 *  \param[in] word Byte word.
 *  \return Reversed byte word.
 */
uint16_t swap2(uint16_t word);

/**
 *  This function swaps 4 bytes i.e. change byte order.
 *
 *  \param[in] dword Byte double word.
 *  \return Reversed byte double word.
 */
uint32_t swap4(uint32_t dword);

/**
 *  This function swaps 4 bytes i.e. change byte order.
 *
 *  \param[in] quad Byte quad word.
 *  \return Reversed byte quad word.
 */
uint64_t swap8(uint64_t quad);

int decompress_data_element(matfile_data_element_t *element) {
    //  Initialize tape for inflated data.
    size_t tag_size = sizeof(matfile_data_element_small_t);
    size_t buffer_size = element->large.size | 0x80;    //  128+ bytes
    tape_t *tape = tape_create(buffer_size);

    if (!tape) {
        fprintf(stderr, "could not create tape\n");
        return 1;
    }


    //  Initialize zlib stream.
    int code = 0;
    void *base = tape_push(tape, tag_size);
    void *data = tape_push(tape, buffer_size - tag_size);
    z_stream stream = {
        element->large.data,    /* next input byte */
        element->large.size,    /* number of bytes available at next_in */
        element->large.size,    /* total number of input bytes read so far */

        base,           /* next output byte will go here */
        buffer_size,    /* remaining free space at next_out */
        buffer_size,    /* total number of bytes output so far */

        NULL, NULL,
        NULL, NULL, NULL,

        Z_BINARY,   /* best guess about the data type: binary or text
                       for deflate, or the decoding state for inflate */
        0, 0,
    };

    if ((code = inflateInit(&stream)) < Z_OK) {
        fprintf(stderr, "inflate init failed with error code %d\n", code);
        tape_destroy(tape);
        return 1;
    }

    //  Inflate the beginning of data which contains data element tag.
    if ((code = inflate(&stream, Z_SYNC_FLUSH)) < Z_OK) {
        fprintf(stderr, "inflate failed with error code %d\n", code);
        tape_destroy(tape);
        inflateEnd(&stream);
        return 1;
    }

    //  Validate (large) data element structure.
    matfile_data_element_t *subel = base;

    if (!(subel->large.type >= MFDT_INT8 && subel->large.type < MFDT_COUNT)) {
        fprintf(stderr, "wrong data type of data subelement: %d\n",
            subel->large.type);
        tape_destroy(tape);
        inflateEnd(&stream);
        return 1;
    }

    //  Copy tag to output data element and move raw data.
    element->large.type = subel->large.type;
    element->large.size = subel->large.size;
    element->large.data = base;

    //  Move payload bytes to the beginning of the tape.
    size_t avail_size = buffer_size - tag_size - stream.avail_out;
    size_t rest_size = element->large.size - avail_size;
    memmove(base, data, avail_size);

    //  Inflate rest of the data.
    tape_pop(tape, buffer_size);
    tape_push(tape, avail_size);

    stream.next_out = tape_push(tape, rest_size);
    stream.avail_out = rest_size;
    stream.total_out = rest_size;

    if ((code = inflate(&stream, Z_SYNC_FLUSH)) < Z_OK) {
        fprintf(stderr, "inflate failed with error code %d\n", code);
        tape_destroy(tape);
        inflateEnd(&stream);
        return 1;
    }

    //  There is no input data on correct data element decompression.
    if (stream.avail_in) {
        fprintf(stderr, "wrong compressed data element: %d bytes remain\n",
            stream.avail_in);
        tape_destroy(tape);
        inflateEnd(&stream);
        return 1;
    }

    //  Destroy zlib stream and tape.
    if ((code = inflateEnd(&stream)) != Z_OK) {
        fprintf(stderr, "inflate end failed with error code %d\n", code);
        tape_destroy(tape);
        return 1;
    }

    element->large.data = tape_purge(tape);

    return 0;
}

matfile_array_t *parse_array(const void *data, size_t length) {
    size_t offset = 0, size = 0, padding;
    const char *bytes = data;
    matfile_data_element_t *elem = NULL;
    matfile_array_t array;

    //  Get array flag subelement. See table 1-2.
    size = sizeof(matfile_data_element_small_t);

    if (length < offset + size + sizeof(uint64_t)) {
        fprintf(stderr, "too short subelement for matrix: array flags\n");
        return NULL;
    }

    elem = (matfile_data_element_t *)(bytes + offset);
    offset += size;

    if (elem->large.type != MFDT_UINT32) {
        fprintf(stderr, "wrong data type of array flag tag: %s(0x%08x)\n",
            matfile_get_type_string(elem->large.type), elem->large.type);
        return NULL;
    }

    if (elem->large.size != 8) {
        fprintf(stderr, "wrong data size of array flag subelement: %u\n",
            elem->large.size);
        return NULL;
    }

    array.flags = *((uint64_t *)(bytes + offset));  //  XXX: swap?
    offset += sizeof(uint64_t);

    //  Get array dimenstion. See section 1-17.
    size = sizeof(matfile_data_element_small_t);

    if (length < offset + size + 2 * sizeof(int32_t)) {
        fprintf(stderr, "too short subelement for matrix: array dimension\n");
        return NULL;
    }

    elem = (matfile_data_element_t *)(bytes + offset);
    offset += size;

    if (elem->large.type != MFDT_INT32) {
        fprintf(stderr, "wrong data type of dimention flag tag: %s(0x%08x)\n",
            matfile_get_type_string(elem->large.type), elem->large.type);
        return NULL;
    }

    if (elem->large.size % 4 != 0) {
        fprintf(stderr, "wrong data size of dimension subelement: %u\n",
            elem->large.size);
        return NULL;
    }

    if (length < offset + elem->large.size) {
        fprintf(stderr, "too short subelement for matix: array dimension\n");
        return NULL;
    }

    array.nodims = elem->large.size / 4;
    array.dims = malloc(elem->large.size);

    if (!array.dims) {
        fprintf(stderr, "could not allocate enough memory\n");
        return NULL;
    }

    memcpy(array.dims, bytes + offset, elem->large.size);
    padding = (MF_ALIGNMENT - elem->large.size % MF_ALIGNMENT) % MF_ALIGNMENT;
    offset += elem->large.size + padding;

    //  Get array name of array variable. See table 1-2.
    size = sizeof(matfile_data_element_small_t);

    if (length < offset + size) {
        fprintf(stderr, "too short subelement for matix: array name\n");
        return NULL;
    }

    elem = (matfile_data_element_t *)(bytes + offset);
    offset += size;

    if (elem->large.type != MFDT_INT8) {
        fprintf(stderr, "wrong data type of array name tag: %s(0x%08x)\n",
            matfile_get_type_string(elem->large.type), elem->large.type);
        return NULL;
    }

    if (length < offset + elem->large.size) {
        fprintf(stderr, "too short subelement for matix: array name\n");
        return NULL;
    }

    array.length = elem->large.size;
    array.name = malloc(elem->large.size + 1);

    if (!array.name) {
        fprintf(stderr, "could not allocate enough memory\n");
        return NULL;
    }

    memcpy(array.name, bytes + offset, elem->large.size);
    array.name[elem->large.size] = '\0';
    padding = (MF_ALIGNMENT - elem->large.size % MF_ALIGNMENT) % MF_ALIGNMENT;
    offset += elem->large.size + padding;

    //  Next array parsing depends on array type.
    int retcode;
    size_t rest_size = length - offset;
    const void *rest = (const void *)(bytes + offset);
    matfile_array_type_t array_type = array.flags & 0xff;

    switch (array_type) {
    case MFMX_CELL_CLASS:
    case MFMX_STRUCT_CLASS:
    case MFMX_OBJECT_CLASS:
    case MFMX_CHAR_CLASS:
    case MFMX_SPARSE_CLASS:
        fprintf(stderr, "array type is not supported by now\n");
        break;

    case MFMX_INT8_CLASS:
    case MFMX_INT16_CLASS:
    case MFMX_INT32_CLASS:
    case MFMX_INT64_CLASS:
    case MFMX_UINT8_CLASS:
    case MFMX_UINT16_CLASS:
    case MFMX_UINT32_CLASS:
    case MFMX_UINT64_CLASS:
    case MFMX_SINGLE_CLASS:
    case MFMX_DOUBLE_CLASS:
        if ((retcode = parse_numerical_array(&array, rest, rest_size)) != 0) {
            fprintf(stderr, "error during numerical array parsing\n");
            return NULL;
        }
        break;

    default:
        fprintf(stderr, "unknown array type: %d\n", array_type);
        return NULL;
    }

    //  Allocate memory for array and exit.
    matfile_array_t *array_ptr = malloc(sizeof(matfile_array_t));

    if (!array_ptr) {
        fprintf(stderr, "could not allocate memory for array\n");
        return NULL;
    }

    memcpy(array_ptr, &array, sizeof(matfile_array_t));

    return array_ptr;
}

matfile_data_element_t *parse_data_elements(const void *data,
                                            size_t length,
                                            matfile_endianness_t endianness,
                                            size_t *noelements) {
    //  Initialize auxillary structure to accumulate data elements.
    tape_t * tape = tape_create(16 * sizeof(matfile_data_element_t));

    if (!tape) {
        return NULL;
    }

    *noelements = 0;
    const unsigned char *bytes = data;

    //  Iterate over stream and interprete bytes accourding matfile
    //  specification.
    for (size_t offset = 0; offset < length; ++(*noelements)) {
        size_t small_size = sizeof(matfile_data_element_small_t);
        size_t large_size = sizeof(matfile_data_element_large_t);
        matfile_data_element_t *elem = tape_push(tape, large_size);

        memcpy((void *)elem, (void *)(bytes + offset), small_size);
        offset += small_size;

        elem->large.data = NULL;    //  prevent uninit pointer bugs.
        elem->large.noelements = 0;

        //  If these 2 bytes are not zero, the tag uses the small data element
        //  format.
        if (matfile_is_small(elem)) {
            continue;
        }

        //  Shortcuts for type and size of data element.
        size_t data_size = elem->large.size;
        size_t data_type = elem->large.type;

        if (!(data_type >= MFDT_INT8 && data_type < MFDT_COUNT)) {
            fprintf(stderr, "parsing was failed: corrupted mat-file\n");
            tape_destroy(tape);
            return NULL;
        }

        //  Estimate the beginning of compressed data.
        void *buffer = (void *)(bytes + offset);

        //  Decompress data element. Only large data element contains
        //  compressed data.
        if (elem->large.type == MFDT_COMPRESSED) {
            //  Decompress input data element.
            elem->large.data = buffer;

            if (decompress_data_element(elem)) {
                fprintf(stderr, "decompression of data element failed\n");
                tape_destroy(tape);
                return NULL;
            }

            buffer = elem->large.data;
        }
        else {
            //  All data that is uncompressed must be aligned on 64-bit
            //  boundaries except for miCOMPRESSED.
            size_t residue = data_size % MATFILE_ALIGNMENT;
            data_size += residue ? MATFILE_ALIGNMENT - residue : 0;
        }

        if(elem->large.type == MFDT_MATRIX) {
            elem->large.array = parse_array(buffer, elem->large.size);
        }
        else {
            //  Allocate memory in data element.
            elem->large.data = malloc(data_size);

            if (!elem->large.data) {
                fprintf(stderr, "could not allocate enough memory\n");
                free(elem->large.data);
                tape_destroy(tape);
                return NULL;
            }

            //  Just copy bytes from buffer into data element content.
            memcpy(elem->large.data, buffer, data_size);
        }

        //  If the data type changes due decompression it means that buffer is
        //  temporary and should be freed.
        if (elem->large.type != data_type) {
            free((void *)buffer);
        }

        offset += data_size;
    }

    return (matfile_data_element_t *)tape_purge(tape);
}

int parse_numerical_array(matfile_array_t *array,
                          const void *data,
                          size_t len) {
    //  Parse numerical parts.
    const void *end = (const void *)((const char *)data + len);

    array->pr.data = NULL;
    array->pi.data = NULL;

    if ((data = parse_numerical_part(array, &array->pr, data, &len)) == NULL) {
        fprintf(stderr, "could not parse real numerical part\n");
        return 1;
    }

    if (data == end) {
        return 0;   //  There is only real part.
    }

    if ((data = parse_numerical_part(array, &array->pi, data, &len)) == NULL) {
        fprintf(stderr, "could not parse imaginary numerical part\n");
        return 1;
    }

    return 0;
}

const void *parse_numerical_part(matfile_array_t *array,
                                 matfile_numerical_part_t *part,
                                 const void *data,
                                 size_t *len) {
    //  Decode tag of numerical part.
    size_t length = *len;
    size_t tag_size = sizeof(matfile_data_element_small_t);
    const matfile_data_element_t *elem = data;

    if (length < tag_size) {
        fprintf(stderr, "numerical part is too small\n");
        return NULL;
    }

    if (length < tag_size + elem->large.size) {
        fprintf(stderr, "numerical part is too small\n");
        return NULL;
    }

    if (!matfile_is_numerical(elem)) {
        fprintf(stderr, "data element as not numerical type\n");
        return NULL;
    }

    //  Calculate total number of elements in array.
    int noelems = 1;

    for (int i = 0; i != array->nodims; ++i) {
        noelems *= array->dims[i];
    }

    //  Validate consisntency of numerical array size.
    size_t type_size = data_type_size[elem->large.type - 1];
    size_t size = type_size * noelems;

    if (elem->large.size != size) {
        fprintf(stderr, "mismatch of data element sizes\n");
        return NULL;
    }

    //  Allocate buffer for numerical part.
    const void *bytes = (const void *)((const char *)data + tag_size);
    part->data = malloc(size);

    if (!part->data) {
        fprintf(stderr, "could not allocate enough memory\n");
        return NULL;
    }

    memcpy(part->data, bytes, size);

    return (const void *)((const char *)bytes + size);
}

uint16_t swap2(uint16_t word) {
    if (swap_bytes) {
        uint16_t byte1 = (word & (0xff << 0)) >> 0;
        uint16_t byte2 = (word & (0xff << 8)) >> 8;
        word = (byte1 << 8) + byte2;
    }
    return word;
}

uint32_t swap4(uint32_t dword) {
    if (swap_bytes) {
        uint32_t byte1 = (dword & (0xff <<  0)) >>  0;
        uint32_t byte2 = (dword & (0xff <<  8)) >>  8;
        uint32_t byte3 = (dword & (0xff << 16)) >> 16;
        uint32_t byte4 = (dword & (0xff << 24)) >> 24;
        dword = (byte1 << 24) + (byte2 << 16) + (byte3 << 8) + (byte4 << 0);
    }
    return dword;
}

uint64_t swap8(uint64_t quad) {
    if (swap_bytes) {
        uint64_t byte1 = (quad & (0xfful <<  0)) >>  0;
        uint64_t byte2 = (quad & (0xfful <<  8)) >>  8;
        uint64_t byte3 = (quad & (0xfful << 16)) >> 16;
        uint64_t byte4 = (quad & (0xfful << 24)) >> 24;
        uint64_t byte5 = (quad & (0xfful << 32)) >> 32;
        uint64_t byte6 = (quad & (0xfful << 40)) >> 40;
        uint64_t byte7 = (quad & (0xfful << 48)) >> 48;
        uint64_t byte8 = (quad & (0xfful << 56)) >> 56;
        quad = ((byte1 << 56) + (byte2 << 48) + (byte3 << 40) + (byte4 << 32) +
                (byte5 << 24) + (byte6 << 16) + (byte7 <<  8) + (byte8 <<  0));
    }
    return quad;
}

void matfile_array_destroy(matfile_array_t *array) {
    //  Nothing to destroy.
    if (!array) {
        return;
    }

    SAFE_RELEASE(array->dims)
    SAFE_RELEASE(array->name)
    SAFE_RELEASE(array->pr.data)
    SAFE_RELEASE(array->pi.data)
    SAFE_RELEASE(array)
}

void matfile_destroy(matfile_t *mat) {
    //  Nothing to destroy.
    if (!mat) {
        return;
    }

    //  If no elements just destroy mat-file.
    if (!mat->elements) {
        free((void *)mat);
        return;
    }

    for (size_t i = 0; i != mat->noelements; ++i) {
        matfile_data_element_t *el = &mat->elements[i];

        if (matfile_is_small(el) && !el->large.data) {
            continue;
        }

        //  Apply different destruct strategies for different types.
        if (mat->elements[i].large.type == MFDT_MATRIX) {
            matfile_array_destroy(el->large.array);
        }
        else {
            free(mat->elements[i].large.data);
        }
    }

    free((void *)mat->elements);
    free((void *)mat);
}

const char *matfile_get_type_string(matfile_data_type_t type) {
    if (type < MFDT_INT8 || type > MFDT_UTF32) {
        return "unknown";
    }
    else {
        return data_type_strings[type - 1];
    }
}

//! Suplementary macro for external API.
#define IS_SMALL(size, type)     (size!= 0 && type != 0)

int matfile_is_small(const matfile_data_element_t *element) {
    return IS_SMALL(element->small.size, element->small.type);
}

int matfile_is_large(const matfile_data_element_t *element) {
    return !IS_SMALL(element->small.size, element->small.type);
}

int matfile_is_numerical(const matfile_data_element_t *element) {
    matfile_data_type_t type = element->large.type;
    return (type >= MFDT_INT8 && type <= MFDT_SINGLE)
        || (type == MFDT_DOUBLE)
        || (type == MFDT_INT64)
        || (type == MFDT_UINT64);
}

matfile_data_element_t *matfile_parse(const void *data,
                                      size_t length,
                                      matfile_endianness_t endianness,
                                      size_t *noelements) {
    //  Compressed data elements contains only not compressed data and not
    //  matrix.
    return parse_data_elements(data, length, endianness, noelements);
}

matfile_t *matfile_read(const char *filename) {
    //  Open source file.
    FILE *fin = fopen(filename, "r");

    if (!fin) {
        fprintf(stderr, "theree is not such file `%s`\n", filename);
        return NULL;
    }

    //  Alloc memory for result struct.
    matfile_t *mat = malloc(sizeof(matfile_t));

    if (!mat) {
        fclose(fin);
        return NULL;
    }

    //  And read bytes from file to header struct.
    size_t header_size = sizeof(matfile_header_t);

    if (fread(mat, 1, header_size, fin) != header_size) {
        matfile_destroy(mat);
        fclose(fin);
        return NULL;
    }

    swap_bytes = mat->header.endianness == 0x494d; //   IM
    matfile_endianness_t endianness;

    if (mat->header.endianness == 0x494d) {
        endianness = MFEND_SWITCH;
    }
    else {
        endianness = MFEND_SAME;
    }

    //  Get to know size of data part of matfile.
    long begin = ftell(fin);
    fseek(fin, 0, SEEK_END);
    long end = ftell(fin);
    fseek(fin, begin, SEEK_SET);

    //  Allocate enough large buffer for data.
    size_t data_size = end - begin;
    void *data = malloc(data_size);

    if (!data) {
        matfile_destroy(mat);
        fclose(fin);
        return NULL;
    }

    //  Read whole data from file here.
    if (fread(data, 1, data_size, fin) != data_size) {
        free(data);
        matfile_destroy(mat);
        fclose(fin);
        return NULL;
    }

    fclose(fin);

    //  Parse data elements.
    mat->header.version = swap2(mat->header.version);
    mat->elements = matfile_parse(data,
                                  data_size,
                                  endianness,
                                  &mat->noelements);

    free(data); //  Buffer is temporary.

    if (!mat->elements) {
        matfile_destroy(mat);
        return NULL;
    }

    return mat;
}

void matfile_varnames_destroy(matfile_varnames_t varnames) {
    SAFE_RELEASE(varnames);
}

matfile_varnames_t matfile_who(const matfile_t *mat) {
    //  Initialize tape for varname list.
    tape_t *tape = tape_create(4 * sizeof(char *));

    if (!tape) {
        fprintf(stderr, "could not allocate memory for tape\n");
        return NULL;
    }

    //  Iterate over data elements for matrix elements.
    for (size_t i = 0; i != mat->noelements; ++i) {
        if (mat->elements[i].large.type == MFDT_MATRIX) {
            char **varname = tape_push(tape, sizeof(char *));

            if (!varname) {
                fprintf(stderr, "could not reallocate memory for tape\n");
                tape_destroy(tape);
                return NULL;
            }

            *varname = mat->elements[i].large.array->name;
        }
    }

    //  End var name list with null.
    char **varname = tape_push(tape, sizeof(char *));

    if (!varname) {
        fprintf(stderr, "could not reallocate memory for tape\n");
        tape_destroy(tape);
        return NULL;
    }

    *varname = NULL;

    //  Purge monadic type.
    return tape_purge(tape);
}
