#ifndef IMAGEBASE_HPP
#define IMAGEBASE_HPP

#include <string>
#include <vector>

#include "BitStream.hpp"
#include "Block.hpp"
#include "MatrixReader.hpp"

namespace dc {
    /**
     *  @brief  The ImageBase class
     */
    class ImageBase {
        protected:
            uint16_t width;
            uint16_t height;

            std::vector<uint8_t>  *raw;
            util::BitStreamReader *reader;
        public:
            ImageBase(const std::string &source_file, const uint16_t &width, const uint16_t &height);
            ~ImageBase(void);
    };

    ////////////////////////////////////////////////////////////////////////////////////

    /**
     *  @brief  The ImageProcessor class
     */
    class ImageProcessor : protected ImageBase {
        protected:
            bool use_rle;
            const MatrixReader<> quant_m;

            const std::string     &dest_file;
            std::vector<Block<>*> *blocks;

            double *pixeldata;
        public:
            ImageProcessor(const std::string &source_file, const std::string &dest_file,
                           const uint16_t &width, const uint16_t &height,
                           const bool &use_rle, MatrixReader<> &quant_m);
            ImageProcessor(const std::string &source_file, const std::string &dest_file);
            virtual ~ImageProcessor(void);

            virtual bool process(void);
            virtual void saveResult(bool with_settings=false) const;

            static constexpr size_t RLE_BITS = 1;
            static constexpr size_t DIM_BITS = 15;
    };
}

#endif // IMAGEBASE_HPP
