#pragma once

#include <Arduino.h>

#define LOGGER_LEVEL_VERBOSE 0
#define LOGGER_LEVEL_DEBUG 1
#define LOGGER_LEVEL_INFO 2
#define LOGGER_LEVEL_WARN 3
#define LOGGER_LEVEL_ERROR 4
#define LOGGER_LEVEL_NONE 5

// ----------------------------------------------------------------
// DEBUG, INFO, WARN and ERROR
// ----------------------------------------------------------------

class SubscriptionLogger
{
public:
    SubscriptionLogger();
    bool registerStream(Print &stream, uint8_t logLevel = LOGGER_LEVEL_INFO);
    void logDetails(uint8_t level, const char *file, int line, const char *format, ...);
    void logText(uint8_t level, const char *file, int line, const char *format, ...);

    void setLogLevel(uint8_t level);

private:
    // private members if needed
    struct StreamInfo
    {
        Print *stream;
        uint8_t logLevel;
        StreamInfo *next;
    };
    StreamInfo *firstStream = nullptr, *lastStream = nullptr;

    uint8_t logLevel = LOGGER_LEVEL_NONE;
};
