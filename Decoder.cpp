#include "Decoder.hpp"
#include "main.hpp"
#include "Logger.hpp"

#include <cassert>

/**
 *  @brief  Default ctor
 *
 *  @param  source_file
 *      Path to a raw image file.
 *  @param  dest_file
 *      Path to the destination file (path needs to exist, file will be overwritten).
 */
dc::Decoder::Decoder(const std::string &source_file, const std::string &dest_file)
    : ImageProcessor(source_file, dest_file)
{
    // Decoding info like (this->quant_m, this->use_rle, width, height)
    // is gathered in the ImageProcessor ctor.

    // Verify settings
    assert(this->width  % dc::BlockSize == 0);
    assert(this->height % dc::BlockSize == 0);

    // Same data stats before decoding starts
    const float hdrlen = float(this->reader->get_position()) / 8.0f;
    const float datlen = float(this->reader->get_size()) - hdrlen;

    util::Logger::WriteLn(std::string_format("[Decoder] Loaded image with "
                                             "%.1f bytes header and %.1f bytes data.",
                                             hdrlen, datlen));

    // Create the output buffer
    this->writer = util::allocVar<util::BitStreamWriter>(this->width * this->height);
}

/**
 *  @brief  Default dtor
 */
dc::Decoder::~Decoder(void) {
    // Empty
}

/**
 *  @brief  Process the image for decoding.
 *
 *          1. Create Blocks
 *      For each Block:
 *          2. Load the encoded Block data from the stream
 *          3. Perform iDCT and multiply with trhe quant_matrix
 *          4. Expand the results to the byte stream
 *
 *  @return Returns true on success.
 */
bool dc::Decoder::process(void) {
    bool success = true;

    util::Logger::WriteLn("[Decoder] Processing image...");

    success = ImageProcessor::process(this->writer->get_buffer());

    util::Logger::WriteLn("[Decoder] Processing Blocks...");

    #ifdef LOG_LOCAL
        size_t blockid = 0u;

        for (Block<>* b : *this->blocks) {
            util::Logger::WriteLn(std::string_format("Block % 3d:", blockid++));

            b->loadFromStream(*this->reader, this->use_rle);
            b->printExpanded();
            util::Logger::WriteLn("", false);

            util::Logger::WriteLn("Reverse DCT and de-quantization:");
            b->processIDCTMulQ(this->quant_m.getData());
            b->printExpanded();
            util::Logger::WriteLn("", false);

            util::Logger::WriteLn("Expanded:");
            b->expand();
            b->printMatrix();
            util::Logger::WriteLn("", false);
            util::Logger::WriteLn("", false);
        }
    #else
        for (Block<>* b : *this->blocks) {
            b->loadFromStream(*this->reader, this->use_rle);
            b->processIDCTMulQ(this->quant_m.getData());
            b->expand();
        }
    #endif

    // Buffer is written implicitly
    this->writer->set_position(this->writer->get_size() * 8u);

    return success;
}

/**
 *  @brief  Save the resulting stream to the destination.
 */
void dc::Decoder::saveResult(void) const {
    ImageProcessor::saveResult(false);
}
