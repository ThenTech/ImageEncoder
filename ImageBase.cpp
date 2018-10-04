#include "ImageBase.hpp"
#include "utils.hpp"
#include "Logger.hpp"

#include <cstring>

dc::ImageBase::ImageBase(const std::string &source_file) {
    try {
        this->raw = util::readBinaryFile(source_file);
    } catch (Exceptions::FileReadException const& e) {
        util::Logger::Write(e.getMessage());
        exit(-1);
    }

    this->reader = new util::BitStreamReader(this->raw->data(), this->raw->size());
}

dc::ImageBase::~ImageBase(void) {
    delete this->raw;
    delete this->reader;
}

////////////////////////////////////////////////////////////////////////////////////

dc::ImageProcessor::ImageProcessor(const std::string &source_file,
                                   const std::string &dest_file)
    : ImageBase(source_file),
      dest_file(dest_file),
      writer(new util::BitStreamWriter(this->raw->size()))
{
    // Empty
}

dc::ImageProcessor::~ImageProcessor(void) {
    delete this->writer;
}

bool dc::ImageProcessor::process(void) {
    util::Logger::Write("[ImageProcessor] Processing image...");

    memcpy(this->writer->get_buffer(), this->reader->get_buffer(), this->reader->get_size());
    this->writer->set_position(1);

    return true;
}

void dc::ImageProcessor::saveResult(void) const {
    #if 1
        // This will only write whole bytes; if pos % 8 != 0, last byte is skipped
        util::writeBinaryFile(this->dest_file,
                              this->writer->get_buffer(),
                              this->writer->get_size());
    #else
        // Will take last byte into account
        std::ofstream file(this->dest_file, std::ofstream::binary);
        util::write(file, *this->writer);
    #endif

    util::Logger::Write("[ImageProcessor] Saved file at: " + this->dest_file);
}
