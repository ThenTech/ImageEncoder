#ifndef ENCODER_HPP
#define ENCODER_HPP

#include "ImageBase.hpp"
#include "MatrixReader.hpp"

namespace dc {
    /**
     *  @brief  The ImageEncoder class
     *          Used to encode raw images.
     */
    class ImageEncoder : public ImageProcessor {
        private:

        public:
            ImageEncoder(const std::string &source_file, const std::string &dest_file,
                         const uint16_t &width, const uint16_t &height, const bool &use_rle,
                         MatrixReader<> &m);
            ~ImageEncoder(void);

            bool process(void);
            void saveResult(void) const;
    };
}

#endif // ENCODER_HPP
