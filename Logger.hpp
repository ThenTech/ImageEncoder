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
            bool paused;
            std::ofstream log_file;

            static Logger instance;

            inline bool canLog(void) { return this->enabled && !this->paused; }
        public:
            Logger() : enabled(false), paused(false) {}
            Logger(Logger const&)         = delete;
            void operator=(Logger const&) = delete;

            static void Create(const std::string &fileName);
            static void Destroy();
            static void Write(const std::string &text, bool timestamp=true);
            static void WriteLn(const std::string &text, bool timestamp=true);
            static void WriteProgress(const size_t& iteration, const size_t& total);

            static void Pause(void);
            static void Resume(void);

            static const std::string FILL;
            static const std::string EMPTY;
    };
}

#endif // LOGGER_HPP
