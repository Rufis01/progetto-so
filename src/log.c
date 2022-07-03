#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "log.h"

static FILE *_fd = 0;
static log_level _level = LOG_INFO;

static char* log_levelString(log_level level);
static log_level log_levelValue(char *level);

void log_setLogLevel(log_level level)
{
	setenv("SO_LOG_LEVEL", log_levelString(level), 1);
}

void log_init(const char *filename)
{
	_level = log_levelValue(getenv("SO_LOG_LEVEL"));
	_fd = filename ? fopen(filename, "w") : stdout;
}

void log_getCurrentTimeString(char out[32])
{
	struct tm* to;
	time_t t;
	t = time(NULL);
	to = localtime(&t);
	LOGD("Lunghezza stringa data: %d\n", strftime(out, 32, "%Y-%m-%d %H:%M:%S", to));
}

void log_printf(log_level level, const char *format, ...)
{
	if(level > _level) return;
	if(_fd == 0) _fd = stdout;
	va_list args;
	va_start (args, format);
	vfprintf(_fd, format, args);
	va_end(args);
	
#ifdef DEBUG
	fflush(_fd);
#endif
}

void log_rename(const char *oldname, const char *newname)
{
	fflush(_fd);
	fclose(_fd);
	rename(oldname, newname);
	_fd = fopen(newname, "a");
}

void log_fini(void)
{
	fflush(_fd);
	fclose(_fd);
}

static inline char* log_levelString(log_level level)
{
	switch (level)
	{
		case LOG_INFO:
			return "INFO";
		case LOG_WARN:
			return "WARN";
		case LOG_ERROR:
			return "ERROR";
		case LOG_DEBUG:
			return "DEBUG";
		default:
			return "INVALID";
	}
}

static inline log_level log_levelValue(char *level)
{
	if(!level)
		return LOG_INVALID_LEVEL;
	if(strcmp(level, "INFO") == 0)
		return LOG_INFO;
	if(strcmp(level, "WARN") == 0)
		return LOG_WARN;
	if(strcmp(level, "ERROR") == 0)
		return LOG_ERROR;
	if(strcmp(level, "DEBUG") == 0)
		return LOG_DEBUG;
	return LOG_INVALID_LEVEL;
}
