#include "VideoEncoder.hpp"
#include "main.hpp"
#include "Logger.hpp"
#include "Huffman.hpp"

#include <cassert>

dc::VideoEncoder::VideoEncoder(const std::string &source_file, const std::string &dest_file,
                               const uint16_t &width, const uint16_t &height, const bool &use_rle,
                               MatrixReader<> &m, const uint16_t &gop, const uint16_t &merange,
                               const bool &motioncomp)
    : VideoProcessor(source_file, dest_file, width, height, use_rle, m, gop, merange, motioncomp)
{
    assert(this->width  % dc::BlockSize == 0);
    assert(this->height % dc::BlockSize == 0);
    assert(this->reader->get_size() % size_t(this->frame_buffer_size + this->frame_garbage_size) == 0);
}

dc::VideoEncoder::~VideoEncoder(void) {
    dc::MacroBlock::DestroyMERLUT();
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
    const uint8_t quant_bit_len = this->quant_m.getMaxBitLength();
    output_length = dc::ImageProcessor::RLE_BITS       // Bit for RLE setting
                  + dc::ImageProcessor::DIM_BITS * 2u  // 2 times bits for video dimension
                  + dc::MatrixReader<>::SIZE_LEN_BITS  // Bits to signify size of quant_matrix contents
                  + (quant_bit_len                     // Size of quantmatrix
                     * dc::BlockSize * dc::BlockSize)
                  + dc::ImageProcessor::DIM_BITS       // Amount of frames
                  + dc::ImageProcessor::DIM_BITS       // gop
                  + dc::ImageProcessor::DIM_BITS       // merange
                  ;

    util::Logger::WriteLn(std::string_format("[VideoEncoder] Settings header length: %.1f bytes.",
                                             float(output_length) / 8.f));

    output_length += this->frames->size() * this->frames->front()->streamSize();

    #ifndef ENABLE_HUFFMAN
        output_length++;    // Add one bit to signal Huffman is not enabled.
    #endif

    output_length = util::round_to_byte(output_length);  // Padding to next whole byte


    this->writer = util::allocVar<util::BitStreamWriter>(output_length);

    #ifndef ENABLE_HUFFMAN
        this->writer->put_bit(0); // '0': No Huffman sequence present.
    #endif

    // Write matrix data first
    this->quant_m.write(*this->writer);

    // Write other settings
    this->writer->put(dc::ImageProcessor::RLE_BITS, uint32_t(this->use_rle));
    this->writer->put(dc::ImageProcessor::DIM_BITS, this->width);
    this->writer->put(dc::ImageProcessor::DIM_BITS, this->height);
    this->writer->put(dc::ImageProcessor::DIM_BITS, uint32_t(this->frame_count));
    this->writer->put(dc::ImageProcessor::DIM_BITS, uint32_t(this->gop));
    this->writer->put(dc::ImageProcessor::DIM_BITS, uint32_t(this->merange));

    dc::MacroBlock::CreateMERLUT(this->merange);

    const size_t frame_count = this->frames->size();
    size_t frameid = 0u;

    util::Logger::WriteLn("[VideoEncoder] Processing Frames...");
    util::Logger::WriteProgress(0, frame_count);

    // TODO
    for (dc::Frame* f : *this->frames) {
        util::Logger::Pause();

        f; // TODO

        // TODO motion vectors : https://web.stanford.edu/class/ee398a/handouts/lectures/EE398a_MotionEstimation_2012.pdf
        f->process();
        f->streamEncoded(*this->writer);

        util::Logger::Resume();
        util::Logger::WriteProgress(++frameid, frame_count);
    }

    util::Logger::WriteLn("", false);

    #ifdef ENABLE_HUFFMAN
        util::BitStreamReader hm_input(this->writer->get_buffer(),
                                       this->writer->get_last_byte_position());

        algo::Huffman<> hm;
        util::BitStreamWriter *hm_output = hm.encode(hm_input);

        if (hm_output != nullptr) {
            util::deallocVar(this->writer);
            this->writer = hm_output;
        }

        util::Logger::WriteLn("", false);
    #endif

    return success;
}

void dc::VideoEncoder::saveResult(void) const {
    VideoProcessor::saveResult(true);
}
