#include "Encoder.hpp"
#include "Logger.hpp"

dc::Encoder::Encoder(const std::string &source_file, const std::string &dest_file,
                     const size_t &width, const size_t &height, const size_t &rle,
                     MatrixReader<dc::DefaultMatrixSize> &m)
    : ImageProcessor(source_file, dest_file), width(width), height(height), rle(rle), quant_m(m)
{

}

dc::Encoder::~Encoder(void) {

}

bool dc::Encoder::process(void) {
    util::Logger::Write("[Encoder] Processing image...");

    // TODO
    util::Logger::Write("TODO Do work");

    return ImageProcessor::process();
}
