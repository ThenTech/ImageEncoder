#include "algo.hpp"

#ifndef _USE_MATH_DEFINES
    #define _USE_MATH_DEFINES
#endif

#include "utils.hpp"
#include "Logger.hpp"

#ifdef _MSC_VER
    // cmath does not seem to exist with MSVC compiler...
    #include <math.h>
#else
    #include <cmath>
#endif

#include <cassert>

////////////////////////////////////////////////////
///   Create zigzag lut
////////////////////////////////////////////////////

/**
 *  @brief  Sort Positions on group or column to create
 *          zig-zag pattern.
 *  @param  i
 *      First Position to compare.
 *  @param  j
 *      Second Position to compare.
 *  @return
 *      Returns true if <i> should be placed before <y> else false.
 */
bool bubble_positions(const algo::Position_t& i, const algo::Position_t& j) {
    return (i.group == j.group)
         ? (i.col < j.col)
         : (i.group < j.group);
}

/**
 * @brief   Creates a std::vector with Position_t entries in BlockZigZagLUT
 *          to be used as a LUT for the zig-zag sequence as given below.
 *
 *          Works for any (size*size) block.
 *
 *          Must be called by Encoder/Decoder before creating blocks.
 *
 *  Example matrix:
 *   0  1  2  3
 *   4  5  6  7
 *   8  9 10 11
 *  12 13 14 15
 *
 *  Zigzag sequence:
 *  0 1 4 8 5 2 3 6 9 12 13 10 7 11 14 15
 *
 *  Printed pattern:
 *   0
 *   1  4
 *   8  5  2
 *   3  6  9 12
 *  13 10  7
 *  11 14
 *  15
 *
 *  Algorithm adapted from:
 *      https://gist.github.com/gokercebeci/10556381
 */
void algo::createZigzagLUT(std::vector<algo::Position_t> &vec, const size_t size) {
    const size_t len = size * size;
    vec.clear();
    vec.resize(len);

    for(uint8_t i = 0; i < len; i++){
        const uint8_t x = i % size;
        const uint8_t y = i / size;

        vec[i] = algo::Position_t {
            uint8_t(x + y),
            ((int8_t(x - y) & 0x1) ? y : x),  // not divisible by 2 => y else x
            x,
            y
        };
    }

    // Sort by groups or columns
    std::sort(vec.begin(), vec.end(), bubble_positions);
}


static constexpr std::pair<int, int> MER_SIGNS[algo::MER_PATTERN_SIZE] = {
    std::pair<int, int>( 0,  0),   // MIDDLE-CENTER  => Already starting position, but also expandable
    std::pair<int, int>(+1, +0),   // MIDDLE-RIGHT
    std::pair<int, int>(+1, +1),   // BOTTOM-RIGHT
    std::pair<int, int>(+0, +1),   // BOTTOM-CENTER
    std::pair<int, int>(-1, +1),   // BOTTOM-LEFT
    std::pair<int, int>(-1, +0),   // MIDDLE-LEFT
    std::pair<int, int>(-1, -1),   //    TOP-LEFT
    std::pair<int, int>(+0, -1),   //    TOP-CENTER
    std::pair<int, int>(+1, -1),   //    TOP-RIGHT
};

/**
 *  @brief  Create nested pattern for motion estimation look-up.
 *  @param  pattern
 *          Target node with pattern start from offset (0, 0).
 *  @param  merange
 *          Depth for pattern provided by motion estimation range value:
 *
 *          Start pattern:
 *           *    *    *
 *
 *           *    *    *     With middle = (x0, y0) starting from (0, 0)
 *                           and other points (MER_PATTERN_SIZE) with offsets by half of merange value.
 *           *    *    *     Starting at merange / 2.
 *
 *          Nested patterns are the same with merange / 2 as new starting offset,
 *          Until merange is 0.
 */
static void generate_mer_lut(algo::MER_level_t &pattern, const int32_t merange, const uint8_t depth) {
    if (merange == 0u) {
        pattern.points = nullptr;
    } else {
        pattern.points = util::allocArray<algo::MER_level_t>(algo::MER_PATTERN_SIZE);

        for (size_t p = 0; p < algo::MER_PATTERN_SIZE; p++) {
            pattern.points[p].depth = depth;
            pattern.points[p].x0 = pattern.x0 + int16_t(MER_SIGNS[p].first  * merange);
            pattern.points[p].y0 = pattern.y0 + int16_t(MER_SIGNS[p].second * merange);
            generate_mer_lut(pattern.points[p], merange / 2, depth + 1);
        }
    }
}

void algo::createMERLUT(algo::MER_level_t &pattern, const size_t merange) {
    pattern.depth = 0;
    pattern.x0    = 0;
    pattern.y0    = 0;
    generate_mer_lut(pattern, int32_t(merange / 2), pattern.depth + 1);
}

void algo::printMERLUT(algo::MER_level_t &pattern) {
    util::Logger::Write(std::string(pattern.depth, '+'), false);
    util::Logger::WriteLn(std::string_format("(%d, %d)", pattern.x0, pattern.y0), false);

    if (pattern.points == nullptr) {
        return;
    }

    for (int i = 0; i < algo::MER_PATTERN_SIZE; i++) {
        printMERLUT(pattern.points[i]);
    }
}

void algo::destroyMERLUT(MER_level_t &pattern) {
    if (pattern.points == nullptr) {
        return;
    }

    for (int i = 0; i < MER_PATTERN_SIZE; i++) {
        algo::destroyMERLUT(pattern.points[i]);
    }

    util::deallocArray(pattern.points);
}

////////////////////////////////////////////////////
///   RLE
////////////////////////////////////////////////////


////////////////////////////////////////////////////
///   DCT
////////////////////////////////////////////////////

#if   defined(ALGO_USE_DCT_LEE)
// From: https://www.nayuki.io/page/fast-discrete-cosine-transform-algorithms

// DCT type II, unscaled. Algorithm by Byeong Gi Lee, 1984.
// See:  http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.118.3056&rep=rep1&type=pdf#page=34
static void forwardTransformDCT(double vec[], double temp[], const size_t len) {
    if (len == 1)
        return;

    const size_t halfLen = len / 2;

    for (size_t i = 0; i < halfLen; i++) {
        const double x = vec[i];
        const double y = vec[len - 1 - i];

        temp[i] = x + y;
        temp[i + halfLen] = (x - y) / (std::cos((i + 0.5) * M_PI / len) * 2);
    }

    forwardTransformDCT(temp, vec, halfLen);
    forwardTransformDCT(&temp[halfLen], vec, halfLen);

    for (size_t i = 0; i < halfLen - 1; i++) {
        vec[i * 2 + 0] = temp[i];
        vec[i * 2 + 1] = temp[i + halfLen] + temp[i + halfLen + 1];
    }

    vec[len - 2] = temp[halfLen - 1];
    vec[len - 1] = temp[len - 1];
}

// DCT type III, unscaled. Algorithm by Byeong Gi Lee, 1984.
// See: https://www.nayuki.io/res/fast-discrete-cosine-transform-algorithms/lee-new-algo-discrete-cosine-transform.pdf
static void inverseTransformDCT(double vec[], double temp[], const size_t len) {
    if (len == 1)
        return;

    const size_t halfLen = len / 2;
    temp[0] = vec[0];
    temp[halfLen] = vec[1];

    for (size_t i = 1; i < halfLen; i++) {
        temp[i] = vec[i * 2];
        temp[i + halfLen] = vec[i * 2 - 1] + vec[i * 2 + 1];
    }

    inverseTransformDCT(temp, vec, halfLen);
    inverseTransformDCT(&temp[halfLen], vec, halfLen);

    for (size_t i = 0; i < halfLen; i++) {
        const double x = temp[i];
        const double y = temp[i + halfLen] / (std::cos((i + 0.5) * M_PI / len) * 2);
        vec[i] = x + y;
        vec[len - 1 - i] = x - y;
    }
}

void algo::transformDCT(double vec[], const size_t len) {
    assert(len > 0 && (len & (len - 1)) == 0); // Length is power of 2
    std::vector<double> temp(len);
    forwardTransformDCT(vec, temp.data(), len);
}

void algo::transformDCTinverse(double vec[], const size_t len) {
    assert(len > 0 && (len & (len - 1)) == 0); // Length is power of 2
    vec[0] /= 2.0;
    std::vector<double> temp(len);
    inverseTransformDCT(vec, temp.data(), len);
}
#elif defined(ALGO_USE_DCT_NAIVE)
/**********************************************
 *  Naive approach, standard from wiki.
 **********************************************/
// From: https://www.nayuki.io/page/fast-discrete-cosine-transform-algorithms

void algo::transformDCT(double vec[], const size_t len) {
    std::vector<double> temp(len);
    const double factor = M_PI / len;

    for (size_t i = 0; i < len; i++) {
        double sum = 0;

        for (size_t j = 0; j < len; j++) {
            sum += vec[j] * std::cos((j + 0.5) * i * factor);
        }

        temp[i] = sum;
    }

    std::copy(temp.begin(), temp.end(), vec);
}

void algo::transformDCTinverse(double vec[], const size_t len) {
    std::vector<double> temp(len);
    const double factor = M_PI / len;

    for (size_t i = 0; i < len; i++) {
        double sum = vec[0] / 2.0;

        for (size_t j = 1; j < len; j++) {
            sum += vec[j] * std::cos(j * (i + 0.5) * factor);
        }

        temp[i] = sum;
    }

    std::copy(temp.begin(), temp.end(), vec);
}
#else
/**********************************************
 *  Naive approach from iPython notebook.
 **********************************************/

/**
 *  @brief  Calculate co-facrtor for each element in DCT matrix.
 *  @param  i
 *      The row or column to give the factor for.
 *  @return
 */
static inline double C(const size_t i) {
//    return i == 0 ? M_SQRT2 / 2.0 : 1.0;  // Wrong?
    return i == 0 ? 0.5 : M_SQRT1_2; // voor size=4 of len=16
}

/**
 *  @brief  Calculate the Discrete Cosine Transformation for the given flattened matrix of size len.
 *          Creates a temporary matrix and copies the results to vec after completion.
 *
 *  @param  vec
 *      The matrix to calculate the DCT for, given as a flattened array of total length len,
 *      with std::sqrt(len) colums and rows.
 *  @param  len
 *      The total length of the given array and the product of the amount of rows and columns.
 */
void algo::transformDCT(double vec[], const size_t len) {
    double *temp        = util::allocArray<double>(len);
    const size_t size   = size_t(std::sqrt(len));
    const double factor = M_PI_2 / double(size);

    for (size_t u = 0; u < size; u++) {
        for (size_t v = 0; v < size; v++) {
            for (size_t i = 0; i < size; i++) {
                for (size_t j = 0; j < size; j++) {
                    temp[u * size + v] += std::cos(double(2.0 * i + 1.0) * u * factor)
                                        * std::cos(double(2.0 * j + 1.0) * v * factor)
                                        * vec[i * size + j];
                }
            }

//            temp[u * size + v] *= C(u) * C(v) / 2.0;  // Wrong? Why extra div by 4?
            temp[u * size + v] *= C(u) * C(v);
        }
    }

    std::copy_n(temp, len, vec);
    util::deallocArray(temp);
}

/**
 *  @brief  Calculate the inverse Discrete Cosine Transformation for the given flattened matrix of size len.
 *          Creates a temporary matrix and copies the results to vec after completion.
 *
 *  @param  vec
 *      The matrix to calculate the iDCT for, given as a flattened array of total length len,
 *      with std::sqrt(len) colums and rows.
 *  @param  len
 *      The total length of the given array and the product of the amount of rows and columns.
 */
void algo::transformDCTinverse(double vec[], const size_t len) {
    double *temp        = util::allocArray<double>(len);
    const size_t size   = size_t(std::sqrt(len));
    const double factor = M_PI_2 / double(size);

    for (size_t u = 0; u < size; u++) {
        for (size_t v = 0; v < size; v++) {
            for (size_t i = 0; i < size; i++) {
                for (size_t j = 0; j < size; j++) {
                    temp[i * size + j] += C(u) * C(v) // / 4.0 // Wrong? Why extra div by 4?
                                        * std::cos(double(2.0 * i + 1.0) * u * factor)
                                        * std::cos(double(2.0 * j + 1.0) * v * factor)
                                        * vec[u * size + v];
                }
            }
        }
    }

    std::copy_n(temp, len, vec);
    util::deallocArray(temp);
}
#endif
