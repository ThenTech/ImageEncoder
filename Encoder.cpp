#include "Encoder.hpp"
#include "Logger.hpp"
#include "utils.hpp"

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
    size_t blockid = 0u;

    for (Block<>* b : *this->blocks) {
        util::Logger::WriteLn(std::string_format("Block % 3d:", blockid++));
        b->print();
        util::Logger::WriteLn("", false);

        util::Logger::WriteLn("After DCT and quantization:");
        b->processDCTDivQ(this->quant_m.getData());
        b->print();
        util::Logger::WriteLn("", false);

//        util::Logger::WriteLn("Reverse DCT and quantization:");
//        b->processIDCTMulQ(this->quant_m.getData());
//        b->print();
//        util::Logger::WriteLn("", false);

        util::Logger::WriteLn("Zigzag:");
        b->printZigzag();
        util::Logger::WriteLn("", false);
    }

    // TODO Discrete Cosine Transform on every block
    // Ok? TODO check more performant algo

    // TODO Quantize every block with this->quant_m
    // Ok

    // TODO Zig-zag store results of each block in a list
    // TODO Perform run-length encoding to list if this->use_rle == true

    // TODO Write output to file with decoding info like (this->quant_m, this->use_rle, width, height)
    //      with minimum amount of bits
    // Ok


    return success;
}

void dc::Encoder::saveResult(void) const {
    ImageProcessor::saveResult(true);
}
