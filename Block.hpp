#ifndef BLOCK_HPP
#define BLOCK_HPP

#include "BitStream.hpp"

namespace dc {
    static constexpr size_t BlockSize = 4u;

    template<size_t size = dc::BlockSize>
    class Block {
        private:
            uint8_t *matrix[size];
        public:
            Block(uint8_t *row_offset_list[]);
            ~Block(void);

            void forwardDCT(void);
            void inverseDCT(void);

            void print(void) const;
    };

    extern template class dc::Block<dc::BlockSize>;
}

#endif // BLOCK_HPP
