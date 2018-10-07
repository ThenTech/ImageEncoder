#include "BitStream.hpp"

namespace util {
    const uint8_t bitmasks[] = {0, 128, 192, 224, 240, 248, 252, 254};

    BitStreamReader::BitStreamReader(uint8_t *b, size_t s) : BitStream(b, s, 0, false) {}

    BitStreamReader::~BitStreamReader() {}

    void BitStreamReader::flush() {
        if (this->position % 8 != 0) {
            this->position += 8 - (this->position % 8);
        }
    }

    uint8_t BitStreamReader::get_bit() {
        const size_t bits_taken = this->position % 8;
        uint8_t value = this->buffer[this->position / 8];
        value &= (1 << (7 - bits_taken));
        this->position++;
        return value != 0;
    }

    uint32_t BitStreamReader::get(size_t l) {
        uint32_t value = 0;

        for (size_t i = 0; i < l; i++) {
            const uint32_t v = this->get_bit();
            value |= v << (l - i - 1);
            //printf("%s %d pos %d i %d v %d value %x\n", __func__, __LINE__, position, i, v, value);
        }

        return value;
    }

    ////////////////////////////////////////////////////////////////////////////////////

    BitStreamWriter::BitStreamWriter(size_t s)
        : BitStream(util::allocArray<uint8_t>(s), s, 0, true)
    {
        //printf("Allocated buffer of size: %d\n", s);
    }

    BitStreamWriter::BitStreamWriter(uint8_t *b, size_t s) : BitStream(b, s, 0, false) {}

    BitStreamWriter::~BitStreamWriter() {}

    void BitStreamWriter::flush() {
        if (this->position % 8 != 0) {
            this->buffer[this->position / 8] &= bitmasks[this->position % 8];
            this->position += 8 - (this->position % 8);
        }
    }

    void BitStreamWriter::put_bit(int8_t value) {
        const size_t bits_taken = this->position % 8;

        if (value) {
            this->buffer[this->position / 8] |= 1 << (7 - bits_taken);
        } else {
            this->buffer[this->position / 8] &= ~(1 << (7 - bits_taken));
        }

        this->position++;
    }

    void BitStreamWriter::put(size_t length, uint32_t value) {
        for (size_t p = 0; p < length; p++) {
            put_bit(1 & (value >> (length - 1 - p)));
        }
    }

    void write(FILE *f, const BitStreamWriter &b) {
        const size_t position = b.get_position();
        const uint8_t *buffer = b.get_buffer();

        fwrite(buffer, 1, position / 8, f);

        if (position % 8 != 0) {
            fwrite(buffer + position / 8, 1, 1, f);
        }
    }

    void write(std::ofstream &fs, const BitStreamWriter &b) {
        const size_t position = b.get_position();
        const uint8_t *buffer = b.get_buffer();

        fs.write(reinterpret_cast<const char*>(buffer), position / 8);

        if (position % 8 != 0) {
            fs.write(reinterpret_cast<const char*>(buffer + position / 8), 1);
        }
    }

}

