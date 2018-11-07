#include "ImageBase.hpp"
#include "utils.hpp"
#include "Logger.hpp"
#include "Block.hpp"
#include "Huffman.hpp"

/**
 *  @brief  Default ctor
 *
 *  @param  source_file
 *      Path to a raw image file.
 *  @param  width
 *      The width in pixels for the image.
 *  @param  height
 *      The height in pixels for the image.
 */
dc::ImageBase::ImageBase(const std::string &source_file, const uint16_t &width, const uint16_t &height)
    : width(width), height(height)
{
    try {
        this->raw = util::readBinaryFile(source_file);
    } catch (Exceptions::FileReadException const& e) {
        util::Logger::WriteLn(e.getMessage());
        exit(-1);
    }

    this->reader = util::allocVar<util::BitStreamReader>(this->raw->data(), this->raw->size());
}

/**
 *  @brief  Default dtor
 */
dc::ImageBase::~ImageBase(void) {
    util::deallocVar(this->raw);
    util::deallocVar(this->reader);
}

////////////////////////////////////////////////////////////////////////////////////

/**
 *  @brief  Default ctor for encoder.
 *
 *  @param  source_file
 *      Path to a raw image file.
 *  @param  dest_file
 *      Path to the destination file (path needs to exist, file will be overwritten).
 *  @param  width
 *      The width in pixels for the image.
 *  @param  height
 *      The height in pixels for the image.
 *  @param  use_rle
 *      Whether to use Run-Length Encoding.
 *      Currently enabling this will drop trailing zeroes in an encoded block.
 *  @param  quant_m
 *      The quantization matrix to use.
 */
dc::ImageProcessor::ImageProcessor(const std::string &source_file,
                                   const std::string &dest_file,
                                   const uint16_t &width, const uint16_t &height,
                                   const bool &use_rle, MatrixReader<> &quant_m)
    : ImageBase(source_file, width, height),
      use_rle(use_rle), quant_m(quant_m),
      dest_file(dest_file),
      blocks(util::allocVar<std::vector<Block<>*>>())
{
    // Empty
}

/**
 *  @brief  Default ctor for decoder, settings need to be determined from the reader stream.
 *
 *  @param  source_file
 *      Path to a raw image file.
 *  @param  dest_file
 *      Path to the destination file (path needs to exist, file will be overwritten).
 */
dc::ImageProcessor::ImageProcessor(const std::string &source_file, const std::string &dest_file)
    : ImageBase(source_file, 0u, 0u),                            ///< Create stream
      dest_file(dest_file),
      blocks(util::allocVar<std::vector<Block<>*>>())
{
    // Assume input is encoded image and settings should be determined from the bytestream

    // Perform Huffman decompress (if used, first bit is '1' else '0')
    algo::Huffman<> hm;
    util::BitStreamReader *hm_output = hm.decode(*this->reader);
    util::Logger::WriteLn("", false);

    #ifdef LOG_LOCAL
        util::Logger::WriteLn("\n", false);
        hm.printDict();
        util::Logger::WriteLn("\n", false);
    #endif

    // Replace reader with result from decompressed Huffman stream
    if (hm_output != nullptr) {
        util::deallocVar(this->reader);
        this->reader = hm_output;
    }

    // Read Matrix
    this->quant_m = dc::MatrixReader<>::fromBitstream(*this->reader);

    // Read other settings in same order as they were presumably written to the encoded stream
    this->use_rle = this->reader->get(dc::ImageProcessor::RLE_BITS);
    this->width   = uint16_t(this->reader->get(dc::ImageProcessor::DIM_BITS));
    this->height  = uint16_t(this->reader->get(dc::ImageProcessor::DIM_BITS));
}

/**
 *  @brief  Default dtor
 */
dc::ImageProcessor::~ImageProcessor(void) {
    util::deallocVector(this->blocks);
    util::deallocVar(this->writer);
}

/**
 *  @brief  Start processing by creating blocks for the given stream.
 *
 *  @param  source_block_buffer
 *      The source strean te create blocks from.
 *      Usually the reader stream when encoding, and the writer stream when decoding.
 *  @return Returns true on success.
 */
bool dc::ImageProcessor::process(uint8_t * const source_block_buffer) {
    util::Logger::WriteLn("[ImageProcessor] Creating blocks...");
    uint8_t *block_starts[dc::BlockSize] = { nullptr };

    constexpr size_t block_size = dc::BlockSize * dc::BlockSize;    ///< Total values inside 1 Block
    const     size_t blockx     = this->width  / dc::BlockSize;     ///< Amount of Blocks on a row
    const     size_t blocky     = this->height / dc::BlockSize;     ///< Amount of Blocks in a column

    // Reserve space for blocks
    this->blocks->reserve(blockx * blocky);

    // Get only pointers to start of each block row and save to Block in this->blocks
    for (size_t b_y = 0; b_y < blocky; b_y++) {                     ///< Block y coord
        for (size_t b_x = 0; b_x < blockx; b_x++) {                 ///< Block x coord
            for (size_t y = 0; y < dc::BlockSize; y++) {            ///< Row inside block
                block_starts[y] = (source_block_buffer +            // Buffer start
                                   (  (b_y * block_size * blockx)   // Block row start
                                    + (b_x * dc::BlockSize)         // Block column start
                                    + (y * this->width)             // Row within block
                                    + (0)));                        // Column withing block
            }

            // Create new block with the found row offsets
            this->blocks->push_back(util::allocVar<dc::Block<>>(block_starts));
        }
    }

    // Create zig-zag-pattern LUT
    util::Logger::WriteLn(std::string_format("[ImageProcessor] Caching zig-zag pattern for blocksize %d...", dc::BlockSize));
    Block<dc::BlockSize>::CreateZigZagLUT();

    return true;
}

/**
 *  @brief  Save the writer stream to this->dest_file,
 *          and give some compression stats.
 *
 *  @param  encoded
 *          Whether this was called after encoding (true) or decoding (false).
 */
void dc::ImageProcessor::saveResult(bool encoded) const {
    const size_t total_length = this->writer->get_last_byte_position();  // Total final write length in bytes

    #if 1
        // This will only write whole bytes; if pos % 8 != 0, last byte is skipped
        // Edit: Padding is now added after settings, so only whole bytes are in buffer.
        util::writeBinaryFile(this->dest_file,
                              this->writer->get_buffer(),
                              total_length);
    #else
        // Will take last byte into account
        std::ofstream file(this->dest_file, std::ofstream::binary);
        util::write(file, *this->writer);
    #endif

    util::Logger::WriteLn(std::string_format("[ImageProcessor] Original file size: %8d bytes", this->raw->size()));
    util::Logger::WriteLn(std::string_format("[ImageProcessor]       %scoded size: %8d bytes  => Ratio: %.2f%%",
                                             (encoded ? "En" : "De"),
                                             total_length,
                                             (float(total_length) / this->raw->size() * 100)));
    util::Logger::WriteLn("[ImageProcessor] Saved file at: " + this->dest_file);
}
