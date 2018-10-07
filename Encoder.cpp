#include "Encoder.hpp"
#include "Logger.hpp"

#include <cassert>

dc::Encoder::Encoder(const std::string &source_file, const std::string &dest_file,
                     const uint16_t &width, const uint16_t &height, const bool &use_rle,
                     MatrixReader<> &quant_m)
    : ImageProcessor(source_file, dest_file, width, height, use_rle, quant_m)
{
    assert(this->width  % dc::BlockSize == 0);
    assert(this->height % dc::BlockSize == 0);
    assert(this->reader->get_size() == size_t(this->width * this->height));
}

dc::Encoder::~Encoder(void) {

}

bool dc::Encoder::process(void) {
    bool success = true;

    util::Logger::Write("[Encoder] Processing image...");

    // Pre-process image
    success = ImageProcessor::process();

    // TODO Write encoder
    util::Logger::Write("TODO Do work");
    size_t x, y;

    for (y = 0; y < this->height; y++) {
        for (x = 0; x < this->width; x++) {
            putchar(this->reader->get(8) ? ' ' : 219);
        }
        putchar('\n');
    }   putchar('\n'); putchar('\n'); putchar('\n');


    for (const Block<>* b : *this->blocks) {
        b->print();
        putchar('\n');
    }

    // TODO Discrete Cosine Transform on every block
    for (Block<>* b : *this->blocks) {
        b->forwardDCT();
        putchar('\n');
    }

    // TODO Quantize every block with this->quant_m
    // TODO Zig-zag store results of each block in a list
    // TODO Perform run-length encoding to list if this->use_rle -- true
    // TODO Write output to file with decoding info like (this->quant_m, this->use_rle, width, height)
    //      with minimum amount of bits


    return success;
}

void dc::Encoder::saveResult(void) const {
    ImageProcessor::saveResult(true);
}
