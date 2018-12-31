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

        public:
            Frame(uint8_t * const raw, const uint16_t& width, const uint16_t& height,
                  const bool &use_rle, MatrixReader<> &quant_m);

            size_t streamSize(void) const;
            void streamEncoded(util::BitStreamWriter& writer) const;

            bool process(void);
    };
}

#endif // FRAME_HPP
