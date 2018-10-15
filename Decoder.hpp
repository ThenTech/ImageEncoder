#ifndef DECODER_HPP
#define DECODER_HPP

#include "ImageBase.hpp"

namespace dc {
    /**
     *  @brief  The Decoder class
     *          Used to decode images that were encoded by the Encoder class.
     */
    class Decoder : public ImageProcessor {
        private:

        public:
            Decoder(const std::string &source_file, const std::string &dest_file);
            ~Decoder(void);

            bool process(void);
            void saveResult(void) const;
    };
}

#endif // DECODER_HPP
