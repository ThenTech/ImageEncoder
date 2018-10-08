#include "Block.hpp"
#include "algo.hpp"
#include "Logger.hpp"
#include "utils.hpp"

#include <algorithm>
#include <functional>

template<size_t size>
dc::Block<size>::Block(double *row_offset_list[])
    : matrix{nullptr}
{
    std::copy_n(row_offset_list, size, this->matrix);

    for (size_t y = 0; y < size; y++) {
        std::copy_n(this->matrix[y], size, &this->expanded[y * size]);
    }
}

template<size_t size>
dc::Block<size>::~Block() {

}

template<size_t size>
void dc::Block<size>::forwardDCT(void) {
    algo::transformDCT(this->expanded, size * size);
}

template<size_t size>
void dc::Block<size>::inverseDCT(void) {
    algo::transformDCTinverse(this->expanded, size * size);
}

template<size_t size>
void dc::Block<size>::quantMult(const double m[]) {
    // Divide every element from this->expanded with an element in m on the same index
    std::transform(this->expanded, this->expanded + size * size,
                   m,
                   this->expanded,
                   std::multiplies<double>());
}

template<size_t size>
void dc::Block<size>::quantDiv(const double m[]) {
    // Multiply every element from this->expanded with an element in m on the same index
    std::transform(this->expanded, this->expanded + size * size,
                   m,
                   this->expanded,
                   std::divides<double>());
}

template<size_t size>
void dc::Block<size>::expand() const {
    for (size_t y = 0; y < size; y++) {
//        for (size_t x = 0; x < size; x++) {
//            this->matrix[y][x] = this->expanded[y * size + x];
//        }
        std::copy_n(&this->expanded[y * size], size, this->matrix[y]);
    }
}

template<size_t size>
void dc::Block<size>::print() const {
    for (size_t y = 0; y < size; y++) {
        for (size_t x = 0; x < size; x++) {
//            printf("%2X ", this->matrix[y][x]);
            util::Logger::Write(std::string_format("%02X ", uint8_t(this->matrix[y][x])), false);
//            util::Logger::Write(this->matrix[y][x] > 0.0 ? util::Logger::EMPTY : util::Logger::FILL, false);
        }
        util::Logger::WriteLn("", false);
    }
}

template<size_t size>
void dc::Block<size>::logExpanded() const {
    for (size_t y = 0; y < size; y++) {
        for (size_t x = 0; x < size; x++) {
            util::Logger::Write(std::string_format("%8.2f ", this->expanded[y * size + x]), false);
        }
        util::Logger::WriteLn("", false);
    }
}

template class dc::Block<dc::BlockSize>;
