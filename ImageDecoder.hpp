#ifndef DECODER_HPP
#define DECODER_HPP

#include "ImageBase.hpp"

namespace dc {
    /**
     *  @brief  The ImageDecoder class
     *          Used to decode images that were encoded by the ImageEncoder class.
     */
    class ImageDecoder : public ImageProcessor {
        private:

        public:
            ImageDecoder(const std::string &source_file, const std::string &dest_file);
            ~ImageDecoder(void);

            bool process(void);
            void saveResult(void) const;
    };
}

#endif // DECODER_HPP
