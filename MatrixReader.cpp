#include "MatrixReader.hpp"

#include <iostream>
#include <iomanip>

#include "utils.hpp"
#include "Exceptions.hpp"

template<size_t size>
dc::MatrixReader<size>::MatrixReader() : matrix{{0}} {

}

template<size_t size>
dc::MatrixReader<size>::~MatrixReader() {

}

template<size_t size>
bool dc::MatrixReader<size>::read(const std::string &fileName) {
    const std::string *data = nullptr;

    try {
        data = util::readStringFromFile(fileName);
    } catch (Exceptions::FileReadException const& e) {
        delete data;
        std::cerr << "[MatrixReader] " << e.getMessage() << std::endl;
        return false;
    }

    std::string line, item;
    std::stringstream ss(*data);
    bool exception = false;

    for (size_t file_row = 0; std::getline(ss, line) && !exception; ++file_row) {
        if (file_row >= size) {
            std::cerr << "[MatrixReader] Too many rows in matrix! Expected "
                      << size << " but got " << file_row << "!" << std::endl;
            exception = true;
            break;
        }

        std::trim(line);
        std::strReplaceConsecutive(line, ' ');

        std::stringstream iss(line);

        for (size_t file_col = 0; std::getline(iss, item, ' '); ++file_col) {
            if (file_col >= size) {
                std::cerr << "[MatrixReader] Too many cols in matrix! Expected "
                          << size << " but got " << file_col << "!" << std::endl;
                exception = true;
                break;
            }

            try {
                this->matrix[file_row][file_col] = util::lexical_cast<uint16_t>(item.c_str());
                //printf("Var => %s \tcast = %d\n", item.c_str(), this->matrix[file_row][file_col]);
            } catch (Exceptions::CastingException const& e) {
                exception = true;
                std::cerr << "[MatrixReader] " << e.getMessage() << std::endl;
                break;
            }
        }
    }

    delete data;

    return !exception;
}

template<size_t size>
const std::string dc::MatrixReader<size>::toString() const {
    size_t row, col;
    std::ostringstream oss;

    for (row = 0; row < size; row++) {
        for (col = 0; col < size; col++) {
            oss << std::setw(4) << this->matrix[row][col];
        }
        oss << std::endl;
    }

    return oss.str();
}

template class dc::MatrixReader<dc::DefaultMatrixSize>;
