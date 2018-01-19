/**
 *  \file main.cc
 *  \brief Utility to inspect payload of mat-files. It reveals data element
 *  structure and lists symbolyc names of arrays.
 *  \author Daniel Bershatsky
 *  \date 2018
 *  \copyright GNU General Public License v3.0
 */

extern "C" {
#include <matfile/matfile.h>
#include <zlib.h>
}

#include <iostream>
#include <memory>

using std::unique_ptr;

typedef unique_ptr<matfile_t, decltype(&matfile_destroy)> matfile_ptr;
typedef unique_ptr<matfile_varname_t,
                   decltype(&matfile_varnames_destroy)> matfile_varname_ptr;

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr
            << "usage: " << argv[0] << " [matfile]" << std::endl
            << "error: too few arguments." << std::endl;
        return 1;
    }

    std::cout << "zlib version is " << zlibVersion() << std::endl;
    std::cout << "read matfile from `" << argv[1] << "`..." << std::endl;

    matfile_ptr mat(matfile_read(argv[1]), matfile_destroy);

    if (!mat) {
        std::cerr << "matfile reading was failed" << std::endl;
        return 1;
    }

    matfile_data_element_t *elements = mat->elements;
    matfile_header_t &hdr = mat->header;

    std::cout << "matfile successfully read" << std::endl;
    std::cout
        << "HEADER:" << std::endl
        << "description:           "
            << std::string(hdr.description, sizeof(hdr.description))
            << std::endl
        << "subsystem data offset: " << hdr.subsys_data_offset << std::endl
        << "version:               "
            << ((hdr.version & 0xff00) >> 8) << '.'
            << ((hdr.version & 0x00ff) >> 0) << std::endl
        << "endianness:            "
            << (char)((hdr.endianness & 0xff00) >> 8)
            << (char)((hdr.endianness & 0x00ff) >> 0) << std::endl;

    std::cout << "DATA ELEMENTS:" << std::endl;

    for (unsigned i = 0; i != mat->noelements; ++i) {
        if (matfile_is_small(&elements[i])) {
            matfile_data_element_small_t &elem = elements[i].small;
            matfile_data_type_t type = (matfile_data_type_t)elem.type;
            const char *type_str = matfile_get_type_string(type);
            std::cout
                << i + 1 << " type:      \t" << "small" << std::endl
                << i + 1 << " data type: \t" << type_str << std::endl
                << i + 1 << " data size: \t" << elem.size << std::endl;
        }
        else {
            matfile_data_element_large_t &elem = elements[i].large;
            matfile_data_type_t type = (matfile_data_type_t)elem.type;
            const char *type_str = matfile_get_type_string(type);
            std::cout
                << i + 1 << " type:      \t" << "large" << std::endl
                << i + 1 << " data type: \t" << type_str << std::endl
                << i + 1 << " data size: \t" << elem.size << std::endl;
        }
    }

    std::cout << "SYMBOLIC NAMES:" << std::endl;
    matfile_varname_ptr varnames(matfile_who(mat.get()),
                                 &matfile_varnames_destroy);

    for (unsigned i = 0; *(varnames.get() + i) != NULL; ++i) {
        std::cout << i << " variable: " << *(varnames.get() + i) << std::endl;
    }

    return 0;
}
