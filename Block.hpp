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
    static constexpr size_t BlockSize      =  4u;
    static constexpr size_t MacroBlockSize = 16u;

    class Frame;

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

            algo::MER_level_t mvec_this;
            algo::MER_level_t mvec;
        public:
            Block(uint8_t *row_offset_list[]);
            Block(uint8_t *row_offset_list[], int16_t x, int16_t y);
            ~Block(void);

            void expand(void) const;
            void expandDifferences(void) const;

            // Microblocks
            void processDCTDivQ(const double m[]);
            void processIDCTMulQ(const double m[]);

            void createRLESequence(void);

            // Macroblocks
            void updateRows(uint8_t *row_offset_list[]);
            inline uint8_t* getRow(size_t) const;
            size_t relativeAbsDifferenceWith(const dc::Block<dc::MacroBlockSize>&);
            void expandDifferenceWith(const dc::Block<dc::MacroBlockSize>&);
            void processFindMotionOffset(dc::Frame * const ref_frame);
            inline algo::MER_level_t getCoord(void) const;
            inline bool isDifferentCoord(const int16_t&, const int16_t&) const;
            inline bool isDifferentCoord(const algo::MER_level_t&) const;
            inline bool isDifferentBlock(const dc::Block<dc::MacroBlockSize>&) const;
            void expandBlock(dc::Block<size>&) const;
            void loadFromReferenceStream(util::BitStreamReader&, dc::Frame * const);

            size_t streamSize(void) const;
            void streamEncoded(util::BitStreamWriter&, bool) const;
            void streamMVec(util::BitStreamWriter&) const;

            void loadFromStream(util::BitStreamReader&, bool);

            void printZigzag(void) const;
            void printRLE(void) const;
            void printExpanded(void) const;
            void printMatrix(void) const;           

            static void CreateZigZagLUT(void);
            static void CreateMERLUT(const uint16_t &merange);
            static void DestroyMERLUT(void);

            static constexpr size_t SIZE_LEN_BITS = 4;  ///< The amount of bits to use to represent the bit length of values inside the Block.
    };

    extern template class dc::Block<dc::BlockSize>;
    extern template class dc::Block<dc::MacroBlockSize>;

    using MicroBlock = dc::Block<dc::BlockSize>;
    using MacroBlock = dc::Block<dc::MacroBlockSize>;
}

#endif // BLOCK_HPP
