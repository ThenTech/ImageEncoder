#ifndef VIDEODECODER_HPP
#define VIDEODECODER_HPP

#include "VideoBase.hpp"

namespace dc {
    /**
     *  @brief  The VideoDecoder class
     *          Used to decode videos that were encoded by the VideoEncoder class.
     */
    class VideoDecoder : public VideoProcessor {
        private:

        public:
            VideoDecoder(const std::string &source_file,
                         const std::string &dest_file,
                         const bool &motioncomp);
            ~VideoDecoder(void);

            bool process(void);
            void saveResult(void) const;
    };
}

#endif // VIDEODECODER_HPP
