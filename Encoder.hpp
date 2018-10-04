#ifndef ENCODER_HPP
#define ENCODER_HPP

#include "ImageBase.hpp"
#include "MatrixReader.hpp"

namespace dc {
    class Encoder : public ImageProcessor {
        private:
            const size_t width;
            const size_t height;
            const size_t rle;
            const MatrixReader<dc::DefaultMatrixSize> &quant_m;

        public:
            Encoder(const std::string &source_file, const std::string &dest_file,
                    const size_t &width, const size_t &height, const size_t &rle,
                    MatrixReader<dc::DefaultMatrixSize> &m);
            ~Encoder(void);

            bool process(void);
    };
}

#endif // ENCODER_HPP
