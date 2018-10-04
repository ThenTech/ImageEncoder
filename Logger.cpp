#include "main.hpp"
#include "Logger.hpp"
#include "Exceptions.hpp"

#include <iostream>
#include <iomanip>
#include <ctime>


util::Logger util::Logger::instance;


void util::Logger::Create(const std::string &fileName) {
    if (fileName.length() > 0) {
        try {
            util::Logger::instance.log_file.open(fileName, ios_base::app | ios_base::out);
            util::Logger::instance.enabled = true;

            #ifdef ENCODER
                #define ENC_TXT "Encoder"
            #else
                #define ENC_TXT
            #endif
            #ifdef DECODER
                #ifdef ENCODER
                    #define DEC_TXT "/Decoder"
                #else
                    #define DEC_TXT "Decoder"
                #endif
            #else
                #define DEC_TXT
            #endif

            util::Logger::Write("JPEG " ENC_TXT DEC_TXT " by " AUTHOR " v" VERSION "\n", false);
        } catch (std::exception const& e) {
            std::cerr << "[Logger] " << e.what() << std::endl;
            util::Logger::instance.enabled = false;
        }
    }
}

void util::Logger::Destroy() {
    if (util::Logger::instance.enabled) {
        util::Logger::instance.log_file
                << std::endl
                << "----------------------------------------------------------------------"
                << std::endl << std::endl;
        util::Logger::instance.log_file.close();
        util::Logger::instance.enabled = false;
    }
}

void util::Logger::Write(const std::string &text, bool timestamp) {
    if (util::Logger::instance.enabled) {
        try {
            std::cout << text << std::endl;

            if (timestamp) {
                auto t = std::time(nullptr);
                auto tm = *std::localtime(&t);
                util::Logger::instance.log_file << std::put_time(&tm, "%Y-%m-%d %H:%M:%S")
                                                << " ";
            }

            util::Logger::instance.log_file << text << std::endl;
        } catch (std::exception const& e) {
            std::cerr << "[Logger] " << e.what() << std::endl;
            util::Logger::Destroy();
        }
    }
}
