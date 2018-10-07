#ifndef ENCODER_HPP
#define ENCODER_HPP

#include "ImageBase.hpp"
#include "MatrixReader.hpp"

namespace dc {
    class Encoder : public ImageProcessor {
        private:

        public:
            Encoder(const std::string &source_file, const std::string &dest_file,
                    const uint16_t &width, const uint16_t &height, const bool &use_rle,
                    MatrixReader<> &m);
            ~Encoder(void);

            bool process(void);
            void saveResult(void) const;
    };
}

#endif // ENCODER_HPP
