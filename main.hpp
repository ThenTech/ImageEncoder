#ifndef MAIN_HPP
#define MAIN_HPP

#define AUTHOR  "William Thenaers"
#define VERSION "0.0.0.5"

#if 0   // Disable for proper building with makefile

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

#endif // 1

//#define LOG_OFF       ///< Force logging off
//#define LOG_LOCAL     ///< Enable Block-level logging (a lot of overhead, use sparingly)

#endif // MAIN_HPP
