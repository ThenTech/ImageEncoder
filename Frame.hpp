#ifndef FRAME_HPP
#define FRAME_HPP

#include "BitStream.hpp"
#include "ImageBase.hpp"

namespace dc {
    /**
     *  @brief  Filler data for decoded UV components.
     */
    static constexpr uint8_t VIDEO_UV_FILL = 0x80;

    class Frame : protected ImageProcessor {
        private:
            const bool is_i_frame;
            Frame * const reference_frame;

        public:
            Frame(uint8_t * const raw, Frame * const reference_frame,
                  const uint16_t& width, const uint16_t& height,
                  const bool &use_rle, MatrixReader<> &quant_m, bool i_frame);
            ~Frame(void);

            inline bool isIFrame(void) const { return this->is_i_frame; }
            size_t streamSize(void) const;

            void streamEncoded(util::BitStreamWriter& writer) const;

            void loadFromStream(util::BitStreamReader& reader);

            bool process(void);
    };
}

#endif // FRAME_HPP
