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

    util::Logger::WriteLn("[Encoder] Processing image...");

    // Pre-process image
    success = ImageProcessor::process();

    // TODO Write encoder
    util::Logger::WriteLn("Loaded image:");
    size_t x, y;

    for (y = 0; y < this->height; y++) {
        for (x = 0; x < this->width; x++) {
            util::Logger::Write(this->reader->get(8) ? util::Logger::EMPTY : util::Logger::FILL, false);
        }
        util::Logger::WriteLn("", false);
    }

    util::Logger::WriteLn("", false);
    util::Logger::WriteLn("", false);


    util::Logger::WriteLn("Blocks:");
    for (Block<>* b : *this->blocks) {
        b->print();

        // Before
        util::Logger::WriteLn("Expanded:");
        b->logExpanded();
        util::Logger::WriteLn("", false);

        // Forward DCT
        util::Logger::WriteLn("Forward DCT:");
        b->forwardDCT();
        b->logExpanded();
        util::Logger::WriteLn("", false);

        // Quantize
        util::Logger::WriteLn("Quantized:");
        b->quantDiv(this->quant_m.getData());
        b->logExpanded();
        util::Logger::WriteLn("", false);

        // Un-quantize
        util::Logger::WriteLn("Un-quantize:");
        b->quantMult(this->quant_m.getData());
        b->logExpanded();
        util::Logger::WriteLn("", false);

        // Inverse DCT  => should be the same as the first??
        util::Logger::WriteLn("Inverse DCT:");
        b->inverseDCT();
        b->logExpanded();
        util::Logger::WriteLn("", false);
    }

    // TODO Discrete Cosine Transform on every block
//    for (Block<>* b : *this->blocks) {
//        b->forwardDCT();
//    }

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
