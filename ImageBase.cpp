#include "utils.hpp"
#include "Logger.hpp"
#include "ImageBase.hpp"
#include "Block.hpp"

dc::ImageBase::ImageBase(const std::string &source_file, const uint16_t &width, const uint16_t &height)
    : width(width), height(height)
{
    try {
        this->raw = util::readBinaryFile(source_file);
    } catch (Exceptions::FileReadException const& e) {
        util::Logger::Write(e.getMessage());
        exit(-1);
    }

    this->reader = util::allocVar<util::BitStreamReader>(this->raw->data(), this->raw->size());
}

dc::ImageBase::~ImageBase(void) {
    util::deallocVar(this->raw);
    util::deallocVar(this->reader);
}

////////////////////////////////////////////////////////////////////////////////////

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

dc::ImageProcessor::ImageProcessor(const std::string &source_file, const std::string &dest_file)
    : ImageBase(source_file, 0u, 0u),
      quant_m(dc::MatrixReader<>::fromBitstream(*this->reader)),
      dest_file(dest_file),
      blocks(util::allocVar<std::vector<Block<>*>>())
{
    // Assume input is encoded image and settings should be determined from the bytestream

    // Matrix was already read here

    this->use_rle = this->reader->get(dc::ImageProcessor::RLE_BITS);
    this->width   = uint16_t(this->reader->get(dc::ImageProcessor::DIM_BITS));
    this->height  = uint16_t(this->reader->get(dc::ImageProcessor::DIM_BITS));

    // Add padding to next whole byte
    this->reader->set_position(this->reader->get_position() +
                               (8 - (this->reader->get_position() % 8u)));
}

dc::ImageProcessor::~ImageProcessor(void) {
    util::deallocVector(this->blocks);
}

bool dc::ImageProcessor::process(void) {
    util::Logger::Write("[ImageProcessor] Creating blocks...");
    uint8_t *block_starts[dc::BlockSize] = { nullptr };
    size_t   b_y = 0, b_x = 0, y;

    constexpr size_t block_size = dc::BlockSize * dc::BlockSize;
    const     size_t blockx     = this->width  / dc::BlockSize;
    const     size_t blocky     = this->height / dc::BlockSize;

    // Start of buffer with position offset
    uint8_t *buffer_start = this->reader->get_buffer() + (this->reader->get_position() / 8u);

    // Get only pointers to start of each block row and save to Block in this->blocks
    for (b_y = 0; b_y < blocky; b_y++) {
        for (b_x = 0; b_x < blockx; b_x++) {
            for (y = 0; y < dc::BlockSize; y++) {
                block_starts[y] = (buffer_start +                   // Buffer start
                                   (  (b_y * block_size * blockx)   // Block row start
                                    + (b_x * dc::BlockSize)         // Block column start
                                    + (y * this->width)             // Row within block
                                    + (0)));                        // Column withing block
            }
            this->blocks->push_back(util::allocVar<dc::Block<>>(block_starts));
        }
    }

    return true;
}

void dc::ImageProcessor::saveResult(bool with_settings) const {
    size_t output_length, hdrlen = 0u, padding = 0u;

    if (with_settings) {
        const uint8_t quant_bit_len = this->quant_m.getMaxBitLength();
        output_length = dc::ImageProcessor::RLE_BITS       // Bit for RLE setting
                      + dc::ImageProcessor::DIM_BITS * 2u  // 2 times bits for image dimension
                      + dc::MatrixReader<>::SIZE_LEN_BITS  // Bits to signify size of quant_;atrix contents
                      + (quant_bit_len                     // Size of quantmatrix
                         * dc::BlockSize * dc::BlockSize);
        padding        = 8 - (output_length % 8u);         // Padding to next whole byte
        output_length += padding;

        hdrlen = output_length / 8u;

        util::Logger::Write("[ImageProcessor] Settings header length: "
                            + std::to_string(hdrlen)
                            + " bytes.");

        output_length += this->reader->get_size() * 8u;    // TODO size of encoded image
    } else {
        output_length = (this->reader->get_size() * 8u) - this->reader->get_position();
    }

    output_length /= 8u;

    util::BitStreamWriter *writer = util::allocVar<util::BitStreamWriter>(output_length);

    if (with_settings) {
        // Write matrix data first
        this->quant_m.write(*writer);

        // Write other settings
        writer->put(dc::ImageProcessor::RLE_BITS, uint32_t(this->use_rle));
        writer->put(dc::ImageProcessor::DIM_BITS, this->width);
        writer->put(dc::ImageProcessor::DIM_BITS, this->height);
        writer->put(padding, 0);

        // Write output buffer
        std::copy_n(this->reader->get_buffer(),
                    output_length - hdrlen,
                    writer->get_buffer() + (writer->get_position() / 8u));
    } else {
        // Write output buffer
        std::copy_n(this->reader->get_buffer() + (this->reader->get_position() / 8u),
                    output_length,
                    writer->get_buffer());
    }

    #if 1
        // This will only write whole bytes; if pos % 8 != 0, last byte is skipped
        // Edit: Padding is now added after settings, so only whole bytes are in buffer.
        util::writeBinaryFile(this->dest_file,
                              writer->get_buffer(),
                              writer->get_size());
    #else
        // Will take last byte into account
        std::ofstream file(this->dest_file, std::ofstream::binary);
        util::write(file, *this->writer);
    #endif

    util::Logger::Write("[ImageProcessor] Total file length: "
                        + std::to_string(writer->get_size())
                        + " bytes (" + std::to_string(float(writer->get_size()) / this->raw->size() * 100.f)
                        + "% " + (with_settings ? "" : "de") + "compression).");
    util::Logger::Write("[ImageProcessor] Saved file at: " + this->dest_file);
    util::deallocVar(writer);
}
