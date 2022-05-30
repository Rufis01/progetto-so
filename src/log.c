#include <stdio.h>
#include <stdarg.h>

#include "log.h"

static FILE *fd;

void log_init(const char *filename)
{
	fd = fopen(filename, "w");
}

void log_printf(log_level level, const char *format, ...)
{
	va_list args;
	va_start (args, format);
	vfprintf(fd, format, args);
	va_end(args);
}

void log_fini(void)
{
	fclose(fd);
}