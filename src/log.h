#ifndef __LIME_LOG_H__
#define __LIME_LOG_H__

//this is test version
#include <stdio.h>
#include <string.h>

#define LIME_LOG_ERROR 0
#define LIME_LOG_WARIN 0xf
#define LIME_LOG_INFO 0xff

#define lime_error(fmt, ...) lime_log_printf(__FILE__, __LINE__, LIME_LOG_ERROR, fmt, __VA_ARGS__)
#define lime_warin(fmt, ...) lime_log_printf(__FILE__, __LINE__, LIME_LOG_WARIN, fmt, __VA_ARGS__)
#define lime_info(fmt, ...) lime_log_printf(__FILE__, __LINE__, LIME_LOG_INFO, fmt, __VA_ARGS__)

void lime_log_printf(char *file, int line, int level, char *fmt, ...);

#endif
