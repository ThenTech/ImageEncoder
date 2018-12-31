#ifndef VIDEOBASE_HPP
#define VIDEOBASE_HPP

#include "ImageBase.hpp"
#include "Frame.hpp"

namespace dc {
    class VideoBase : protected ImageBase {
        protected:
            size_t frame_buffer_size;     // Y data size
            size_t frame_garbage_size;    // UV data size (if inlcuded, e.g. only when encoding)
        public:
            VideoBase(const std::string &source_file, const uint16_t &width, const uint16_t &height);
            ~VideoBase(void);
    };

    class VideoProcessor : protected VideoBase {
        protected:
            size_t frame_count;         ///< Amount of frames in inputbuffer.

            bool use_rle;               ///< Whether to use Run Length Encoding.
            MatrixReader<> quant_m;     ///< A quantization matrix instance.

            uint16_t   gop;             ///<
            uint16_t   merange;         ///<
            const bool motioncomp;      ///<

            const std::string           &dest_file; ///< The path to the destination file.
            std::vector<dc::Frame*>     *frames;    ///< A list of every Image for the video.
            util::BitStreamWriter       *writer;    ///< The output stream.

            void saveResult(bool) const;
            bool process(uint8_t * const);
        public:
            VideoProcessor(const std::string &source_file, const std::string &dest_file,
                           const uint16_t &width, const uint16_t &height,
                           const bool &use_rle, MatrixReader<> &quant_m,
                           const uint16_t &gop, const uint16_t &merange,
                           const bool &motioncomp);
            VideoProcessor(const std::string &source_file, const std::string &dest_file,
                           const bool &motioncomp);

            virtual ~VideoProcessor(void);

            virtual bool process(void)=0;
            virtual void saveResult(void) const {}
    };
}

#endif // VIDEOBASE_HPP