#ifndef ALGO_HPP
#define ALGO_HPP

#include <vector>
#include <cstdint>

// Use one implementation of:
//#define ALGO_USE_DCT_LEE
//#define ALGO_USE_DCT_NAIVE
#define ALGO_USE_DCT_NAIVE_PY

namespace algo {
    /**
     *  Zigzag pattern data struct.
     */
    typedef struct {
        uint8_t group;
        uint8_t col;
        uint8_t x;
        uint8_t y;
    } Position_t;

    /**
     *  Zig-zag function
     */
    void createZigzagLUT(std::vector<algo::Position_t>&, const size_t);


    static constexpr uint8_t MER_PATTERN_SIZE = 8u;  ///< Corners and sides on diamond pattern.

    /**
     *  Motion estimation pattern data struct.
     */
    struct MER_level_t {
        uint8_t depth;
        int16_t x0;
        int16_t y0;
        MER_level_t *points;
    };

    /**
     *  Motion estimation pattern function
     */
    void createMERLUT(MER_level_t&, const size_t);
    void printMERLUT(algo::MER_level_t&);
    void destroyMERLUT(algo::MER_level_t&);


    /**
     *  RLE data struct
     */
    typedef struct {
        uint8_t zeroes;
        uint8_t data_bits;
        int16_t data;
    } RLE_data_t;


    /**
     *  DCT functions
     */
    void transformDCT(double[], const size_t);
    void transformDCTinverse(double[], const std::size_t);
}

#endif // ALGO_HPP
