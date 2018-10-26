#ifndef BLOCK_HPP
#define BLOCK_HPP

#include "BitStream.hpp"
#include "algo.hpp"
#include <vector>

namespace dc {
    /**
     *  @brief  The size (width and height) for a Block inside an image.
     *          This is used everywhere and can be changed if needed.
     */
    static constexpr size_t BlockSize = 4u;

    /**
     *  @brief  The Block class
     *          Represents a block, linked to the reader/writer stream,
     *          and can be used to perform Block-level operations.
     *
     * @tparam  size
     *          The used Block size.
     */
    template<size_t size = dc::BlockSize>
    class Block {
        private:
            uint8_t *matrix[size];
            double   expanded[size * size];
            std::vector<algo::RLE_data_t*> *rle_Data;
        public:
            Block(uint8_t *row_offset_list[]);
            ~Block(void);

            void expand(void) const;

            void processDCTDivQ(const double m[]);
            void processIDCTMulQ(const double m[]);

            void createRLESequence(void);

            size_t streamSize(void) const;
            void streamEncoded(util::BitStreamWriter&, bool) const;

            void loadFromStream(util::BitStreamReader&, bool);

            void printZigzag(void) const;
            void printRLE(void) const;
            void printExpanded(void) const;
            void printMatrix(void) const;

            static void CreateZigZagLUT(void);
            static constexpr size_t SIZE_LEN_BITS = 4;  ///< The amount of bits to use to represent the bit length of values insode the Block.
    };

    extern template class dc::Block<dc::BlockSize>;
}

#endif // BLOCK_HPP
