#include "Frame.hpp"
#include "Logger.hpp"

#include <cassert>

uint8_t dc::Frame::GOP_BIT_SIZE;


dc::Frame::Frame(uint8_t * const raw, Frame * const reference_frame,
                 const uint16_t& width, const uint16_t& height,
                 const bool &use_rle, MatrixReader<> &quant_m, bool i_frame)
    : ImageProcessor(raw, width, height, use_rle, quant_m)
    , is_i_frame(i_frame)
    , reference_frame(reference_frame)
{
    // Empty
}

dc::Frame::~Frame(void) {
    // Empty
}

size_t dc::Frame::streamSize(void) const {
    // Raw/Decoded        : this->width * this->height * 8u;
    // Encode  isIFrame() : std::reduce(this->blocks, [](const Block<>& b) { return b.streamSize(); }, 0)
    // Encode !isIFrame() : (this->width * this->height) / (dc::MacroBlockSize * dc::MacroBlockSize) * dc::Frame::VectorBits * 2

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
    const size_t UV_bytes    = frame_bytes / 2;
    const size_t frame_size  = frame_bytes + UV_bytes;  // 2/3 Y + 1/3 UV data

    this->writer = util::allocVar<util::BitStreamWriter>(frame_size);

    if (this->isIFrame()) {
        // Frame contains only MicroBlocks

        util::Logger::WriteLn("[IFrame] Creating MicroBlocks...");
        dc::ImageProcessor::process(this->writer->get_buffer());

        for (MicroBlock* b : *this->blocks) {
            b->loadFromStream(reader, this->use_rle);
            b->processIDCTMulQ(this->quant_m.getData());
            b->expand();
        }
    } else {
        // Frame contains mvecs + iframe with motion error diff

        util::Logger::WriteLn("[PFrame] Recreating MacroBlocks...");
        dc::ImageProcessor::processMacroBlocks(this->writer->get_buffer());

        for (MacroBlock* b : *this->macroblocks) {
            b->loadFromReferenceStream(reader, this->reference_frame);
        }

        // TODO Motion compensation?
        util::Logger::WriteLn("[PFrame] Recreating MicroBlocks (for motion compansation if enabled)...");
        dc::ImageProcessor::process(this->writer->get_buffer());

        for (MicroBlock* b : *this->blocks) {

            b->printExpanded();

            b->printMatrix();

            b->loadFromStream(reader, this->use_rle);
            b->processIDCTMulQ(this->quant_m.getData());
            b->expandDifferences();
        }
    }

    // For expanding to decoded, fill UV data
    std::fill_n(this->writer->get_buffer() + frame_bytes,
                UV_bytes,
                dc::VIDEO_UV_FILL);

    this->writer->set_position(this->writer->get_size() * 8u);
}

bool dc::Frame::process(void) {
    if (this->isIFrame()) {
        util::Logger::WriteLn("[IFrame] Creating MicroBlocks...");
        dc::ImageProcessor::process(this->reader->get_buffer());

        const size_t output_length = util::round_to_byte(this->blocks->size()
                                                       * this->blocks->front()->streamSize());

        this->writer = util::allocVar<util::BitStreamWriter>(output_length);

        util::Logger::WriteLn("[IFrame] Processing MicroBlocks...");
        for (MicroBlock* b : *this->blocks) {
            b->processDCTDivQ(this->quant_m.getData());
            b->createRLESequence();
            b->streamEncoded(*this->writer, this->use_rle);
        }
    } else {
        util::Logger::WriteLn("[PFrame] Creating MacroBlocks...");
        // Create Macroblocks that reference current raw frame
        dc::ImageProcessor::processMacroBlocks(this->reader->get_buffer());

        // Out for Macroblocks expanded => target for motion estimation error
        util::BitStreamWriter *expanded_buffer = util::allocVar<util::BitStreamWriter>(this->reader->get_size());

        // Encode prediction errors as new IFrame
        dc::Frame block_processor(expanded_buffer->get_buffer(), nullptr,
                                  this->width, this->height, this->use_rle,
                                  this->quant_m, true);

        // Create Macroblocks referencing new expanded_buffer
        block_processor.processMacroBlocks(expanded_buffer->get_buffer());

        // Output for all
        const size_t output_length = (this->macroblocks->size()
                                       * dc::Frame::GOP_BIT_SIZE * 2)   ///< 2 values for mvec for each block
                                   + (expanded_buffer->get_size())      ///< Size of resulting frame
                                   ;
        // Final output for PFrame (mvecs + encoded me-error frame)
        this->writer = util::allocVar<util::BitStreamWriter>(output_length);

        util::Logger::WriteLn("[PFrame] Processing MacroBlocks...");

        auto target_blocks = block_processor.macroblocks->begin();

        for (MacroBlock* b : *this->macroblocks) {
            b->processFindMotionOffset(this->reference_frame);

            // Motion vector offset now in b->mvec
            // Actual vector offset = b->mvec + b->mvec_this
            // Prediction error now within b->expanded

            // Encode b->expanded as if it where a new dc::MacroBlockSize * dc::MacroBlockSize image
            // by writing b->expanded to new buffer and encoding that one as this->blocks

            // Expand b->expanded to the same Microbloks.expanded to allow encoding

            b->expandBlock(**target_blocks);

            // Write mvec for each frame to output
            b->streamMVec(*this->writer);

            target_blocks++;
        }

        // Buffer in block_processor now has the expanded values from this->macroblocks
        // Process them now again as IFrame
        block_processor.process();

        // Write Prediction error IFrame after mvecs
        block_processor.streamEncoded(*this->writer);
        util::deallocVar(expanded_buffer);
    }

    return true;
}

dc::MacroBlock* dc::Frame::getBlockAtCoord(int16_t x, int16_t y) const {
    return dc::ImageProcessor::getBlockAtCoord(x, y);
}
