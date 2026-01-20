#ifndef LOGGER_H
#define LOGGER_H

#include <thread>
#include <string>
#include <sstream>
#include <iostream>
using std::cout, std::endl, std::string, std::stringstream, std::ostringstream;

struct Colors
{
    static constexpr const char* NONE = "";
    static constexpr const char* RESET = "\033[0m";
    static constexpr const char* BOLD = "\033[1m";
    static constexpr const char* DIM = "\033[2m";
    static constexpr const char* ITALIC = "\033[3m";
    static constexpr const char* UNDERLINE = "\033[4m";
    static constexpr const char* TWINKLE = "\033[5m";


    static constexpr const char* BLACK = "\033[30m";
    static constexpr const char* RED = "\033[31m";
    static constexpr const char* GREEN = "\033[32m";
    static constexpr const char* YELLOW = "\033[33m";
    static constexpr const char* BLUE = "\033[34m";
    static constexpr const char* MAGENTA = "\033[35m";
    static constexpr const char* CYAN = "\033[36m";
    static constexpr const char* WHITE = "\033[37m";

    static constexpr const char* BG_BLACK = "\033[40m";
    static constexpr const char* BG_RED = "\033[41m";
    static constexpr const char* BG_GREEN = "\033[42m";
    static constexpr const char* BG_YELLOW = "\033[43m";
    static constexpr const char* BG_BLUE = "\033[44m";
    static constexpr const char* BG_MAGENTA = "\033[45m";
    static constexpr const char* BG_CYAN = "\033[46m";
    static constexpr const char* BG_WHITE = "\033[47m";
};

class Logger
{
private:
    Logger() {}
    Logger(const Logger&) = delete;
    void operator=(const Logger&) = delete;

    enum class TYPE
    {
        INFO,
        SUCCESS,
        WARNING,
        ERROR,
        DEBUG
    };

    template<typename... Args>
    void log(unsigned int level, Args... args)
    {
        if (!enabled || level > verbose)
            return;
        ostringstream ss;
        ((ss << args << " "), ...);
        ss << endl;
        cout << ss.str();
    }

    string translateTypeToString(TYPE type)
    {
        switch (type)
        {
            case TYPE::INFO:    return " INFO";
            case TYPE::SUCCESS: return " SUCC";
            case TYPE::WARNING: return " WARN";
            case TYPE::ERROR:   return "ERROR";
            case TYPE::DEBUG:   return "DEBUG";
            default:            return "UNKNOWN";
        }
    }

    constexpr const char* translateTypeToColor(TYPE type)
    {
        switch (type)
        {
            case TYPE::INFO:    return wrapcolor(Colors::NONE);
            case TYPE::SUCCESS: return wrapcolor(Colors::GREEN);
            case TYPE::WARNING: return wrapcolor(Colors::YELLOW);
            case TYPE::ERROR:   return wrapcolor(Colors::RED);
            case TYPE::DEBUG:   return wrapcolor(Colors::CYAN);
            default:            return wrapcolor(Colors::NONE);
        }
    }

    constexpr const char* wrapcolor(const char* color)
    {
        return enableColors ? color : Colors::NONE;
    }

    string getLineHeading(TYPE type)
    {
        // [DD/MM/YY HH:MM | T-ID | TYPE]
        stringstream heading ("");
        heading << wrapcolor(Colors::WHITE) << "[" << wrapcolor(Colors::RESET);

        if (enableTimestamp)
        {
            struct tm* timeinfo;
            time_t rawtime;
            time(&rawtime);
            timeinfo = localtime(&rawtime);

            heading << wrapcolor(Colors::DIM) << wrapcolor(Colors::CYAN)
                    << (timeinfo->tm_mday < 10 ? "0" : "") << timeinfo->tm_mday << "/"
                    << (timeinfo->tm_mon + 1 < 10 ? "0" : "") << (timeinfo->tm_mon + 1) << "/"
                    << (timeinfo->tm_year + 1900) % 100 << " "
                    << (timeinfo->tm_hour < 10 ? "0" : "") << timeinfo->tm_hour << ":"
                    << (timeinfo->tm_min < 10 ? "0" : "") << timeinfo->tm_min
                    << wrapcolor(Colors::RESET) << wrapcolor(Colors::WHITE) << " | ";
        }

        if (enableThreadID)
        {
            heading << wrapcolor(Colors::MAGENTA) << "T-" << std::this_thread::get_id() << wrapcolor(Colors::WHITE) << " | " << wrapcolor(Colors::RESET);
        }

        heading << translateTypeToColor(type) << wrapcolor(Colors::BOLD) << translateTypeToString(type) << wrapcolor(Colors::RESET);
        heading << wrapcolor(Colors::WHITE) << "]" << wrapcolor(Colors::RESET) << " ";

        return heading.str();
    }

public:
    static Logger& getInstance()
    {
        static Logger instance;
        return instance;
    }

    bool enabled              = true;
    bool enableTimestamp      = false;
    bool enableColors         = true;
    bool enableThreadID       = false;
    unsigned int verbose      = 1;

    template<typename... Args>
    void logi(unsigned int level, Args... args)
    {
        log(level, getLineHeading(TYPE::INFO), translateTypeToColor(TYPE::INFO), args..., wrapcolor(Colors::RESET));
    }

    template<typename... Args>
    void logs(unsigned int level, Args... args)
    {
        log(level, getLineHeading(TYPE::SUCCESS), translateTypeToColor(TYPE::SUCCESS), args..., wrapcolor(Colors::RESET));
    }

    template<typename... Args>
    void logw(unsigned int level, Args... args)
    {
        log(level, getLineHeading(TYPE::WARNING), translateTypeToColor(TYPE::WARNING), args..., wrapcolor(Colors::RESET));
    }

    template<typename... Args>
    void loge(unsigned int level, Args... args)
    {
        log(level, getLineHeading(TYPE::ERROR), translateTypeToColor(TYPE::ERROR), args..., wrapcolor(Colors::RESET));
    }
};

#endif