/**
 ******************************************************************************
 * @file      log.c
 * @brief     日志记录模块
 * @details   This file including all API functions's 
 *            implement of log.c.
 *
 * @copyright
 ******************************************************************************
 */
 
/*-----------------------------------------------------------------------------
 Section: Includes
 ----------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>

#include "log.h"

/*-----------------------------------------------------------------------------
 Section: Type Definitions
 ----------------------------------------------------------------------------*/
/* NONE */

/*-----------------------------------------------------------------------------
 Section: Constant Definitions
 ----------------------------------------------------------------------------*/
/* NONE */

/*-----------------------------------------------------------------------------
 Section: Global Variables
 ----------------------------------------------------------------------------*/
/* NONE */

/*-----------------------------------------------------------------------------
 Section: Local Variables
 ----------------------------------------------------------------------------*/
static FILE *the_log_fp = NULL;
int the_log_level = 0;
//static char the_log_buf[10240]; //10k 打印buf

/*-----------------------------------------------------------------------------
 Section: Local Function Prototypes
 ----------------------------------------------------------------------------*/
/* NONE */

/*-----------------------------------------------------------------------------
 Section: Function Definitions
 ----------------------------------------------------------------------------*/
#ifdef _WIN32
static int
on_log_exit(void)
{
    log_exit();
    return 0;
}
#endif

/**
 ******************************************************************************
 * @brief   设置日志级别
 * @param[in]  level : 日志级别
 *
 * @return  None
 ******************************************************************************
 */
void
log_set_level(int level)
{
    the_log_level = level;
}

/**
 ******************************************************************************
 * @brief   按当前日期创建日志文件
 * @retval  None
 ******************************************************************************
 */
static void
log_check_file(time_t t)
{
    static int pre_day = -1;
    char fname[4+2+2+1+3+1];
    struct tm daytime;

#ifndef _WIN32
    int _timezone = (int)__timezone;//fixbug
#endif

    if ((int)((t - _timezone) / (1440 * 60)) != pre_day)
    {
        daytime = *localtime(&t);
        snprintf(fname, sizeof(fname), "%04d%02d%02d.log",
                daytime.tm_year + 1900,
                daytime.tm_mon + 1,
                daytime.tm_mday);
        (void)log_exit();
        the_log_fp = fopen(fname, "a+");
        pre_day = (int)((t - _timezone) / (1440 * 60));
    }
}

/**
 ******************************************************************************
 * @brief      日志模块初始化
 * @param[in]  max_frame_size : 最大报文字节数
 *
 * @retval     0 : 初始化成功
 * @retval    -1 : 初始化失败
 ******************************************************************************
 */
int
log_init(void)
{
    log_check_file(time(NULL));
#ifdef _WIN32
    _onexit(on_log_exit);
#endif
    return 0;
}

/**
 ******************************************************************************
 * @brief      日志模块退出
 * @param[in]  None
 *
 * @retval  NONE
 ******************************************************************************
 */
void
log_exit(void)
{
    if (the_log_fp != NULL)
    {
        (void)fclose(the_log_fp);
        the_log_fp = NULL;
    }
}

/**
 ******************************************************************************
 * @brief   打印时标
 * @return  None
 ******************************************************************************
 */
static void
log_time(void)
{
    char buf[1+4+15+1+2];
    struct tm daytime;
    time_t t = time(NULL);
    daytime = *localtime(&t);

    log_check_file(t);
    snprintf(buf, sizeof(buf), "[%04d-%02d-%02d %02d:%02d:%02d] ",
            daytime.tm_year + 1900,
            daytime.tm_mon + 1,
            daytime.tm_mday,
            daytime.tm_hour,
            daytime.tm_min,
            daytime.tm_sec);
    if (the_log_fp)
    {
        (void)fprintf(the_log_fp, buf);
    }
    (void)fprintf(stdout, buf);
}

/**
 ******************************************************************************
 * @brief   记录buf
 * @param[in]  level    : 打印级别
 * @param[in]  *pformat : 名称
 * @param[in]  *pbuffer : 数据首地址
 * @param[in]  len      : 长度
 *
 * @return  None
 *
 * @todo: 速度优化
 ******************************************************************************
 */
void
log_buf(int level,
        const char *pformat,
        const unsigned char *pbuffer,
        int len)
{
    int i;

    if (level <= the_log_level)
    {
        if (the_log_fp)
        {
            log_time();
            (void)fprintf(the_log_fp, pformat);
            (void)fprintf(stdout, pformat);
            for (i = 0; i < len; i++)
            {
                (void)fprintf(the_log_fp, "%02X ", *(pbuffer + i));
                (void)fprintf(stdout, "%02X ", *(pbuffer + i));
            }
            (void)fprintf(the_log_fp, "\n");
            (void)fprintf(stdout, "\n");
            (void)fflush(the_log_fp);
        }
    }
}

/**
 ******************************************************************************
 * @brief   日志print
 * @param[in]  fmt  : 日志信息与printf相同
 *
 * @return  None
 ******************************************************************************
 */
void
log_print(int level,
        const char *fmt, ...)
{
    va_list args;
    va_list args_;

    if (level <= the_log_level)
    {
        va_start( args, fmt );
        log_time();
        if (the_log_fp)
        {
            va_copy(args_, args);
            (void)vfprintf(the_log_fp, fmt, args_);
            (void)fflush(the_log_fp);
        }
        (void)vfprintf(stdout, fmt, args);
    }
}

/*----------------------------------log.c------------------------------------*/
