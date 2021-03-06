/* * 
 *  $Id: RTSP_describe.c 338 2006-04-27 16:45:52Z shawill $
 *  
 *  This file is part of Fenice
 *
 *  Fenice -- Open Media Server
 *
 *  Copyright (C) 2004 by
 *  	
 *	- Giampaolo Mancini	<giampaolo.mancini@polito.it>
 *	- Francesco Varano	<francesco.varano@polito.it>
 *	- Marco Penno		<marco.penno@polito.it>
 *	- Federico Ridolfo	<federico.ridolfo@polito.it>
 *	- Eugenio Menegatti 	<m.eu@libero.it>
 *	- Stefano Cau
 *	- Giuliano Emma
 *	- Stefano Oldrini
 * 
 *  Fenice is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Fenice is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Fenice; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *  
 * */

#include <stdio.h>
#include <string.h>

#include <fenice/rtsp.h>
#include <fenice/utils.h>
#include <fenice/prefs.h>
#include <fenice/sdp.h>
#include <fenice/fnc_log.h>

#include "../config.h"
/*
****************************************************************
*处理DESCRIBE方法
****************************************************************
*/
int RTSP_describe(RTSP_buffer * rtsp)
{
    /*DEBUG_PRINTF("processing describe method!\n");*/    /*just for debug,yanf*/
    
    int valid_url, res;
    char object[255], server[255], trash[255];
    char *p;
    unsigned short port;
    char url[255];
    media_entry media, req;
    description_format descr_format = df_SDP_format;    
    char descr[MAX_DESCR_LENGTH];

    char szDebug[128];

    /*根据收到的请求请求消息，跳过方法名，分离出URL*/
    if (!sscanf(rtsp->in_buffer, " %*s %254s ", url)) 
    {
        send_reply(400, 0, rtsp);                       /* bad request */
        return ERR_NOERROR;
    }
    
    /*验证URL */
    switch (parse_url(url, server, sizeof(server), &port, object, sizeof(object))) 
    {
        case 1: /*请求错误*/
            send_reply(400, 0, rtsp);
            return ERR_NOERROR;
            break;
            
        case -1: /*内部错误*/
            DEBUG_PRINT_ERROR("url error while parsing !")
            send_reply(500, 0, rtsp);
            return ERR_NOERROR;
            break;
            
        default:
            break;
    }
    if (strcmp(server, prefs_get_hostname()) != 0) 
    {
        /* Currently this feature is disabled. */
        /* wrong server name */
        //      send_reply(404, 0 , rtsp);  /* Not Found */
        //      return ERR_NOERROR;
    }
    
    if (strstr(object, "../")) 
    {
        /* 不允许本地路径之外的相对路径*/
        send_reply(403, 0, rtsp);   /* Forbidden */
        return ERR_NOERROR;
    }
    
    if (strstr(object, "./")) 
    {
        /*不允许相对路径，包括相对于当前的相对路径 */
        send_reply(403, 0, rtsp);    /* Forbidden */
        return ERR_NOERROR;
    }

    /*根据后缀名确定媒体格式*/
    p = strrchr(object, '.');
    valid_url = 0;
    if (p == NULL) 
    {
        send_reply(415, 0, rtsp);   /* Unsupported media type */
        return ERR_NOERROR;
    } 
    else 
    {
        valid_url = is_supported_url(p);     /*判断媒体文件格式是否可以被识别*/
    }
    
    if (!valid_url)
    {
        send_reply(415, 0, rtsp);   /* 不支持的媒体类型 */
        return ERR_NOERROR;
    }
    
    /*禁止头中有require选项*/
    if (strstr(rtsp->in_buffer, HDR_REQUIRE)) 
    {
        /*DEBUG_PRINTF("Has require header!Another describe will be issued maybe!\n");*/   /*just for debug,yanf*/
        send_reply(551, 0, rtsp);    /* 不支持的选项*/
        return ERR_NOERROR;
    }

    /*DEBUG_PRINTF("using sdp to parse media session description!\n");*/   /*just for debug,yanf*/

    /* 获得描述符格式，推荐SDP*/
    if (strstr(rtsp->in_buffer, HDR_ACCEPT) != NULL) 
    {
        if (strstr(rtsp->in_buffer, "application/sdp") != NULL) 
        {
            descr_format = df_SDP_format;
        } 
        else
        {
            /* 其他的描述符格式*/
            send_reply(551, 0, rtsp);    /* 不支持的媒体类型*/
            return ERR_NOERROR;
        }
    }
    
    /*取得序列号,并且必须有这个选项*/
    if ((p = strstr(rtsp->in_buffer, HDR_CSEQ)) == NULL) 
    {
        send_reply(400, 0, rtsp);  /* Bad Request */
        return ERR_NOERROR;
    } 
    else 
    {
        if (sscanf(p, "%254s %d", trash, &(rtsp->rtsp_cseq)) != 2) 
        {
            send_reply(400, 0, rtsp);   /*请求错误*/
            return ERR_NOERROR;
        }
    }


    memset(&media, 0, sizeof(media));
    memset(&req, 0, sizeof(req));
    req.flags = ME_DESCR_FORMAT;
    req.descr_format = descr_format;
    
    /*DEBUG_PRINTF("begin to get media description!");*/   /*just for debug,yanf*/
    
    res = get_media_descr(object, &req, &media, descr);   /*按照请求格式获取媒体描述*/
    
    /*DEBUG_PRINTF("media description got !\n");*/    /*just for debug ,yanf*/
    
    if (res == ERR_NOT_FOUND) 
    {
        send_reply(404, 0, rtsp);   /*没有找到*/
        return ERR_NOERROR;
    }
    if (res == ERR_PARSE || res == ERR_GENERIC || res == ERR_ALLOC) 
    {
        sprintf(szDebug,"get media description error!ret code is :%d",res);
        DEBUG_PRINT_ERROR(szDebug)
        send_reply(500, 0, rtsp);  /*服务器内部错误*/
        return ERR_NOERROR;
    }

    /*如果达到了最大连接数，则重定向*/
    if(max_connection()==ERR_GENERIC)
    {
        /*redirect*/
        return send_redirect_3xx(rtsp, object);  
    }

    #ifdef RTSP_METHOD_LOG
    fnc_log(FNC_LOG_INFO,"DESCRIBE %s RTSP/1.0 ",url);
    #endif
    
    /*DEBUG_PRINTF("prepare to send describe reply to the client!\n");*/    /*just for debug,yanf*/
    
    send_describe_reply(rtsp, object, descr_format, descr);               /*发送DESCRIBE*/


    /*写日志文件*/
    #ifdef RTSP_METHOD_LOG
    if ((p=strstr(rtsp->in_buffer, HDR_USER_AGENT))!=NULL) 
    {
        char cut[strlen(p)];
        strcpy(cut,p);
        p=strstr(cut, "\n");
        cut[strlen(cut)-strlen(p)-1]='\0';
        fnc_log(FNC_LOG_CLIENT,"%s\n",cut);
    }
    else
    {
        fnc_log(FNC_LOG_CLIENT,"- \n");
    }
    #endif
    
    /*DEBUG_PRINTF("end of processing  describe  method!\n");*/   /*just for debug,yanf*/
    return ERR_NOERROR;
}
