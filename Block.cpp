#include "Block.hpp"
#include "algo.hpp"

template<size_t size>
dc::Block<size>::Block(uint8_t *row_offset_list[])
    : matrix{nullptr}
{
    std::copy_n(row_offset_list, size, this->matrix);
}

template<size_t size>
dc::Block<size>::~Block() {

}

template<size_t size>
void dc::Block<size>::forwardDCT(void) {
    double expanded[size * size];

    for (size_t y = 0; y < size; y++) {
        for (size_t x = 0; x < size; x++) {
            expanded[y * size + x] = this->matrix[y][x];
        }
    }

    algo::transformDCTinverse(expanded, size * size);
}

template<size_t size>
void dc::Block<size>::inverseDCT(void) {

}

template<size_t size>
void dc::Block<size>::print() const {
    for (size_t y = 0; y < size; y++) {
        for (size_t x = 0; x < size; x++) {
            putchar(this->matrix[y][x] ? ' ' : 219);
//            printf("%2X ", this->matrix[y][x]);
        }
        putchar('\n');
    }
}

template class dc::Block<dc::BlockSize>;
