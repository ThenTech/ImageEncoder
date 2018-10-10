#include "Block.hpp"
#include "algo.hpp"
#include "Logger.hpp"
#include "utils.hpp"

#include <algorithm>
#include <functional>
#include <cmath>

/**
 *  Zigzag pattern data struct.
 */
typedef struct {
    uint8_t group;
    uint8_t col;
    uint8_t x;
    uint8_t y;
} Position_t;

static std::vector<Position_t> BlockZigZagLUT;


template<size_t size>
dc::Block<size>::Block(uint8_t *row_offset_list[])
    : matrix{nullptr}
{
    std::copy_n(row_offset_list, size, this->matrix);

    for (size_t y = 0; y < size; y++) {
        std::copy_n(this->matrix[y], size, &this->expanded[y * size]);
    }

    // Transform values to [-128, 127] by subtracting 128?
    #if 1
        std::transform(this->expanded, this->expanded + size * size,
                       this->expanded,
                       bind2nd(std::plus<double>(), -128));
    #endif
}

template<size_t size>
dc::Block<size>::~Block() {

}

template<size_t size>
void dc::Block<size>::forwardDCT(void) {
    algo::transformDCT(this->expanded, size * size);
}

template<size_t size>
void dc::Block<size>::inverseDCT(void) {
    algo::transformDCTinverse(this->expanded, size * size);
}

template<size_t size>
void dc::Block<size>::quantMult(const double m[]) {
    // Divide every element from this->expanded with an element in m on the same index
    std::transform(this->expanded, this->expanded + size * size,
                   m,
                   this->expanded,
                   std::multiplies<double>());
}

template<size_t size>
void dc::Block<size>::quantDiv(const double m[]) {
    // Multiply every element from this->expanded with an element in m on the same index
    std::transform(this->expanded, this->expanded + size * size,
                   m,
                   this->expanded,
                   std::divides<double>());
}

template<size_t size>
void dc::Block<size>::expand() const {
    for (size_t y = 0; y < size; y++) {
//        for (size_t x = 0; x < size; x++) {
//            this->matrix[y][x] = this->expanded[y * size + x];
//        }
        std::copy_n(&this->expanded[y * size], size, this->matrix[y]);
    }
}

template<size_t size>
void dc::Block<size>::processDCTDivQ(const double m[]) {
    this->forwardDCT();

    // TODO std::transform with lambda?

    for (size_t y = 0; y < size; y++) {
        for (size_t x = 0; x < size; x++) {
            #if 0
                double t = this->expanded[y * size + x];
                util::Logger::Write(std::string_format("%8.2f => ", t));

                t /= m[y * size + x];
                util::Logger::Write(std::string_format("%8.2f => ", t));

                t = std::round(t);
                util::Logger::WriteLn(std::string_format("%8.2f", t));

                this->matrix[y][x] = uint8_t(t);
            #else
                this->matrix[y][x] = uint8_t(std::round(this->expanded[y * size + x]
                                                                   / m[y * size + x]));
            #endif
        }
    }
}

template<size_t size>
void dc::Block<size>::processIDCTMulQ(const double m[]) {
    this->inverseDCT();

    for (size_t y = 0; y < size; y++) {
        for (size_t x = 0; x < size; x++) {
            this->matrix[y][x] = uint8_t(std::round(this->expanded[y * size + x]
                                                               * m[y * size + x]));
        }
    }
}

template<size_t size>
void dc::Block<size>::printZigzag(void) const {
    size_t line_length = 1, current = 0;
    bool incr = true;

    for (const Position_t& p : BlockZigZagLUT) {
        util::Logger::Write(std::string_format("%02X ", this->matrix[p.y][p.x]), false);

        if (++current >= line_length) {
            current = 0;

            if (line_length >= size) {
                incr = false;
            }

            if (incr) line_length++;
            else      line_length--;

            util::Logger::WriteLn("", false);
        }
    }
}

template<size_t size>
void dc::Block<size>::print() const {
    for (size_t y = 0; y < size; y++) {
        for (size_t x = 0; x < size; x++) {
//            printf("%2X ", this->matrix[y][x]);
            util::Logger::Write(std::string_format("%02X ", uint8_t(this->matrix[y][x])), false);
//            util::Logger::Write(this->matrix[y][x] > 0.0 ? util::Logger::EMPTY : util::Logger::FILL, false);
        }
        util::Logger::WriteLn("", false);
    }
}

template<size_t size>
void dc::Block<size>::logExpanded() const {
    for (size_t y = 0; y < size; y++) {
        for (size_t x = 0; x < size; x++) {
            util::Logger::Write(std::string_format("%8.2f ", this->expanded[y * size + x]), false);
        }
        util::Logger::WriteLn("", false);
    }
}

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
bool bubble_positions(const Position_t& i, const Position_t& j) {
    return (i.group == j.group)
         ? (i.col < j.col)
         : (i.group < j.group);
}


/**
 * @brief   dc::Block<size>::CreateZigZagLUT
 *          Creates a std::vector with Position_t entries in BlockZigZagLUT
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
template<size_t size>
void dc::Block<size>::CreateZigZagLUT(void) {
    constexpr size_t len = size * size;
    BlockZigZagLUT.resize(len);

    for(uint8_t i = 0; i < len; i++){
        const uint8_t x = i % size;
        const uint8_t y = i / size;

        BlockZigZagLUT[i] = Position_t {
            uint8_t(x + y),
            ((int8_t(x - y) & 1) ? y : x),  // not divisible by 2 => y else x
            x,
            y
        };
    }

    // Sort by groups or columns
    std::sort(BlockZigZagLUT.begin(), BlockZigZagLUT.end(), bubble_positions);
}

template class dc::Block<dc::BlockSize>;
