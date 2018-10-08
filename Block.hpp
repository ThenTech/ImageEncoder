#ifndef BLOCK_HPP
#define BLOCK_HPP

#include "BitStream.hpp"
#include <array>

namespace dc {
    static constexpr size_t BlockSize = 4u;

    template<size_t size = dc::BlockSize>
    class Block {
        private:
            double *matrix[size];
            double  expanded[size * size];
        public:
            Block(double *row_offset_list[]);
            ~Block(void);

            void forwardDCT(void);
            void inverseDCT(void);

            void quantMult(const double m[]);
            void quantDiv(const double m[]);

            void expand(void) const;
            void print(void) const;
            void logExpanded(void) const;
    };

    extern template class dc::Block<dc::BlockSize>;
}

#endif // BLOCK_HPP
