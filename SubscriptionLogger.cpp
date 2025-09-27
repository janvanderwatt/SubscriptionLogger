#include "SubscriptionLogger.h"

SubscriptionLogger::SubscriptionLogger() {
    // Constructor implementation
}

void SubscriptionLogger::setLogLevel(uint8_t level) {
    logLevel = level > LOGGER_LEVEL_NONE ? LOGGER_LEVEL_NONE : level;
}

bool SubscriptionLogger::registerStream(Print &stream, uint8_t logLevel) {
    // Initialize the logger
    StreamInfo *info = new StreamInfo;
    info->stream = &stream;
    info->logLevel = logLevel;
    info->next = nullptr;
    if (firstStream == nullptr) {
        firstStream = lastStream = info;
    } else {
        lastStream->next = info;
        lastStream = info;
    }
    return true;
}

void SubscriptionLogger::logText(uint8_t level, const char *file, int line, const char *format, ...) {
    char logger_buffer[128];

    if (level >= this->logLevel) {
        char *temp = logger_buffer;
        va_list arg;
        va_list copy;
        va_start(arg, format);
        va_copy(copy, arg);
        int len = vsnprintf(temp, sizeof(logger_buffer), format, copy);
        va_end(copy);
        if (len < 0) {
            va_end(arg);
            return;
        };
        if (len >= sizeof(logger_buffer)) {
            temp = (char *)malloc(len + 1);
            if (temp == NULL) {
                va_end(arg);
                return;
            }
            len = vsnprintf(temp, len + 2, format, arg);
        }
        va_end(arg);

        if (temp[len - 1] == '\n') {
            if ((len == 1) || (len > 1 && temp[len - 2] != '\r')) {
                temp[len - 1] = '\r';
                temp[len] = '\n';
                temp[len + 1] = 0;
                len++;
            }
        }

        // Write the log message to all registered streams
        if (firstStream != nullptr) {
            StreamInfo *s = firstStream;
            do {
                // Serial.print("XYZ");
                s->stream->write((uint8_t *)temp, len);
                s = s->next;
            } while (s != nullptr);
        } else {
            Serial.println("No streams registered for logging!");
        }

        if (temp != logger_buffer) {
            free(temp);
        }
    }
}

void SubscriptionLogger::logDetails(uint8_t level, const char *file, int line, const char *format, ...) {
    // Serial.printf("LOGDETAILS: level=%d, logLevel=%d, file=%s, line=%d, format=%s\n", level, this->logLevel, file, line, format);
    if (level >= this->logLevel) {
        // Prepare the log prefix
        char format_buffer[128];
        const char *level_str = "VDIWE", *format_prefix = "[%6d][%c][%s:%d]", *fmt_concat = "%s%s";

        char *temp_format = format_buffer;
        int len = snprintf(temp_format, sizeof(format_buffer), fmt_concat, format_prefix, format);

        if (len < 0) {
            return;
        };
        if (len >= sizeof(format_buffer)) {
            temp_format = (char *)malloc(len + 1);
            if (temp_format == NULL) {
                return;
            }
            len = snprintf(temp_format, len + 2, fmt_concat, format_prefix, format);
        }


        va_list arg;
        va_start(arg, format);
        // Serial.print("DEF");
        logText(level, file, line, temp_format, millis(), level_str[level], file, line, arg);
        va_end(arg);
    }
}
