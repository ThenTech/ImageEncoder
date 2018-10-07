#ifndef DECODER_HPP
#define DECODER_HPP

#include "ImageBase.hpp"

namespace dc {
    class Decoder : public ImageProcessor {
        private:

        public:
            Decoder(const std::string &source_file, const std::string &dest_file);
            ~Decoder(void);

            bool process(void);
    };
}

#endif // DECODER_HPP
