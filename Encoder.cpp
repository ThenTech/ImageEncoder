#include "Encoder.hpp"
#include "main.hpp"
#include "Logger.hpp"
#include "utils.hpp"

#include <cassert>

/**
 *  @brief  dc::Encoder::Encoder
 *
 *  @param  source_file
 *      Path to a raw image file.
 *  @param  dest_file
 *      Path to the destination file (path needs to exist, file will be overwritten).
 *  @param  width
 *  @param  height
 *  @param  use_rle
 *  @param  quant_m
 */
dc::Encoder::Encoder(const std::string &source_file, const std::string &dest_file,
                     const uint16_t &width, const uint16_t &height, const bool &use_rle,
                     MatrixReader<> &quant_m)
    : ImageProcessor(source_file, dest_file, width, height, use_rle, quant_m)
{
    assert(this->width  % dc::BlockSize == 0);
    assert(this->height % dc::BlockSize == 0);
    assert(this->reader->get_size() == size_t(this->width * this->height));
}

/**
 *  @brief  Default dtor
 */
dc::Encoder::~Encoder(void) {
    // Empty
}

/**
 *  @brief  Process the raw image for encoding.
 *
 *          1. Create Blocks
 *          2. Determine header length
 *          3. Estimate final stream length (header + size for each Block)
 *          4. Write header (encoding settings)
 *      For each Block:
 *          5. Perform DCT and divide with the quant_matrix
 *          6. Create the RLE sequence
 *          7. Stream the results to the byte stream, ignoring trailing zeroes if use_rle == true
 *
 *  @return Returns true on success.
 */
bool dc::Encoder::process(void) {
    bool success = true;

    util::Logger::WriteLn("[Encoder] Processing image...");

    // Pre-process image
    success = ImageProcessor::process(this->reader->get_buffer());

    // Write setting header
    util::Logger::WriteLn("[Encoder] Creating settings header...");
    size_t output_length;

    const uint8_t quant_bit_len = this->quant_m.getMaxBitLength();
    output_length = dc::ImageProcessor::RLE_BITS       // Bit for RLE setting
                  + dc::ImageProcessor::DIM_BITS * 2u  // 2 times bits for image dimension
                  + dc::MatrixReader<>::SIZE_LEN_BITS  // Bits to signify size of quant_matrix contents
                  + (quant_bit_len                     // Size of quantmatrix
                     * dc::BlockSize * dc::BlockSize);

    util::Logger::WriteLn(std::string_format("[Encoder] Settings header length: %.1f bytes.",
                                             float(output_length) / 8.f));

    output_length += this->blocks->size() * this->blocks->front()->streamSize();
    output_length += 8 - (output_length % 8u);         // Padding to next whole byte
    output_length /= 8u;

    this->writer = util::allocVar<util::BitStreamWriter>(output_length);

    // Write matrix data first
    this->quant_m.write(*this->writer);

    // Write other settings
    this->writer->put(dc::ImageProcessor::RLE_BITS, uint32_t(this->use_rle));
    this->writer->put(dc::ImageProcessor::DIM_BITS, this->width);
    this->writer->put(dc::ImageProcessor::DIM_BITS, this->height);

    util::Logger::WriteLn("[Encoder] Processing Blocks...");

    #ifdef LOG_LOCAL
        size_t blockid = 0u;

        for (Block<>* b : *this->blocks) {
            util::Logger::WriteLn(std::string_format("Block % 3d:", blockid++));
            b->printExpanded();
            util::Logger::WriteLn("", false);

            util::Logger::WriteLn("After DCT and quantization:");
            b->processDCTDivQ(this->quant_m.getData());
            b->printExpanded();
            util::Logger::WriteLn("", false);

            b->printZigzag();
            b->createRLESequence();
            b->printRLE();

            b->streamEncoded(*this->writer, this->use_rle);
            util::Logger::WriteLn("", false);
        }
    #else
        for (Block<>* b : *this->blocks) {
            b->processDCTDivQ(this->quant_m.getData());
            b->createRLESequence();
            b->streamEncoded(*this->writer, this->use_rle);
        }
    #endif

    return success;
}

/**
 *  @brief  Save the resulting stream to the destination.
 */
void dc::Encoder::saveResult(void) const {
    ImageProcessor::saveResult(true);
}
