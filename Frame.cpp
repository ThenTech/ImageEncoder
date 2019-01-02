#include "Frame.hpp"
#include "Logger.hpp"

dc::Frame::Frame(uint8_t * const raw, const uint16_t& width, const uint16_t& height,
                 const bool &use_rle, MatrixReader<> &quant_m)
    : ImageProcessor(raw, width, height, use_rle, quant_m)
{

}

dc::Frame::~Frame(void) {
    // Empty
}

size_t dc::Frame::streamSize(void) const {
    return this->width * this->height * 8u;
}

void dc::Frame::streamEncoded(util::BitStreamWriter& writer) const {
    // TODO

    const size_t bits_to_write  = this->writer->get_position();
    const size_t bytes_to_write = bits_to_write / 8u;

    for (size_t byte = 0; byte < bytes_to_write; byte++) {
        writer.put(8, this->writer->get_buffer()[byte]);
    }

    writer.put(bits_to_write - 8 * bytes_to_write,
               this->writer->get_buffer()[bytes_to_write]);


//    uint8_t *buf = writer.get_buffer() + writer.get_last_byte_position();

//    // Copy Y data
//    std::copy_n(this->writer->get_buffer(),
//                this->writer->get_last_byte_position(),
//                buf);

//    writer.set_position(writer.get_position() + this->writer->get_position());
}

void dc::Frame::loadFromStream(util::BitStreamReader &reader) {
    const size_t frame_bytes = this->width * this->height;

    this->writer = util::allocVar<util::BitStreamWriter>(frame_bytes * 1.5);

    util::Logger::WriteLn("[Frame] Creating blocks...");
    dc::ImageProcessor::process(this->writer->get_buffer());

    for (Block<>* b : *this->blocks) {
        b->loadFromStream(reader, this->use_rle);
        b->processIDCTMulQ(this->quant_m.getData());
        b->expand();
    }

    this->writer->set_position(frame_bytes * 8u);

    // For expanding to decoded
    // Pad UV data
    uint8_t *buf = this->writer->get_buffer() + this->writer->get_last_byte_position();
    size_t uv_size = frame_bytes / 2u;
    std::fill(buf,
              buf + uv_size,
              dc::VIDEO_UV_FILL);

    this->writer->set_position(this->writer->get_size() * 8u);
}

bool dc::Frame::process(void) {
    // TODO

    util::Logger::WriteLn("[Frame] Creating blocks...");
    dc::ImageProcessor::process(this->reader->get_buffer());

    const size_t output_length = util::round_to_byte(this->blocks->size()
                                                   * this->blocks->front()->streamSize());

    this->writer = util::allocVar<util::BitStreamWriter>(output_length);

    util::Logger::WriteLn("[Frame] Processing Blocks...");
    for (Block<>* b : *this->blocks) {
        b->processDCTDivQ(this->quant_m.getData());
        b->createRLESequence();
        b->streamEncoded(*this->writer, this->use_rle);
    }

    return true;
}
