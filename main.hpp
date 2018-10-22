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
     *  Enable OpenMP.
     */
    #ifndef ENABLE_OPENMP
        #define ENABLE_OPENMP
    #endif
#endif // Makefile disable

/**
 *  Enable OpenMP to parallelise Block operations.
 *  Only available if linked with -fopenmp. 
 *  Use `-DENABLE_OPENMP` in the makefile to force using it.
 *  (I cannot seem to get it to work with the Qt .qbs file,
 *   but the makefile will set it.)
 *  Check version: If lower than 4.0 (201511), don't attempt to use it (looking at you MSVC).
 */
#if defined(ENABLE_OPENMP) && defined(_OPENMP) && _OPENMP >= 201511u
    #include "omp.h"
#else
	#undef ENABLE_OPENMP
#endif

//#define LOG_OFF       ///< Force logging off
//#define LOG_LOCAL     ///< Enable Block-level logging (a lot of overhead, use sparingly)

#endif // MAIN_HPP
