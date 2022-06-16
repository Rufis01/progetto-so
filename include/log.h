#ifndef LOG_H
#define LOG_H

#define LOGI(format, ...) (log_printf(LOG_INFO, __FILE__ "@%s " format, __func__ __VA_OPT__(,)  __VA_ARGS__))
#define LOGW(format, ...) (log_printf(LOG_WARN, __FILE__ "@%s " format, __func__ __VA_OPT__(,)  __VA_ARGS__))
#define LOGE(format, ...) (log_printf(LOG_ERROR, __FILE__ "@%s " format, __func__ __VA_OPT__(,)  __VA_ARGS__))
#define LOGD(format, ...) (log_printf(LOG_DEBUG, __FILE__ "@%s " format, __func__ __VA_OPT__(,)  __VA_ARGS__))

typedef enum
{
	LOG_INFO,
	LOG_WARN,
	LOG_ERROR,
	LOG_DEBUG,
	LOG_INVALID_LEVEL
} log_level;

void log_setLogLevel(log_level level);
void log_init(const char *filepath);
void log_printf(log_level level, const char *format, ...);
void log_rename(const char *oldname, const char *newname);
void log_fini(void);

#endif