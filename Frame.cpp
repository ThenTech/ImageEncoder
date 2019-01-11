#include "Frame.hpp"
#include "Logger.hpp"

#include <cassert>

uint8_t dc::Frame::MVEC_BIT_SIZE;


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
    const size_t bits_to_write  = this->writer->get_position();
    const size_t bytes_to_write = bits_to_write / 8u;

    for (size_t byte = 0; byte < bytes_to_write; byte++) {
        writer.put(8, this->writer->get_buffer()[byte]);
    }

    const size_t bits_left = bits_to_write - 8 * bytes_to_write;

    if (bits_left) {
        writer.put(bits_left,
                   this->writer->get_buffer()[bytes_to_write] >> (8 - bits_left));
    }
}

void dc::Frame::loadFromStream(util::BitStreamReader &reader, bool motioncomp) {
    const size_t frame_bytes = this->width * this->height;
    const size_t UV_bytes    = frame_bytes / 2;
    const size_t frame_size  = frame_bytes + UV_bytes;  // 2/3 Y + 1/3 UV data

    this->writer = util::allocVar<util::BitStreamWriter>(frame_size);

    if (this->isIFrame()) {
        // Frame contains only MicroBlocks

        util::Logger::WriteLn("[IFrame] Creating MicroBlocks...");
        dc::ImageProcessor::process(this->writer->get_buffer());

        #ifdef ENABLE_OPENMP
            // Reading raw must happen in sequence
            for (MicroBlock* b : *this->blocks) {
                b->loadFromStream(reader, this->use_rle);
            }

            #pragma omp parallel for schedule(dynamic)
            for (auto it = this->blocks->begin(); it < this->blocks->end(); it++) {
                Block<> *b = *it;
                b->processIDCTMulQ(this->quant_m.getData());
                b->expand();
            }
        #else
            for (MicroBlock* b : *this->blocks) {
                b->loadFromStream(reader, this->use_rle);
                b->processIDCTMulQ(this->quant_m.getData());
                b->expand();
            }
        #endif
    } else {
        // Frame contains mvecs + iframe with motion error diff

        util::Logger::WriteLn("[PFrame] Recreating MacroBlocks...");
        dc::ImageProcessor::processMacroBlocks(this->writer->get_buffer());

        for (MacroBlock* b : *this->macroblocks) {
            b->loadFromReferenceStream(reader, this->reference_frame);
        }

        util::Logger::WriteLn("[PFrame] Recreating MicroBlocks (for motion compansation if enabled)...");
        dc::ImageProcessor::process(this->writer->get_buffer());

        #ifdef ENABLE_OPENMP
            // Reading raw must happen in sequence
            for (MicroBlock* b : *this->blocks) {
                b->loadFromStream(reader, this->use_rle);
            }

            if (motioncomp) {
                #pragma omp parallel for schedule(dynamic)
                for (auto it = this->blocks->begin(); it < this->blocks->end(); it++) {
                    Block<> *b = *it;
                    b->processIDCTMulQ(this->quant_m.getData());
                    b->expandDifferences();
                }
            }
        #else
            for (MicroBlock* b : *this->blocks) {
                b->loadFromStream(reader, this->use_rle);

                if (motioncomp) {
                    // Decode prediction errors
                    b->processIDCTMulQ(this->quant_m.getData());
                    b->expandDifferences();
                } else {
                    // Just consume the prediction error compensation iframe
                }
            }
        #endif
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

        #ifdef ENABLE_OPENMP
            #pragma omp parallel for schedule(dynamic)
            for (auto it = this->blocks->begin(); it < this->blocks->end(); it++) {
                MicroBlock *b = *it;
                b->processDCTDivQ(this->quant_m.getData());
                b->createRLESequence();
            }

            // Writing results must happen in sequence
            for (MicroBlock* b : *this->blocks) {
                b->streamEncoded(*this->writer, this->use_rle);
            }
        #else
            for (MicroBlock* b : *this->blocks) {
                b->processDCTDivQ(this->quant_m.getData());
                b->createRLESequence();
                b->streamEncoded(*this->writer, this->use_rle);
            }
        #endif
    } else {
        util::Logger::WriteLn("[PFrame] Creating MacroBlocks...");
        // Create Macroblocks that reference current raw frame
        dc::ImageProcessor::processMacroBlocks(this->reader->get_buffer());

        // Also create MicroBlocks to encode expanded motion prediction errors
        dc::ImageProcessor::process(this->reader->get_buffer());

        // Output for all
        const size_t output_length = (this->macroblocks->size()
                                       * dc::Frame::MVEC_BIT_SIZE * 2)  ///< 2 values for mvec for each block
                                   + util::round_to_byte(              ///< Size of resulting predict error iframe
                                         this->blocks->size()
                                       * this->blocks->front()->streamSize());

        // Final output for PFrame (mvecs + encoded me-error frame)
        this->writer = util::allocVar<util::BitStreamWriter>(output_length);

        util::Logger::WriteLn("[PFrame] Processing MacroBlocks...");

        #ifdef ENABLE_OPENMP
            #pragma omp parallel for schedule(dynamic)
            for (auto it = this->macroblocks->begin(); it < this->macroblocks->end(); it++) {
                MacroBlock *b = *it;
                b->processFindMotionOffset(this->reference_frame);
                this->copyMacroblockToMatchingMicroblocks(*b);

                const algo::MER_level_t mvec_coord = b->getCoordAfterMotion();
                dc::MacroBlock *ref_block = this->reference_frame->getBlockAtCoord(
                                                mvec_coord.x0, mvec_coord.y0);
                ref_block->copyBlockMatrixTo(*b);
                util::deallocVar(ref_block);
            }

            // Writing results must happen in sequence
            for (MacroBlock* b : *this->macroblocks) {
                b->streamMVec(*this->writer);
            }

            #pragma omp parallel for schedule(dynamic)
            for (auto it = this->blocks->begin(); it < this->blocks->end(); it++) {
                MicroBlock *b = *it;
                b->expandDifferences();
            }

            // Writing results must happen in sequence
            for (MicroBlock* b : *this->blocks) {
                b->streamEncoded(*this->writer, this->use_rle);
            }
        #else
            for (MacroBlock* b : *this->macroblocks) {
                b->processFindMotionOffset(this->reference_frame);

                // Motion vector offset now in b->mvec
                // Actual vector offset = b->mvec + b->mvec_this
                // Prediction error now within b->expanded

                // Expand b->expanded to the same Microbloks->expanded and encode
                this->copyMacroblockToMatchingMicroblocks(*b);

                // Copy ref_frame MacroBlock to this, for better motion estimation in next frame
                const algo::MER_level_t mvec_coord = b->getCoordAfterMotion();
                dc::MacroBlock *ref_block = this->reference_frame->getBlockAtCoord(
                                                mvec_coord.x0, mvec_coord.y0);
                ref_block->copyBlockMatrixTo(*b);
                util::deallocVar(ref_block);

                // Write mvec for each frame to output
                b->streamMVec(*this->writer);
            }

            // this->blocks[*]->expanded now has the expanded values from this->macroblocks
            // Process them now again as IFrame
            // + Write Prediction error IFrame after mvecs
            for (MicroBlock* b : *this->blocks) {
                // Expand previously encoded and decoded diffs back into self
                // b->matrix was already replaced by ref_frame (copyBlockMatrixTo),
                // b->expanded still contains decoded diffs, so just add back together.
                b->expandDifferences();

                // Write previously encoded RLE sequence to stream
                b->streamEncoded(*this->writer, this->use_rle);
            }
        #endif
    }

    return true;
}

dc::MacroBlock* dc::Frame::getBlockAtCoord(int16_t x, int16_t y) const {
    return dc::ImageProcessor::getBlockAtCoord(x, y);
}
