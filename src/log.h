#pragma once

typedef enum {
	LOG_LEVEL_ERROR = 0,
	LOG_LEVEL_WARN  = 1,
	LOG_LEVEL_INFO  = 2,
	LOG_LEVEL_DEBUG = 3,
} LogLevel;

// Run time settable and anything above this level is supressed
void log_set_level(LogLevel level);


// Internal - call through the macros below, not directly
// takes the severity, the source file and line, and printf
// style formatting string and  .. - a variadic function
// meaning it can accept any number of extra arguments
// matching the format specifiers in fmt.
void log_message(LogLevel level, const char *file, int line, const char *fmt, ...);

#define LOG_ERROR(...) log_message(LOG_LEVEL_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_WARN(...) log_message(LOG_LEVEL_WARN, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_INFO(...) log_message(LOG_LEVEL_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_DEBUG(...) log_message(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __VA_ARGS__)

// Logs at ERROR level, then aborts. Used for unrecoverable conditions
#define LOG_FATAL(...) do { \
	log_message(LOG_LEVEL_ERROR, __FILE__, __LINE__, __VA_ARGS__); \
	abort(); \
   } while (0)

