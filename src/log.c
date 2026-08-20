#include <stdio.h>
#include <stdarg.h>
#include "log.h"

static LogLevel current_log_level = LOG_LEVEL_INFO;

static const char *level_name(LogLevel level) {
	switch(level) {
		case LOG_LEVEL_ERROR: return "ERROR";
		case LOG_LEVEL_WARN: return "WARN";
		case LOG_LEVEL_INFO: return "INFO";
		case LOG_LEVEL_DEBUG: return "DEBUG";
		default: return "UNKNOWN";
	}
}

void log_set_level(LogLevel level) {
	current_log_level = level;
}

void log_message(LogLevel level, const char *file, int line, const char *fmt, ...) {
	if(level > current_log_level) {
		return;
	}

	fprintf(stderr, "[%s] %s:%d: ", level_name(level), file, line);

	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
}

