#include "MatrixReader.hpp"

#include <iostream>
#include <iomanip>
#include <cassert>

#include "utils.hpp"
#include "Exceptions.hpp"

template<size_t size>
dc::MatrixReader<size>::MatrixReader(uint32_t *matrix) {
    std::copy_n(matrix, size * size, this->matrix);
    std::copy_n(matrix, size * size, this->expanded);
}

template<size_t size>
dc::MatrixReader<size>::MatrixReader() : matrix{0} {

}

template<size_t size>
dc::MatrixReader<size>::~MatrixReader() {

}

template<size_t size>
dc::MatrixReader<> dc::MatrixReader<size>::fromBitstream(util::BitStreamReader &reader) {
    const uint32_t bit_size = reader.get(dc::MatrixReader<>::SIZE_LEN_BITS);
    uint32_t matrix[size * size];

    for (size_t y = 0; y < size; y++) {
        for (size_t x = 0; x < size; x++) {
            matrix[y * size + x] = reader.get(bit_size);
        }
    }

    return dc::MatrixReader<>(matrix);
}

template<size_t size>
bool dc::MatrixReader<size>::read(const std::string &fileName) {
    const std::string *data = nullptr;

    try {
        data = util::readStringFromFile(fileName);
    } catch (Exceptions::FileReadException const& e) {
        util::deallocVar(data);
        std::cerr << "[MatrixReader] " << e.getMessage() << std::endl;
        return false;
    }

    std::string line, item;
    std::stringstream ss(*data);
    bool exception = false;
    size_t file_row, file_col;

    for (file_row = 0; std::getline(ss, line) && !exception; ++file_row) {
        if (file_row >= size) {
            std::cerr << "[MatrixReader] Too many rows in matrix! Expected "
                      << size << " but got " << file_row << " or more!" << std::endl;
            exception = true;
            break;
        }

        std::trim(line);
        std::strReplaceConsecutive(line, ' ');

        std::stringstream iss(line);

        for (file_col = 0; std::getline(iss, item, ' '); ++file_col) {
            if (file_col >= size) {
                std::cerr << "[MatrixReader] Too many cols in matrix! Expected "
                          << size << " but got " << file_col << " or more!" << std::endl;
                exception = true;
                break;
            }

            try {
                this->matrix[file_row * size + file_col] = util::lexical_cast<uint16_t>(item.c_str());
                //printf("Var => %s \tcast = %d\n", item.c_str(), this->matrix[file_row][file_col]);
            } catch (Exceptions::CastingException const& e) {
                exception = true;
                std::cerr << "[MatrixReader] " << e.getMessage() << std::endl;
                break;
            }
        }

        if (file_col < size) {
            std::cerr << "[MatrixReader] Too little cols in matrix! Expected "
                      << size << " but got " << file_col << "!" << std::endl;
            exception = true;
            break;
        }
    }

    if (!exception && (file_row < size)) {
        std::cerr << "[MatrixReader] Too little rows in matrix! Expected "
                  << size << " but got " << file_row << "!" << std::endl;
        exception = true;
    }

    if (!exception) {
        std::copy_n(this->matrix, size * size, this->expanded);
    }

    util::deallocVar(data);

    return !exception;
}

template<size_t size>
void dc::MatrixReader<size>::write(util::BitStreamWriter &writer) const {
    const uint8_t quant_bit_len = this->getMaxBitLength();

    // Assert that quant_bit_len actually fits in dc::MatrixReader<>::SIZE_LEN_BITS bits
    assert((quant_bit_len & ((1 << dc::MatrixReader<>::SIZE_LEN_BITS) - 1))
           == quant_bit_len);

    writer.put(dc::MatrixReader<>::SIZE_LEN_BITS, quant_bit_len);
    for (size_t y = 0; y < size; y++) {
        for (size_t x = 0; x < size; x++) {
            writer.put(quant_bit_len, this->matrix[y * size + x]);
        }
    }
}

template<size_t size>
const std::string dc::MatrixReader<size>::toString(void) const {
    size_t row, col;
    std::ostringstream oss;

    for (row = 0; row < size; row++) {
        for (col = 0; col < size; col++) {
            oss << std::setw(4) << this->matrix[row * size + col];
        }
        oss << std::endl;
    }

    return oss.str();
}

template<size_t size>
uint8_t dc::MatrixReader<size>::getMaxBitLength(void) const {
    uint8_t length = 0u;

    for (size_t i = 0; i < size * size; i++) {
        length = std::max(length, util::ffs(this->matrix[i]));
    }

    return length;
}

template<size_t size>
const double* dc::MatrixReader<size>::getData() const {
    return this->expanded;
}

template class dc::MatrixReader<dc::BlockSize>;