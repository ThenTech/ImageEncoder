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
     *          Provides a base with the image dimensions and the raw byte buffer
     *          read into an std::vector and accessible through a BitStreamReader instance.
     */
    class ImageBase {
        protected:
            uint16_t width;                 ///< The width of the image.
            uint16_t height;                ///< The height of the image.

            std::vector<uint8_t>  *raw;     ///< The raw input stream.
            util::BitStreamReader *reader;  ///< A BitStreamReader linked to the raw input stream.
        public:
            ImageBase(const std::string &source_file, const uint16_t &width, const uint16_t &height);
            ImageBase(uint8_t * const raw, const uint16_t &width, const uint16_t &height);
            ~ImageBase(void);
    };

    ////////////////////////////////////////////////////////////////////////////////////

    /**
     *  @brief  The ImageProcessor class
     */
    class ImageProcessor : protected ImageBase {
        protected:
            bool use_rle;                   ///< Whether to use Run Length Encoding.
            MatrixReader<> quant_m;         ///< A quantization matrix instance.

            const std::string     &dest_file; ///< The path to the destination file.

            std::vector<dc::MicroBlock*> *blocks;       ///< A list of every Block for the image.
            std::vector<dc::MacroBlock*> *macroblocks;  ///< A list of every MacroBlock for the image.

            util::BitStreamWriter *writer;  ///< The output stream.

            void saveResult(bool) const;
            bool process(uint8_t * const);
            bool processMacroBlocks(uint8_t * const);

        public:
            ImageProcessor(const std::string &source_file, const std::string &dest_file,
                           const uint16_t &width, const uint16_t &height,
                           const bool &use_rle, MatrixReader<> &quant_m);
            ImageProcessor(const std::string &source_file, const std::string &dest_file);

            ImageProcessor(uint8_t * const raw,
                           const uint16_t &width, const uint16_t &height,
                           const bool &use_rle, MatrixReader<> &quant_m);

            virtual ~ImageProcessor(void);

            /**
             *  @brief  Process the image, needs to be implemented in a child class.
             *          A child class can call ImageProcessor::process(buffer) to create
             *          block from the buffer.
             */
            virtual bool process(void)=0;
            virtual void saveResult(void) const {}

            dc::MacroBlock* getBlockAtCoord(int16_t, int16_t) const;

            static constexpr size_t RLE_BITS = 1u;   ///< The amount of bits to use to represent zhether to use RLE or not.
            static constexpr size_t DIM_BITS = 15u;  ///< The amount of bits to use to represent the image dimensions (width or height).
    };
}

#endif // IMAGEBASE_HPP
