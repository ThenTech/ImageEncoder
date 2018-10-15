#include "main.hpp"
#include "Logger.hpp"
#include "Exceptions.hpp"

#include <iostream>
#include <iomanip>
#include <ctime>
#include "utils.hpp"

/**
 *  @brief  The shared Logger instance;
 */
util::Logger util::Logger::instance;

/**
 *  Synbols for printing an image to the console.
 */
// Using "█" is logged, but only std::string_format("%c", 219) is printed to console...
const std::string util::Logger::FILL  = std::string_format("%c", 219); // std::string_format("%c", 219)  char 219  █
const std::string util::Logger::EMPTY = " ";

/**
 *  @brief  Create the Logger instance from a filename.
 *          If the file does not exist, it will be created, text will be appended otherwise.
 *
 *          Disable logging if fileName is an empty string or on error.
 *
 *          Writes version and author info to the log on start.
 *
 *  @param  fileName
 *      The log file to use.
 */
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

            util::Logger::WriteLn("Simplified JPEG " ENC_TXT DEC_TXT " by " AUTHOR " v" VERSION "\n", false);
            return;
        } catch (std::exception const& e) {
            std::cerr << "[Logger] " << e.what() << std::endl;
        }
    }

    util::Logger::instance.enabled = false;
}

/**
 *  @brief  Detroy the Logging instance by closing the output file.
 */
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

/**
 *  @brief  Write the text <text> to the console and the log file.
 *
 *  @param  text
 *      The text to write.
 *  @param  timestamp
 *      Whether to include a timestamp (in the file only).
 */
void util::Logger::Write(const std::string &text, bool timestamp) {
    if (util::Logger::instance.enabled) {
        try {
            std::cout << text;

            if (timestamp) {
                auto t  = std::time(nullptr);
                auto tm = std::localtime(&t);
                util::Logger::instance.log_file << "[" << std::put_time(tm, "%Y-%m-%d %H:%M:%S")
                                                << "] ";
            }

            util::Logger::instance.log_file << text;
        } catch (std::exception const& e) {
            std::cerr << "[Logger] " << e.what() << std::endl;
            util::Logger::Destroy();
        }
    }
}

/**
 *  @brief  Write the text <text> to the console and the log file and append a new line.
 *
 *  @param  text
 *      The text to write.
 *  @param  timestamp
 *      Whether to include a timestamp (in the file only).
 */
void util::Logger::WriteLn(const std::string &text, bool timestamp) {
    util::Logger::Write(text + "\n", timestamp);
}
