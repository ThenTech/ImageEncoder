#include "Block.hpp"
#include "main.hpp"
#include "Logger.hpp"
#include "utils.hpp"

#include <algorithm>
#include <functional>
#include <numeric>
#include <limits>
#include <cmath>

#include "Frame.hpp"

/**
 *  Transform values to [-128, 127] by subtracting 128?
 *  This makes the DCT values smaller to fit in 0..255
 */
#define SUBTRACT_128

/**
 *  Template specification.
 *  Specify the template class to use dc::BlockSize as default <size>.
 */
template class dc::Block<dc::BlockSize>;
template class dc::Block<dc::MacroBlockSize>;

/**
 *  @brief  Lookup table (vector) for zig-zag indices.
 */
static std::vector<algo::Position_t> BlockZigZagLUT;
static algo::MER_level_t BlockMERLUT;


/**
 *  @brief  Default ctor
 *
 *  @param  row_offset_list
 *      This should be an array of length <size> with pointers to the
 *      start of each row for a Block inside a byte stream.
 *
 *      The values from the stream will be copied row-by-row to an internal
 *      array of doubles for calculation.
 */
template<size_t size>
dc::Block<size>::Block(uint8_t *row_offset_list[])
    : matrix{nullptr}
    , expanded{0.0}
    , rle_Data(nullptr)
{
    this->updateRows(row_offset_list);

    for (size_t y = 0; y < size; y++) {
        std::copy_n(this->matrix[y], size, &this->expanded[y * size]);
    }
}

/**
 *  @brief  Default ctor for Macroblock
 *
 *  @param  row_offset_list
 *      This should be an array of length <size> with pointers to the
 *      start of each row for a Block inside a byte stream.
 *
 *      The values from the stream will be copied row-by-row to an internal
 *      array of doubles for calculation.
 */
template<size_t size>
dc::Block<size>::Block(uint8_t *row_offset_list[], int16_t x, int16_t y)
    : matrix{nullptr}
    , expanded{0.0}
    , rle_Data(nullptr)
    , mvec_this{0, x, y, nullptr}
    , mvec{0, 0, 0, nullptr}
{
    this->updateRows(row_offset_list);
}

/**
 *  @brief  Default dtor
 */
template<size_t size>
dc::Block<size>::~Block() {
    if (this->rle_Data != nullptr) {
        util::deallocVector(this->rle_Data);
    }
}

template<size_t size>
void dc::Block<size>::updateRows(uint8_t *row_offset_list[]) {
    std::copy_n(row_offset_list, size, this->matrix);
}

/**
 *  @brief  Copy the internal double data back into the original stream as bytes.
 *          (all size*size elements, so only when decoding)
 *
 *          Clamp the results to fit inside a byte.
 */
template<size_t size>
void dc::Block<size>::expand(void) const {
    for (size_t y = 0; y < size; y++) {
        for (size_t x = 0; x < size; x++) {
            this->matrix[y][x] = uint8_t(std::clamp(this->expanded[y * size + x], 0.0, 255.0));
        }
//        std::copy_n(&this->expanded[y * size], size, this->matrix[y]);
    }
}


template<size_t size>
void dc::Block<size>::expandDifferences(void) const {
    for (size_t y = 0; y < size; y++) {
        for (size_t x = 0; x < size; x++) {
            this->matrix[y][x] = uint8_t(std::clamp(this->matrix[y][x]
                                                     + this->expanded[y * size + x],
                                                    0.0, 255.0));
        }
    }
}

template<size_t size>
void dc::Block<size>::copyBlockMatrixTo(dc::Block<size>& other) const {
    // Write this->matrix to other.matrix

    for (size_t y = 0; y < size; y++) {
        std::copy_n(this->matrix[y], size, other.getRow(y));
    }
}

/**
 *  @brief  Perform forward DCT on the Block data.
 *
 *          Subtract 128 from every value (if enabled) to make the
 *          resulting DCT components smaller,
 *          call transformDCT on the data,
 *          then divide each element with the quant_matrix.
 */
template<size_t size>
void dc::Block<size>::processDCTDivQ(const double m[]) {
    #ifdef SUBTRACT_128
        std::transform(this->expanded, this->expanded + size * size,
                       this->expanded,
                       std::bind(std::plus<double>(), std::placeholders::_1, -128));
    #endif

    algo::transformDCT(this->expanded, size * size);

    // Divide every element from this->expanded with an element in m on the same index
    std::transform(this->expanded, this->expanded + size * size,
                   m,
                   this->expanded,
                   [=](const double& e_, const double& m_){ return std::round(e_ / m_); });
}

/**
 *  @brief  Perform inverse DCT on the Block data.
 *
 *          Multiply with the quant_matrix,
 *          call transformDCTinverse on the data,
 *          then add 128 to every value (if enabled) to restore the original DCT components.
 */
template<size_t size>
void dc::Block<size>::processIDCTMulQ(const double m[]) {
    // Multiply every element from this->expanded with an element in m on the same index
    std::transform(this->expanded, this->expanded + size * size,
                   m,
                   this->expanded,
                   std::multiplies<double>());

    algo::transformDCTinverse(this->expanded, size * size);

    #ifdef SUBTRACT_128
        std::transform(this->expanded, this->expanded + size * size,
                       this->expanded,
                       std::bind(std::plus<double>(), std::placeholders::_1, 128));
    #endif
}

/**
 *  @brief  Create an RLE sequence from the calculated values according to zig-zag pattern.
 *          For every element, store it in the form: (#zeroes, #bits)(data)
 *
 *          Print with this->printRLE().
 */
template<size_t size>
void dc::Block<size>::createRLESequence(void) {
    if (this->rle_Data != nullptr) {
        util::deallocVector(this->rle_Data);
    }

    this->rle_Data = util::allocVar<std::vector<algo::RLE_data_t*>>();

    algo::RLE_data_t *info  = util::allocVar<algo::RLE_data_t>();
    algo::RLE_data_t *entry = nullptr;

    // Block info element
    info->zeroes    = 0;    // Unused
    info->data_bits = 0;    // Max bits needed for any following element
    info->data      = 0;    // Amount of elements after this

    this->rle_Data->push_back(info);

    // Iterate Block data by zig-zag positions
    for (const algo::Position_t& p : BlockZigZagLUT) {
        const int16_t data = int16_t(this->expanded[p.y * size + p.x]);

        if (entry == nullptr) {
            entry = util::allocVar<algo::RLE_data_t>();
        }

        if (data == 0) {
            entry->zeroes++;
        } else {
            entry->data_bits = util::bits_needed(data);  // Returns minimal bits needed to represent data as signed.
            entry->data      = data;

            // Gather block info
            info->data_bits = std::max(info->data_bits, entry->data_bits);  // Save max bits needed
            info->data     += 1 + entry->zeroes;                            // Add total data elements

            this->rle_Data->push_back(entry);
            entry = nullptr;
        }
    }

    if (entry != nullptr) {
        delete entry;
    }

    // Increase needed data bits if the data length does not fit in the current amount of bits
    info->data_bits = std::max(info->data_bits, util::ffs(uint32_t(info->data)));
}

/**
 *  @brief  Difference between this and other Block.
 *          Absolute substraction of every pixel, added together,
 *          so smaller value is less difference.
 *  @param  other
 *  @return
 */
template<size_t size>
size_t dc::Block<size>::relativeAbsDifferenceWith(const dc::Block<dc::MacroBlockSize>& other) {
    size_t diff = 0ull;

    for (size_t y = 0; y < size; y++) {
        const uint8_t* other_y = other.getRow(y);

        for (size_t x = 0; x < size; x++) {
            diff += size_t(std::abs(int16_t(this->matrix[y][x]) - int16_t(other_y[x])));
        }
    }

    return diff;
}

template<size_t size>
void dc::Block<size>::expandDifferenceWith(const dc::Block<dc::MacroBlockSize>& other) {
    for (size_t y = 0; y < size; y++) {
        const uint8_t* other_y = other.getRow(y);

        for (size_t x = 0; x < size; x++) {
            this->expanded[y * size + x] = double(this->matrix[y][x]) - double(other_y[x]);
        }
    }
}

template<size_t size>
void dc::Block<size>::processFindMotionOffset(dc::Frame * const ref_frame) {
    // Store best value in motion vector mvec
    // mvec_this has block pixel coords

    algo::MER_level_t *lowest_point = &BlockMERLUT;
    dc::MacroBlock    *lowest_block = ref_frame->getBlockAtCoord(lowest_point->x0, lowest_point->y0);
    size_t             lowest_diff  = std::numeric_limits<size_t>::max();


    // Find block in ref_frame at coord mvec with lowest diff
    while (lowest_point->points != nullptr) {
        // If lowest_point has points, a better point could still be found.

        // Best point found in lowest_point->points
        algo::MER_level_t *new_lowest_point = nullptr;
        size_t             new_lowest_diff  = lowest_diff;
        dc::MacroBlock    *new_lowest_block = nullptr;

        // For each point offset in pattern
        for (size_t p = 0; p < algo::MER_PATTERN_SIZE; p++) {
            algo::MER_level_t *current_point = &lowest_point->points[p];

            // Block pixel (x, y) = current block pixel + offset
            const int16_t pixel_x = current_point->x0 + this->mvec_this.x0;
            const int16_t pixel_y = current_point->y0 + this->mvec_this.y0;

            // Get MacroBlock at that offset
            dc::MacroBlock *current_block = ref_frame->getBlockAtCoord(pixel_x, pixel_y);

            if (p > 0 && !this->isDifferentBlock(*current_block)) {
                // If (clamped) coord is the same as this, skip
                util::deallocVar(current_block);
                continue;
            }

            // Calculate diff with offset block
            const size_t current_diff = this->relativeAbsDifferenceWith(*current_block);

            if (current_diff <= new_lowest_diff) {
                // Block at offset appears better than previously found Block
                util::deallocVar(new_lowest_block);

                new_lowest_point = current_point;
                new_lowest_diff  = current_diff;
                new_lowest_block = current_block;
            } else {
                util::deallocVar(current_block);
            }
        }

        if (new_lowest_point == nullptr) {
            // No other point had a lower diff than current middle => early exit
            util::deallocVar(new_lowest_block);
            break;
        } else {
            util::deallocVar(lowest_block);

            lowest_point = new_lowest_point;
            lowest_block = new_lowest_block;
            lowest_diff  = new_lowest_diff;
        }
    }

    // Relative offset only, this->mvec_this should be added to this value by the decoder
    // to get the pixel coordinate back (now in lowest_block->getCoord()).
    this->mvec.x0 = lowest_point->x0;
    this->mvec.y0 = lowest_point->y0;

    // Expand diff with lowest_block to this->expanded
    this->expandDifferenceWith(*lowest_block);
    util::deallocVar(lowest_block);
}

/**
 *  @brief  Give an upper estimate of the required bytes to represent this Block as encoded data.
 *  @return Returns the length for the Block in bits.
 */
template<size_t size>
size_t dc::Block<size>::streamSize(void) const {
    if (this->rle_Data == nullptr) {
        return dc::Block<size>::SIZE_LEN_BITS   // 4 bits for bit length
             + (size * size * 16u);         // Upper estimate for needed bits
    } else {
        // Exact prediction if RLE sequence is known
        return dc::Block<size>::SIZE_LEN_BITS + (size * size * this->rle_Data->front()->data_bits);
    }
}

/**
 *  @brief  Stream the encoded Block data to the given BitStreamWriter.
 *
 *          1. Write the required bit length for every element to the stream
 *          2. If using RLE, write the amount of data elements that will follow,
 *             else assume it will always be (size*size)
 *          3. Write zeroes and data according to the RLE sequence (RLE sequence is already stored in zig-zag pattern),
 *             limiting to the maximum required bit_len.
 *          4. Append trailing zeroes if length was not reached (not using RLE)
 *
 *  @param  writer
 *      The BitStreamWriter to stream the encoded data to.
 *  @param  use_rle
 *      Whether to use RLE.
 */
template<size_t size>
void dc::Block<size>::streamEncoded(util::BitStreamWriter& writer, bool use_rle) const {
    if (this->rle_Data == nullptr) {
        return;
    }

    const algo::RLE_data_t* info = this->rle_Data->front();
    const uint32_t bit_len = info->data_bits;
           int32_t length  = info->data;

    writer.put(Block::SIZE_LEN_BITS, bit_len);

    // If using RLE, strip last data element and its leading zeroes from the length
    // and add the length to the stream.
    // Else, use the entire Block size as length and don't add the length to the
    // stream, since it will be (size*size) for every Block.
    if (use_rle) {
        if ((length == size * size) && this->rle_Data->back()->zeroes) {
            length -= this->rle_Data->back()->zeroes + 1;  // Loose last zeroes and data
        }

        // Write amount of data elements written
        writer.put(bit_len, uint32_t(length));
    } else {
        // Always use (size * size) amount of data elements if not using rle, so no need to include length
        length = size * size;
    }

    // Iterate over the RLE sequence and add zeroes and the data element, according to
    // the maximum required length to write.
    for (auto start = this->rle_Data->begin() + 1; start != this->rle_Data->end() && length > 0; start++, length--) {
        for (size_t i = (*start)->zeroes; i--;) {
            writer.put(bit_len, 0u);
            length--;
        }
        writer.put(bit_len, uint32_t((*start)->data));
    }

    // Append extra zeroes if length is not reached (when not using rle)
    for (int32_t i = length; i > 0; i--) {
        writer.put(bit_len, 0u);
    }
}

template<size_t size>
void dc::Block<size>::streamMVec(util::BitStreamWriter& writer) const {
    // Write Motion vector to writer
    // This is only the relative offset in pixels from the position of this block,
    // So the full vector would be this->mvec_this + this->mvec

    writer.put(dc::Frame::MVEC_BIT_SIZE, uint32_t(this->mvec.x0));
    writer.put(dc::Frame::MVEC_BIT_SIZE, uint32_t(this->mvec.y0));
}

/**
 *  @brief  Load the Block data from the given BitStreamReader.
 *          Do the reverse as in this->streamEncoded() and read every element
 *          according to its encoded representation.
 *
 *          1. Read the bit length from the stream
 *          2. If using RLE, read the amount of data element that will follow,
 *             else, read (size*size) data elements
 *          3. Read every data element with a maximum bit_len and
 *             store it in the internal double array by the zig-zag positions.
 *             If the amount of data elements was smaller than the size of the array,
 *             the other elements will stay at 0.0 as expected.
 *
 *  @param  reader
 *  @param  use_rle
 */
template<size_t size>
void dc::Block<size>::loadFromStream(util::BitStreamReader &reader, bool use_rle) {
    const size_t bit_len = reader.get(Block::SIZE_LEN_BITS);
    const size_t length  = (use_rle ? (reader.get(bit_len)) : (size * size));

    #ifdef LOG_LOCAL
        util::Logger::WriteLn("Loaded from stream:", false);
        util::Logger::WriteLn(std::string_format("Bits: %d, data: %d", bit_len, length), false);

        size_t start = reader.get_position();

        for (size_t i = 0; i < length; i++) {
            util::Logger::Write(std::string_format("%X ", int16_t(reader.get(bit_len))), false);
        }
        util::Logger::WriteLn("", false);

        reader.set_position(start);
    #endif

    for (size_t i = 0; i < length; i++) {
        const algo::Position_t pos = BlockZigZagLUT[i];
        // Shift data exactly bit_len bits to the left, and shift back to the right
        // to make it properly signed again.
        this->expanded[pos.y * size + pos.x] = util::shift_signed<int16_t>(reader.get(bit_len), bit_len);
    }

    // Fill values that were not read with 0
    for (size_t i = length; i < size * size; i++) {
        const algo::Position_t pos = BlockZigZagLUT[i];
        this->expanded[pos.y * size + pos.x] = 0;
    }
}

template<size_t size>
void dc::Block<size>::loadFromReferenceStream(util::BitStreamReader& reader, dc::Frame * const ref_frame) {
    // Unused for size != dc::MacroBlockSize
    (void)reader;
    (void)ref_frame;
}

template<>
void dc::Block<dc::MacroBlockSize>::loadFromReferenceStream(util::BitStreamReader& reader, dc::Frame * const ref_frame) {
    // Get motion vector offset from stream
    this->mvec.x0 = util::shift_signed<int16_t>(reader.get(dc::Frame::MVEC_BIT_SIZE), dc::Frame::MVEC_BIT_SIZE);
    this->mvec.y0 = util::shift_signed<int16_t>(reader.get(dc::Frame::MVEC_BIT_SIZE), dc::Frame::MVEC_BIT_SIZE);

    const algo::MER_level_t mvec_coord = this->getCoordAfterMotion();

    // Get Macroblock in reference frame at location of motion offset
    dc::MacroBlock *ref_block = ref_frame->getBlockAtCoord(mvec_coord.x0, mvec_coord.y0);

    // Copy values from reference block to this
    ref_block->copyBlockMatrixTo(*this);

    util::deallocVar(ref_block);
}

//////////////////////////////////////////////////////////////////
/// Debug print functions
//////////////////////////////////////////////////////////////////

/**
 *  @brief  Print the internal double matrix in a zig-zag pattern.
 *          (As seen from the wiki)
 *
 *  Example matrix:
 *   0  1  2  3
 *   4  5  6  7
 *   8  9 10 11
 *  12 13 14 15
 *
 *  Printed pattern:
 *   0
 *   1  4
 *   8  5  2
 *   3  6  9 12
 *  13 10  7
 *  11 14
 *  15
 */
template<size_t size>
void dc::Block<size>::printZigzag(void) const {
    size_t line_length = 1, current = 0;
    bool incr = true;

    util::Logger::WriteLn("Zigzag:");

    for (const algo::Position_t& p : BlockZigZagLUT) {
        util::Logger::Write(std::string_format("%3d ", int16_t(this->expanded[p.y * size + p.x])), false);

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

    util::Logger::WriteLn("", false);
}

/**
 *  @brief  Print the RLE sequence (if initialised).
 *
 *          In the form: (#zeroes, #bits)(data)
 *              with #zeroes: the amount of zeroes preceeding this element,
 *                   #bits  : the amount of bits needed to represent this element,
 *                   data   : the data itself.
 *
 *          The first element of the sequence will be printed seperately, as
 *          it is used for information about the entire sequence.
 *          (total bits needed and amount of data+zero elements)
 */
template<size_t size>
void dc::Block<size>::printRLE(void) const {
    bool info = true;

    util::Logger::WriteLn("RLE:");
    if (this->rle_Data == nullptr) {
        return;
    }

    for (const algo::RLE_data_t* e : *this->rle_Data) {
        if (info) {
            util::Logger::WriteLn(std::string_format("Bits needed: %d\n"
                                                     "Data length: %d\n"
                                                     "Sequence   : (#zeroes, #bits)(data)",
                                                     e->data_bits, e->data),
                                  false);
            info = false;
        } else {
            util::Logger::Write(std::string_format("(%d,%d)(%02X), ",
                                                   e->zeroes, e->data_bits, uint8_t(e->data)),
                                false);
        }
    }

    util::Logger::WriteLn("", false);
    util::Logger::WriteLn("", false);
}

/**
 *  @brief  Print the internal double matrix data as an int16.
 */
template<size_t size>
void dc::Block<size>::printExpanded(void) const {
    for (size_t y = 0; y < size; y++) {
        for (size_t x = 0; x < size; x++) {
            util::Logger::Write(std::string_format("% 4d ", int16_t(this->expanded[y * size + x])), false);
        }
        util::Logger::WriteLn("", false);
    }
}

/**
 *  @brief  Print the matrix data as seen inside the bytestream.
 */
template<size_t size>
void dc::Block<size>::printMatrix(void) const {
    for (size_t y = 0; y < size; y++) {
        for (size_t x = 0; x < size; x++) {
            util::Logger::Write(std::string_format("%3d ", this->matrix[y][x]), false);
        }
        util::Logger::WriteLn("", false);
    }
}

/**
 *  @brief  Create the zig-zag positions LUT.
 */
template<size_t size>
void dc::Block<size>::CreateZigZagLUT(void) {
    // TODO Zigzag for Macroblock?

    if (BlockZigZagLUT.size() == 0) {
        util::Logger::WriteLn(std::string_format("[Block] Caching zig-zag pattern for blocksize %d...", dc::BlockSize));
        algo::createZigzagLUT(BlockZigZagLUT, size);
    }
}

template<size_t size>
void dc::Block<size>::CreateMERLUT(const uint16_t& merange) {
    if (BlockMERLUT.points == nullptr) {
        util::Logger::WriteLn(std::string_format("[MacroBlock] Caching motion estimation patterns for merange %d...", merange));
        algo::createMERLUT(BlockMERLUT, merange);
    }
}

template<size_t size>
void dc::Block<size>::DestroyMERLUT(void) {
    algo::destroyMERLUT(BlockMERLUT);
}

