#include "VideoBase.hpp"
#include "utils.hpp"
#include "Logger.hpp"


dc::VideoBase::VideoBase(const std::string &source_file, const uint16_t &width, const uint16_t &height)
    : ImageBase(source_file, width, height)
    , frame_buffer_size(width * height)
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
dc::VideoBase::~VideoBase(void) {
    util::deallocVar(this->raw);
    util::deallocVar(this->reader);
}

dc::VideoProcessor::VideoProcessor(const std::string &source_file, const std::string &dest_file,
                                   const uint16_t &width, const uint16_t &height,
                                   const bool &use_rle, MatrixReader<> &quant_m,
                                   const uint16_t &gop, const uint16_t &merange,
                                   const bool &motioncomp)
    : VideoBase(source_file, width, height)
    , use_rle(use_rle), quant_m(quant_m)
    , gop(gop), merange(merange), motioncomp(motioncomp)
    , dest_file(dest_file)
    , frames(util::allocVar<std::vector<dc::ImageBase*>>())
{
    // Empty
}

dc::VideoProcessor::VideoProcessor(const std::string &source_file, const std::string &dest_file,
                                   const bool &motioncomp)
    : VideoBase(source_file, 0, 0)
    , motioncomp(motioncomp)
    , dest_file(dest_file)
    , frames(util::allocVar<std::vector<dc::ImageBase*>>())
{
    // TODO decode
}

/**
 *  @brief  Default dtor
 */
dc::VideoProcessor::~VideoProcessor(void) {
    util::deallocVector(this->frames);
    util::deallocVar(this->writer);
}

bool dc::VideoProcessor::process(uint8_t * const source_frame_buffer) {

    return false;
}

void dc::VideoProcessor::saveResult(bool encoded) const {
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

    util::Logger::WriteLn(std::string_format("[VideoProcessor] Original file size: %8d bytes", this->raw->size()));
    util::Logger::WriteLn(std::string_format("[VideoProcessor]       %scoded size: %8d bytes  => Ratio: %.2f%%",
                                             (encoded ? "En" : "De"),
                                             total_length,
                                             (float(total_length) / this->raw->size() * 100)));
    util::Logger::WriteLn("[VideoProcessor] Saved file at: " + this->dest_file);
}
