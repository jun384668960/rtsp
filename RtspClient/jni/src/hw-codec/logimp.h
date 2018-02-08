#ifndef __LOGIMP_H__
#define __LOGIMP_H__

#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <jni.h>
#include <android/log.h>

#ifndef _DEBUG //写日志到logcat/file
#define LOCAL_LOG_IMPLEMENT(p, n)

#define LOGT_print(n, t, ...) 
#define LOGD_print(n, t, ...) 
#define LOGI_print(n, t, ...) 
#define LOGW_print(n, t, ...)
#define LOGE_print(n, t, ...) 

#define CLASS_LOG_DECLARE(n)
#define CLASS_LOG_IMPLEMENT(c, n)

#define THIS_LOGT_print(t, ...)
#define THIS_LOGD_print(t, ...)
#define THIS_LOGI_print(t, ...)
#define THIS_LOGW_print(t, ...)
#define THIS_LOGE_print(t, ...)

#define COST_STAT_DECLARE(n)

#else

#ifndef _LOGFILE
#define __log_write __android_log_print
#else
#define __log_write __file_log_write
extern void __file_log_write(int level, const char *n, const char *t, ...);
#endif

#define LOCAL_LOG_IMPLEMENT(p, n) static const char *p = n

#ifndef _RELEASE
#define LOGT_print(n, t, ...) __log_write(ANDROID_LOG_VERBOSE, n, t, ##__VA_ARGS__)
#define LOGD_print(n, t, ...) __log_write(ANDROID_LOG_DEBUG,   n, t, ##__VA_ARGS__)
#else
#define LOGT_print(n, t, ...)
#define LOGD_print(n, t, ...)
#endif
#define LOGI_print(n, t, ...) __log_write(ANDROID_LOG_INFO,    n, t, ##__VA_ARGS__)
#define LOGW_print(n, t, ...) __log_write(ANDROID_LOG_WARN,    n, t, ##__VA_ARGS__)
#define LOGE_print(n, t, ...) __log_write(ANDROID_LOG_ERROR,   n, t, ##__VA_ARGS__)

#define CLASS_LOG_DECLARE(n) 	  static const char *s_logger
#define CLASS_LOG_IMPLEMENT(c, n) const char *c::s_logger = n

#ifndef _RELEASE
#define THIS_LOGT_print(t, ...) __log_write(ANDROID_LOG_VERBOSE, s_logger, "%p " t, this, ##__VA_ARGS__)
#define THIS_LOGD_print(t, ...) __log_write(ANDROID_LOG_DEBUG,   s_logger, "%p " t, this, ##__VA_ARGS__)
#else
#define THIS_LOGT_print(t, ...)
#define THIS_LOGD_print(t, ...)
#endif
#define THIS_LOGI_print(t, ...) __log_write(ANDROID_LOG_INFO,    s_logger, "%p " t, this, ##__VA_ARGS__)
#define THIS_LOGW_print(t, ...) __log_write(ANDROID_LOG_WARN,    s_logger, "%p " t, this, ##__VA_ARGS__)
#define THIS_LOGE_print(t, ...) __log_write(ANDROID_LOG_ERROR,   s_logger, "%p " t, this, ##__VA_ARGS__)

#define COST_STAT_DECLARE(n) int64_t n = GetTickCount()

#endif

extern int64_t GetTickCount(); //us

#endif /* __LOGIMP_H__ */
