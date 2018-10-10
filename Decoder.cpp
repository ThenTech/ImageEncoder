#include "Decoder.hpp"
#include "Logger.hpp"

#include <cassert>

dc::Decoder::Decoder(const std::string &source_file, const std::string &dest_file)
    : ImageProcessor(source_file, dest_file)
{
    // TODO Get decoding info like (this->quant_m, this->use_rle, width, height)
    //      from the raw bytestream instead.

    assert(this->width  % dc::BlockSize == 0);
    assert(this->height % dc::BlockSize == 0);

    const size_t hdrlen = this->reader->get_position() / 8u;
    const size_t datlen = this->reader->get_size() - hdrlen;

    assert(datlen == size_t(this->width * this->height));

    util::Logger::WriteLn("[Decoder] Loaded image with "
                        + std::to_string(hdrlen) + " bytes header and "
                        + std::to_string(datlen) + " bytes data.");
}

dc::Decoder::~Decoder(void) {

}

bool dc::Decoder::process(void) {
    bool success = true;

    util::Logger::WriteLn("[Decoder] Processing image...");

    success = ImageProcessor::process();

    // TODO Write decoder
    util::Logger::WriteLn("TODO Do work");

    return success;
}
