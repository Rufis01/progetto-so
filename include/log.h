#ifndef LOG_H
#define LOG_H

#define LOGD(format, ...) (log_printf(LOG_DEBUG, "[DEBUG] " __FILE__ "@%s:%d " format, __func__, __LINE__ __VA_OPT__(,)  __VA_ARGS__))
#define LOGI(format, ...) (log_printf(LOG_INFO, "[INFO] " __FILE__ "@%s:%d " format, __func__, __LINE__ __VA_OPT__(,)  __VA_ARGS__))
#define LOGW(format, ...) (log_printf(LOG_WARN, "[WARNING] " __FILE__ "@%s:%d " format, __func__, __LINE__ __VA_OPT__(,)  __VA_ARGS__))
#define LOGE(format, ...) (log_printf(LOG_ERROR, "[ERROR] " __FILE__ "@%s:%d " format, __func__, __LINE__ __VA_OPT__(,)  __VA_ARGS__))
#define LOGF(format, ...) (log_printf(LOG_FATAL, "[FATAL] " __FILE__ "@%s:%d " format, __func__, __LINE__ __VA_OPT__(,)  __VA_ARGS__))


typedef enum
{
	LOG_NONE,
	LOG_FATAL,
	LOG_ERROR,
	LOG_WARN,
	LOG_INFO,
	LOG_DEBUG,
	LOG_INVALID_LEVEL
} log_level;

void log_setLogLevel(log_level level);
void log_init(const char *filepath);
void log_getCurrentTimeString(char out[32]);
void log_printf(log_level level, const char *format, ...);
void log_rename(const char *oldname, const char *newname);
void log_fini(void);

#endif
