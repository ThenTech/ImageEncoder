#include "VideoDecoder.hpp"
#include "main.hpp"
#include "Logger.hpp"

#include <cassert>

dc::VideoDecoder::VideoDecoder(const std::string &source_file,
                               const std::string &dest_file,
                               const bool &motioncomp)
    : VideoProcessor(source_file, dest_file, motioncomp)
{
    // Verify settings
    assert(this->width  % dc::BlockSize == 0);
    assert(this->height % dc::BlockSize == 0);

    // Same data stats before decoding starts
    const float hdrlen = float(this->reader->get_position()) / 8.0f;
    const float datlen = float(this->reader->get_size()) - hdrlen;

    util::Logger::WriteLn(std::string_format("[VideoDecoder] Loaded %dx%d video with "
                                             "%.1f bytes header and %.1f bytes data.",
                                             this->width, this->height, hdrlen, datlen));

    // Create the output buffer
    this->writer = util::allocVar<util::BitStreamWriter>(this->width * this->height);
}

dc::VideoDecoder::~VideoDecoder(void) {
    // Empty
}

bool dc::VideoDecoder::process(void) {
    bool success = true;

    util::Logger::WriteLn("[VideoDecoder] Processing image...");

    success = VideoProcessor::process(this->writer->get_buffer());

    const size_t frame_count = this->frames->size();
    size_t frameid = 0u;

    util::Logger::WriteLn("[VideoDecoder] Processing Frames...");
    util::Logger::WriteProgress(0, frame_count);

    // TODO
    for (dc::Frame* f : *this->frames) {
        f; // TODO
        util::Logger::WriteProgress(++frameid, frame_count);
    }

    util::Logger::WriteLn("", false);

    // Buffer is written implicitly
    this->writer->set_position(this->writer->get_size_bits());

    return success;
}

void dc::VideoDecoder::saveResult(void) const {
    VideoProcessor::saveResult(false);
}
