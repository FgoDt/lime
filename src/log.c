#include "log.h"
#include <stdarg.h>
#include <time.h>

static FILE *lime_log_file = NULL;
void lime_log_printf(char *file, int line, int level, char *fmt, ...)
{

	time_t t;
	time(&t);
	struct tm *time_info = localtime(&t);

	char log_fmt[1024] = {0};
	char log[1024] = {0};
	switch (level)
	{
	case LIME_LOG_ERROR:
		sprintf(log_fmt, "%d-%d-%dT%02d:%02d:%02d [ERROR] file:%s:%d %s\n",
				time_info->tm_year + 1900, time_info->tm_mon + 1, time_info->tm_mday,
				time_info->tm_hour, time_info->tm_min, time_info->tm_sec,
				file, line, fmt);
		break;
	case LIME_LOG_WARIN:
		sprintf(log_fmt, "%d-%d-%dT%02d:%02d:%02d [WARING] file:%s:%d %s\n",
				time_info->tm_year + 1900, time_info->tm_mon + 1, time_info->tm_mday,
				time_info->tm_hour, time_info->tm_min, time_info->tm_sec,
				file, line, fmt);
		break;
	case LIME_LOG_INFO:
		sprintf(log_fmt, "%d-%d-%dT%02d:%02d:%02d [INFO] file:%s:%d %s\n",
				time_info->tm_year + 1900, time_info->tm_mon + 1, time_info->tm_mday,
				time_info->tm_hour, time_info->tm_min, time_info->tm_sec,
				file, line, fmt);
		break;
	default:
		sprintf(log_fmt, "%d-%d-%dT%02d:%02d:%02d [UNKNOW] file:%s:%d %s\n",
				time_info->tm_year + 1900, time_info->tm_mon + 1, time_info->tm_mday,
				time_info->tm_hour, time_info->tm_min, time_info->tm_sec,
				file, line, fmt);
		break;
	}
	va_list args;
	va_start(args, fmt);
	vsprintf(log, log_fmt, args);
	va_end(args);
	if (lime_log_file == NULL)
	{
		lime_log_file = fopen("lime.log", "ab");
	}
	if (lime_log_file)
	{
		fwrite(log, 1, strlen(log), lime_log_file);
		fflush(lime_log_file);
	}
	printf("%s",log);
}
