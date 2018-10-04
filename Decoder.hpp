#ifndef DECODER_HPP
#define DECODER_HPP

#include "ImageBase.hpp"
#include "MatrixReader.hpp"

namespace dc {
    class Decoder : public ImageProcessor {
        private:
            const size_t width;
            const size_t height;
            const size_t rle;
            const MatrixReader<dc::DefaultMatrixSize> &quant_m;

        public:
            Decoder(const std::string &source_file, const std::string &dest_file,
                    const size_t &width, const size_t &height, const size_t &rle,
                    MatrixReader<dc::DefaultMatrixSize> &m);
            ~Decoder(void);

            bool process(void);
    };
}

#endif // DECODER_HPP
