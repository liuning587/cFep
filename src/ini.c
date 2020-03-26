/**
 ******************************************************************************
 * @file      ini.c
 * @brief     C Source file of ini.c.
 * @details   This file including all API functions's 
 *            implement of ini.c.	
 *
 * @copyright
 ******************************************************************************
 */
 
/*-----------------------------------------------------------------------------
 Section: Includes
 ----------------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include "iniparser.h"
#include "ini.h"

/*-----------------------------------------------------------------------------
 Section: Type Definitions
 ----------------------------------------------------------------------------*/
/* NONE */

/*-----------------------------------------------------------------------------
 Section: Constant Definitions
 ----------------------------------------------------------------------------*/
#ifndef DEFAULT_INI_FILE
# define DEFAULT_INI_FILE   "./cFep.ini"
#endif

/*-----------------------------------------------------------------------------
 Section: Global Variables
 ----------------------------------------------------------------------------*/
/* NONE */

/*-----------------------------------------------------------------------------
 Section: Local Variables
 ----------------------------------------------------------------------------*/
/* NONE */

/*-----------------------------------------------------------------------------
 Section: Local Function Prototypes
 ----------------------------------------------------------------------------*/
/* NONE */

/*-----------------------------------------------------------------------------
 Section: Global Function Prototypes
 ----------------------------------------------------------------------------*/
/* NONE */

/*-----------------------------------------------------------------------------
 Section: Function Definitions
 ----------------------------------------------------------------------------*/
/**
 ******************************************************************************
 * @brief   创建默认配置文件
 * @return  None
 ******************************************************************************
 */
static void
create_example_ini_file(void)
{
    FILE * ini;

    ini = fopen(DEFAULT_INI_FILE, "w");
    fprintf(ini,
            "#cFep配置By LiuNing\n"
            "[cfg]\n"
            "#是否启用前置通信\n"
            "support_front          = 0\n\n"
            "#前置IP\n"
            "front_ip               = 121.40.80.159\n\n"
            "#前置TCP端口\n"
            "front_tcp_port         = 20014\n\n"
            "#前置超时(单位: us)\n"
            "front_timeout          = 0\n\n"
            "#后台TCP端口\n"
            "app_tcp_port           = 20014\n\n"
            "#终端TCP端口\n"
            "terminal_tcp_port      = 20013\n\n"
            "#终端UDP端口\n"
            "terminal_udp_port      = 20013\n\n"
            "#TCP连接超时时间(单位:分钟)\n"
            "timeout                = 30\n\n"
            "#协议类型,0:国网 1:南网 2:广东、浙江规约 3:吉林规约 4:62056-47 5:698测试\n"
            "ptcl_type              = 0\n\n"
            "#是否支持加密(南网使用)\n"
            "support_compress       = 0\n\n"
            "#是否级联(南网使用)\n"
            "support_cas            = 0\n\n"
            "#是否级联终端登陆、心跳(南网使用)\n"
            "support_cas_link       = 0\n\n"
            "#单报文最大字节数(通常1k~2k)\n"
            "max_frame_bytes        = 2048\n\n"
            "#是否支持终端重复登陆(Y/N)\n"
            "support_comm_terminal  = N\n\n"
            "#是否支持前置机维护心跳(Y/N)\n"
            "is_cfep_reply_heart    = Y\n\n"
            "#默认调试级别\n"
            "#  0 : 打印重要信息\n"
            "#  1 : 打印重要信息 + 报文日志\n"
            "#  2 : 打印重要信息 + 报文日志 + 调试信息\n"
            "default_debug_level    = 0\n"
            );
    fclose(ini);
}

/**
 ******************************************************************************
 * @brief   从配置文件中获取文件合并信息
 * @param[out] *pinfo   : 返回info
 *
 * @retval     -1 失败
 * @retval      0 成功
 ******************************************************************************
 */
int
ini_get_info(pcfg_t *pinfo)
{
    dictionary  *   ini ;
    int vtmp;

    memset(pinfo, 0x00, sizeof(*pinfo));

    ini = iniparser_load(DEFAULT_INI_FILE);
    if (NULL == ini)
    {
        create_example_ini_file();
        ini = iniparser_load(DEFAULT_INI_FILE);
        if (ini == NULL)
        {
            return -1;
        }
    }

    iniparser_dump(ini, NULL);//stderr

    //后台TCP端口
    vtmp = iniparser_getint(ini, "cfg:app_tcp_port", -1);
    if (vtmp == -1)
    {
        iniparser_freedict(ini);
        return -1;
    }
    if ((vtmp < 0) || (vtmp > 65535))
    {
        fprintf(stderr, "后台TCP端口[%d]非法!\n", vtmp);
        pinfo->app_tcp_port = 20014;
    }
    else
    {
        pinfo->app_tcp_port = vtmp;
    }

    //终端TCP端口
    vtmp = iniparser_getint(ini, "cfg:terminal_tcp_port", -1);
    if (vtmp == -1)
    {
        iniparser_freedict(ini);
        return -1;
    }
    if ((vtmp < 0) || (vtmp > 65535))
    {
        fprintf(stderr, "终端TCP端口[%d]非法!\n", vtmp);
        pinfo->terminal_tcp_port = 20013;
    }
    else
    {
        pinfo->terminal_tcp_port = vtmp;
    }

    //终端UDP端口
    vtmp = iniparser_getint(ini, "cfg:terminal_udp_port", -1);
    if (vtmp == -1)
    {
        iniparser_freedict(ini);
        return -1;
    }
    if ((vtmp < 0) || (vtmp > 65535))
    {
        fprintf(stderr, "终端UDP端口[%d]非法!\n", vtmp);
        pinfo->terminal_udp_port = 20013;
    }
    else
    {
        pinfo->terminal_udp_port = vtmp;
    }

    //TCP连接超时时间(单位:分钟)
    vtmp = iniparser_getint(ini, "cfg:timeout", -1);
    if (vtmp == -1)
    {
        iniparser_freedict(ini);
        return -1;
    }
    if ((vtmp < 0) || (vtmp > 1440))
    {
        fprintf(stderr, "TCP连接超时时间[%d]非法!\n", vtmp);
        pinfo->timeout = 30;    //default 30 min
    }
    else
    {
        pinfo->timeout = vtmp;
    }

    //协议类型
    vtmp = iniparser_getint(ini, "cfg:ptcl_type", -1);
    if (vtmp == -1)
    {
        iniparser_freedict(ini);
        return -1;
    }
    if ((vtmp < 0) || (vtmp > 5))
    {
        fprintf(stderr, "协议类型[%d]非法!采用默认值0\n", vtmp);
        pinfo->ptcl_type = 0;
    }
    else
    {
        pinfo->ptcl_type = vtmp;
    }

    //是否加密
    vtmp = iniparser_getint(ini, "cfg:support_compress", -1);
    if (vtmp == -1)
    {
        iniparser_freedict(ini);
        return -1;
    }
    if ((vtmp < 0) || (vtmp > 1))
    {
        fprintf(stderr, "是否加密[%d]非法!采用默认值0\n", vtmp);
        pinfo->support_compress = 0;
    }
    else
    {
        pinfo->support_compress = (pinfo->ptcl_type == 1) ? vtmp : 0;
    }

    //是否支持级联
    vtmp = iniparser_getint(ini, "cfg:support_cas", -1);
    if (vtmp == -1)
    {
        iniparser_freedict(ini);
        return -1;
    }
    if ((vtmp < 0) || (vtmp > 1))
    {
        fprintf(stderr, "是否加密[%d]非法!采用默认值0\n", vtmp);
        pinfo->support_cas = 0;
    }
    else
    {
        pinfo->support_cas = (pinfo->ptcl_type == 1) ? vtmp : 0;
    }

    //是否支持级联终端登陆、心跳
    vtmp = iniparser_getint(ini, "cfg:support_cas_link", -1);
    if (vtmp == -1)
    {
        iniparser_freedict(ini);
        return -1;
    }
    if ((vtmp < 0) || (vtmp > 1))
    {
        fprintf(stderr, "是否加密[%d]非法!采用默认值0\n", vtmp);
        pinfo->support_cas_link = 0;
    }
    else
    {
        pinfo->support_cas_link = (pinfo->ptcl_type == 1) ? vtmp : 0;
    }

    //单报文最大字节数
    vtmp = iniparser_getint(ini, "cfg:max_frame_bytes", -1);
    if (vtmp == -1)
    {
        iniparser_freedict(ini);
        return -1;
    }
    if ((vtmp < 512) || (vtmp > 8192))
    {
        fprintf(stderr, "单报文最大字节数[%d]非法!\n", vtmp);
        pinfo->max_frame_bytes = 2048;
    }
    else
    {
        pinfo->max_frame_bytes = vtmp;
    }

    //允许终端重复上线
    vtmp = iniparser_getboolean(ini, "cfg:support_comm_terminal", -1);
    if (vtmp == -1)
    {
        iniparser_freedict(ini);
        return -1;
    }
    if ((vtmp < 0) || (vtmp > 1))
    {
        fprintf(stderr, "允许终端重复上线[%d]非法!\n", vtmp);
        pinfo->support_comm_terminal = 1;
    }
    else
    {
        pinfo->support_comm_terminal = vtmp;
    }

    //允许前置机维护心跳命令
    vtmp = iniparser_getboolean(ini, "cfg:is_cfep_reply_heart", -1);
    if (vtmp == -1)
    {
        iniparser_freedict(ini);
        return -1;
    }
    if ((vtmp < 0) || (vtmp > 1))
    {
        fprintf(stderr, "允许前置机维护心跳命令[%d]非法!\n", vtmp);
        pinfo->is_cfep_reply_heart = 1;
    }
    else
    {
        pinfo->is_cfep_reply_heart = vtmp;
    }

    //默认调试级别
    vtmp = iniparser_getint(ini, "cfg:default_debug_level", -1);
    if (vtmp == -1)
    {
        iniparser_freedict(ini);
        return -1;
    }
    if ((vtmp < 0) || (vtmp > 2))
    {
        fprintf(stderr, "默认调试级别[%d]非法!\n", vtmp);
        pinfo->default_debug_level = 0;
    }
    else
    {
        pinfo->default_debug_level = vtmp;
    }

    iniparser_freedict(ini);

    return 0;
}

/*---------------------------------ini.c-------------------------------------*/
