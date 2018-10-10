#ifndef BLOCK_HPP
#define BLOCK_HPP

#include "BitStream.hpp"
#include <vector>

namespace dc {
    static constexpr size_t BlockSize = 4u;

    template<size_t size = dc::BlockSize>
    class Block {
        private:
            uint8_t *matrix[size];
            double   expanded[size * size];
        public:
            Block(uint8_t *row_offset_list[]);
            ~Block(void);

            void forwardDCT(void);
            void inverseDCT(void);

            void quantMult(const double m[]);
            void quantDiv(const double m[]);

            void expand(void) const;

            void processDCTDivQ(const double m[]);
            void processIDCTMulQ(const double m[]);

            void print(void) const;
            void printZigzag(void) const;
            void logExpanded(void) const;

            static void CreateZigZagLUT(void);
    };

    extern template class dc::Block<dc::BlockSize>;
}

#endif // BLOCK_HPP
