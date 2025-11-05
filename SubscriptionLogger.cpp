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

// --------------------------------------------------------------------------------------------------------
// Write the buffer to all the registered streams with a low enough logging level
// --------------------------------------------------------------------------------------------------------
void SubscriptionLogger::writeToStreams(uint8_t level, const char *buffer, int len) {
    // Write the log message to all registered streams
    if (firstStream != nullptr) {
        StreamInfo *s = firstStream;
        do {
            if (level >= s->logLevel) {
                s->stream->write((uint8_t *)buffer, len);
            }
            s = s->next;
        } while (s != nullptr);
    } else {
        Serial.print("FALLBACK:");
        Serial.write(buffer, len);
    }
}

// --------------------------------------------------------------------------------------------------------
// Log text if the current golbabl logging level is low enough
// --------------------------------------------------------------------------------------------------------
void SubscriptionLogger::logText(uint8_t level, const char *format, ...) {
    char logger_buffer[64];

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

        writeToStreams(level, temp, len);

        if (temp != logger_buffer) {
            free(temp);
        }
    }
}

void SubscriptionLogger::logDetails(uint8_t level, const char *file, int line, const char *format, ...) {
    // Serial.printf("LOGDETAILS: level=%d, logLevel=%d, file=%s, line=%d, format=%s\n", level, this->logLevel, file, line, format);
    if (level >= this->logLevel) {
        // Prepare the log prefix
        char logger_buffer[128];
        const char *level_str = "VDIWE", *format_prefix = "[%6d][%c][%s:%d]";

        char *temp = logger_buffer;
        int len_prefix = snprintf(logger_buffer, sizeof(logger_buffer), format_prefix, millis(), level_str[level], file, line);

        if (len_prefix < 0) {
            return;
        }

        va_list arg;
        va_list copy;
        va_start(arg, format);
        va_copy(copy, arg);
        int len_log;
        if (len_prefix < sizeof(logger_buffer)) {
            // The prefix fit into the buffer. Try our luck, attempt to add the log after the prefix.
            len_log = vsnprintf(logger_buffer + len_prefix, sizeof(logger_buffer) - len_prefix, format, copy);
        } else {
            // The prefix did not fit into the buffer. Get the length of the log without writing anything in memory.
            len_log = vsnprintf(NULL, 0, format, copy);
        }
        va_end(copy);
        if (len_log < 0) {
            va_end(arg);
            return;
        };

        // Check if the prefix and the log did fit in the buffer
        int len_buffer = len_prefix + len_log;
        if (len_buffer > sizeof(logger_buffer)) {
            // No, it didn't, so allocate memory for it.
            temp = (char *)malloc(len_buffer);
            if (temp == NULL) {
                return;
            }
            if (len_prefix < sizeof(logger_buffer)) {
                // If the prefix fit into the original buffer, it's faster to just copy it
                memcpy(temp, logger_buffer, len_prefix);
            } else {
                // The prefix string was too long, so write it again
                snprintf(temp, len_buffer, format_prefix, millis(), level_str[level], file, line);
            }
            // Write the log
            vsnprintf(temp + len_prefix, len_log, format, arg);
        }
        va_end(arg);

        writeToStreams(level, temp, len_buffer);

        if (temp != logger_buffer) {
            free(temp);
        }
    }
}
