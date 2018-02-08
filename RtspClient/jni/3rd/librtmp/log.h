/*
 *  Copyright (C) 2008-2009 Andrej Stepanchuk
 *  Copyright (C) 2009-2010 Howard Chu
 *
 *  This file is part of librtmp.
 *
 *  librtmp is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation; either version 2.1,
 *  or (at your option) any later version.
 *
 *  librtmp is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with librtmp see the file COPYING.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 *  http://www.gnu.org/copyleft/lgpl.html
 */

#ifndef __RTMP_LOG_H__
#define __RTMP_LOG_H__

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
/* Enable this to get full debugging output */
/* #define _DEBUG */

typedef enum
{  
  RTMP_LOGTRACESTR = -1, RTMP_LOGTRACE, RTMP_LOGDEBUG, RTMP_LOGINFO, RTMP_LOGWARNING, RTMP_LOGERROR
} RTMP_LogLevel;

#ifdef __GNUC__
void RTMP_Log(int level, const char *format, ...) __attribute__ ((__format__ (__printf__, 2, 3)));
#else
void RTMP_Log(int level, const char *format, ...);
#endif
void RTMP_LogHex(int level, const uint8_t *data, unsigned long len);
void RTMP_LogHexString(int level, const uint8_t *data, unsigned long len);

typedef void (RTMP_LogCallback)(int level, const char *fmt, va_list);
void RTMP_LogSetCallback(int level, RTMP_LogCallback *cb);

#ifdef __cplusplus
}
#endif

#endif
