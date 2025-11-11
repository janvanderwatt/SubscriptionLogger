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
        Serial.write((const uint8_t*)buffer, len);
    }
}

// --------------------------------------------------------------------------------------------------------
// Log text if the current golbabl logging level is low enough
// --------------------------------------------------------------------------------------------------------
void SubscriptionLogger::logText(uint8_t level, const char *format, ...) {
    if (level < this->logLevel) return;

    char logger_buffer[64];
    const size_t buf_sz = sizeof(logger_buffer);

    va_list ap;
    va_start(ap, format);

    // determine required length
    va_list ap_copy;
    va_copy(ap_copy, ap);
    int needed = vsnprintf(NULL, 0, format, ap_copy);
    va_end(ap_copy);
    if (needed < 0) { va_end(ap); return; }

    char *out = logger_buffer;
    bool allocated = false;
    size_t out_sz = buf_sz;

    if ((size_t)needed + 1 > buf_sz) {
        out_sz = (size_t)needed + 1;
        out = (char *)malloc(out_sz);
        if (!out) { va_end(ap); return; }
        allocated = true;
    }

    // actually format into the chosen buffer, pass the real buffer size
    int written = vsnprintf(out, out_sz, format, ap);
    va_end(ap);
    if (written < 0) {
        if (allocated) free(out);
        return;
    }

    // write exact number of bytes (exclude terminating NUL)
    writeToStreams(level, out, written);

    if (allocated) free(out);
}

void SubscriptionLogger::logDetails(uint8_t level, const char *file, int line, const char *format, ...) {
    if (level < this->logLevel) return;

    const char *level_str = "VDIWE";
    const char *format_prefix = "[%6d][%c][%s:%d]";
    char prefix_buf[64];

    // Get prefix length (snprintf returns required length excluding NUL)
    int prefix_len = snprintf(prefix_buf, sizeof(prefix_buf), format_prefix, millis(), level_str[level], file, line);
    if (prefix_len < 0) return;

    // Determine formatted message length using a copy of the va_list
    va_list ap;
    va_start(ap, format);
    va_list ap_copy;
    va_copy(ap_copy, ap);
    int msg_len = vsnprintf(NULL, 0, format, ap_copy);
    va_end(ap_copy);
    if (msg_len < 0) { va_end(ap); return; }

    // Allocate total buffer (prefix + message + NUL)
    int total_len = prefix_len + msg_len;
    char *out = (char *)malloc((size_t)total_len + 1);
    if (!out) { va_end(ap); return; }

    // Write prefix: if the temporary prefix_buf was truncated, reprint into out
    if (prefix_len < (int)sizeof(prefix_buf)) {
        memcpy(out, prefix_buf, prefix_len);
    } else {
        // prefix was truncated in prefix_buf, re-snprintf fully into out
        int r = snprintf(out, prefix_len + 1, format_prefix, millis(), level_str[level], file, line);
        if (r < 0) { free(out); va_end(ap); return; }
    }

    // Write formatted message (provide msg_len+1 as buffer size to include terminating NUL)
    int written = vsnprintf(out + prefix_len, (size_t)msg_len + 1, format, ap);
    va_end(ap);
    if (written < 0) { free(out); return; }

    // Send exactly prefix_len + msg_len bytes (no extra NUL)
    writeToStreams(level, out, prefix_len + msg_len);
    free(out);
}
