#ifndef VIDEOENCODER_HPP
#define VIDEOENCODER_HPP

#include "VideoBase.hpp"

namespace dc {
    /**
     *  @brief  The VideoEncoder class
     *          Used to encode raw videos.
     */
    class VideoEncoder : public VideoProcessor {
        public:
            VideoEncoder(const std::string &source_file, const std::string &dest_file,
                         const uint16_t &width, const uint16_t &height, const bool &use_rle,
                         MatrixReader<> &m, const uint16_t &gop, const uint16_t &merange,
                         const bool &motioncomp);
            ~VideoEncoder(void);

            bool process(void);
            void saveResult(void) const;
    };
}

#endif // VIDEOENCODER_HPP
