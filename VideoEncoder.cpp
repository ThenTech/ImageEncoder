#include "VideoEncoder.hpp"
#include "main.hpp"
#include "Logger.hpp"

#include <cassert>

dc::VideoEncoder::VideoEncoder(const std::string &source_file, const std::string &dest_file,
                               const uint16_t &width, const uint16_t &height, const bool &use_rle,
                               MatrixReader<> &m, const uint16_t &gop, const uint16_t &merange,
                               const bool &motioncomp)
    : VideoProcessor(source_file, dest_file, width, height, use_rle, m, gop, merange, motioncomp)
{
    assert(this->width  % dc::BlockSize == 0);
    assert(this->height % dc::BlockSize == 0);
    assert(this->reader->get_size() % size_t(this->width * this->height) == 0);
}

dc::VideoEncoder::~VideoEncoder(void) {
    // Empty
}

bool dc::VideoEncoder::process(void) {
    bool success = true;

    util::Logger::WriteLn("[VideoEncoder] Processing video...");

    // Pre-process image
    success = VideoProcessor::process(this->reader->get_buffer());

    // Write setting header
    util::Logger::WriteLn("[VideoEncoder] Creating settings header...");
    size_t output_length;

    // TODO
    output_length = 0;

    this->writer = util::allocVar<util::BitStreamWriter>(output_length);

    // TODO

    const size_t frame_count = this->frames->size();
    size_t frameid = 0u;

    util::Logger::WriteLn("[VideoEncoder] Processing Frames...");
    util::Logger::WriteProgress(0, frame_count);

    // TODO
    for (dc::ImageBase* f : *this->frames) {
        f; // TODO
        util::Logger::WriteProgress(++frameid, frame_count);
    }

    util::Logger::WriteLn("", false);

    return success;
}

void dc::VideoEncoder::saveResult(void) const {
    VideoProcessor::saveResult(true);
}
