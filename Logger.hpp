#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <fstream>

namespace util {
    /**
     *  @brief  A class with static methods for logging to std::cout and a log file.
     *          Create the Loging instance with Logger::Create(),
     *          and destroy it by calling Logger::Destroy()
     */
    class Logger {
        private:
            bool enabled;
            std::ofstream log_file;

            static Logger instance;
        public:
            Logger() : enabled(false) {}
            Logger(Logger const&)         = delete;
            void operator=(Logger const&) = delete;

            static void Create(const std::string &fileName);
            static void Destroy();
            static void Write(const std::string &text, bool timestamp=true);
            static void WriteLn(const std::string &text, bool timestamp=true);
            static void WriteProgress(const size_t& iteration, const size_t& total);

            static const std::string FILL;
            static const std::string EMPTY;
    };
}

#endif // LOGGER_HPP
