#include "Decoder.hpp"
#include "Logger.hpp"


dc::Decoder::Decoder(const std::string &source_file, const std::string &dest_file,
                     const size_t &width, const size_t &height, const size_t &rle,
                     MatrixReader<dc::DefaultMatrixSize> &m)
    : ImageProcessor(source_file, dest_file), width(width), height(height), rle(rle), quant_m(m)
{

}

dc::Decoder::~Decoder(void) {

}

bool dc::Decoder::process(void) {
    util::Logger::Write("[Decoder] Processing image...");

    // TODO
    util::Logger::Write("TODO Do work");

    return ImageProcessor::process();
}
