#include "Frame.hpp"

dc::Frame::Frame(uint8_t * const raw, const uint16_t& width, const uint16_t& height,
                 const bool &use_rle, MatrixReader<> &quant_m)
    : ImageProcessor(raw, width, height, use_rle, quant_m)
{

}

size_t dc::Frame::streamSize(void) const {
    return this->width * this->height;
}

void dc::Frame::streamEncoded(util::BitStreamWriter& writer) const {
    // TODO

    uint8_t *buf = writer.get_buffer() + writer.get_last_byte_position();

    // Copy Y data
    std::copy_n(this->reader->get_buffer(),
                this->streamSize(),
                buf);

    writer.set_position(writer.get_position() + this->streamSize() * 8u);

    // For expanding to decoded
//    // Pad UV data
//    buf = writer.get_buffer() + writer.get_last_byte_position();
//    size_t uv_size = (this->width * this->height / 2);
//    std::fill(buf,
//              buf + uv_size,
//              dc::VIDEO_UV_FILL);

//    writer.set_position(writer.get_position() + uv_size * 8u);
}

bool dc::Frame::process(void) {
    // TODO
    return true;
}
