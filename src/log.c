#include <stdio.h>
#include <stdarg.h>

#include "log.h"

static FILE *fd = 0;

void log_init(const char *filename)
{
	fd = fopen(filename, "w");
	//fd = stdout;
}

void log_printf(log_level level, const char *format, ...)
{
	if(fd == 0) fd = stdout;
	va_list args;
	va_start (args, format);
	vfprintf(fd, format, args);
	va_end(args);
	
#ifdef DEBUG
	fflush(fd);
#endif
}

void log_fini(void)
{
	fclose(fd);
}