#ifndef MAIN_HPP
#define MAIN_HPP

#define AUTHOR  "William Thenaers"
#define VERSION "0.0.0.7"

#if 0  // Disable for proper building with makefile
    /*
     *  Targets will be set through makefile,
     *  or enforced here.
     */
    // Create target code for encoding
    #ifndef ENCODER
        #define ENCODER
    #endif

    // Create target code for decoding
    #ifndef DECODER
        #define DECODER
    #endif


    /**
     *  Compress the encoded image further with Huffman encoding.
     *  Decoding will always check for Huffman and deco;press if needed.
     */
    #ifndef ENABLE_HUFFMAN
        #define ENABLE_HUFFMAN
    #endif

    /**
     *  Enable OpenMP if available to parallelise Block operations.
     *  Only enable if linked with -fopenmp.
     *  (I cannot seem to get it to work with the Qt .qbs file,
     *   but the makefile will set it.)
     */
    #ifndef ENABLE_OPENMP
        #define ENABLE_OPENMP
    #endif
#endif // Makefile disable

#ifdef ENABLE_OPENMP
    #include "omp.h"
#endif

//#define LOG_OFF       ///< Force logging off
//#define LOG_LOCAL     ///< Enable Block-level logging (a lot of overhead, use sparingly)

#endif // MAIN_HPP
