/**
 ******************************************************************************
 * @file       param.h
 * @brief      API include file of param.h.
 * @details    This file including all API functions's declare of param.h.
 * @copyright  
 *
 ******************************************************************************
 */
#ifndef PARAM_H_
#define PARAM_H_ 

/*-----------------------------------------------------------------------------
 Section: Includes
 ----------------------------------------------------------------------------*/
#include "listLib.h"
#include "ptcl.h"

/*-----------------------------------------------------------------------------
 Section: Macro Definitions
 ----------------------------------------------------------------------------*/
/* None */

/*-----------------------------------------------------------------------------
 Section: Type Definitions
 ----------------------------------------------------------------------------*/
typedef enum
{
    E_TYPE_APP = 0,
    E_TYPE_TERMINAL,
} conn_type_e;

typedef struct
{
    int support_front;                      /**< 是否启用前置通信 */
    char front_ip[64];                      /**< 前置IP地址 */
    unsigned short int front_tcp_port;      /**< 前置TCP端口 */
    int front_timeout;                      /**< 前置通信超时(单位: us) */

    unsigned short int app_tcp_port;        /**< 后台TCP端口 */
    unsigned short int terminal_tcp_port;   /**< 终端TCP端口 */
    unsigned short int terminal_udp_port;   /**< 终端UDP端口 */
    unsigned short int telnet_port;         /**< telnet端口 */

    int timeout;                            /**< TCP连接超时时间(单位:分钟) */

    int ptcl_type;                          /**< 协议类型 */
    int support_compress;                   /**< 支持南网加密算法 */
    int support_cas;                        /**< 支持南网级联 */
    int support_cas_link;                   /**< 支持南网终端登陆、心跳 */
    int max_frame_bytes;                    /**< 单报文最大字节数 */
    int support_comm_terminal;              /**< 允许终端重复上线 */
    int is_cfep_reply_heart;                /**< 允许前置机维护心跳命令 */

    int terminal_heartbeat_cycle_multiple;  /**< 终端心跳超时倍数0表示无需关心 */
    int terminal_disconnect_mode;           /**< 终端断开连接模式: 0关闭TCP; 1关闭转发 */

    /**
     * 默认调试级别
     * 0 : 打印出错信息
     * 1 : 打印出错信息 + 报文日志
     * 2 : 打印出错信息 + 报文日志 + 调试打印信息
     */
    int default_debug_level;
} pcfg_t;

/** socket list */
typedef struct
{
    struct ListNode node;
    int listen;
    conn_type_e type;   //类别
} slist_t;

/** 运行参数 */
typedef struct
{
    pcfg_t pcfg;
    slist_t app_tcp;
    slist_t terminal_tcp;
    slist_t terminal_udp;

    int front_socket;
} prun_t;

/** 连接结构 */
typedef struct _connect_t
{
    struct ListNode node;
    int socket;             /**< socket连接句柄 */
    //void *saddr;            /**< UDP连接时使用struct sockaddr_in */
    time_t connect_time;    /**< 连接时间 */
    time_t last_time;       /**< 上一次通信时间 */
    time_t last_heartbeat;  /**< 上一次心跳时间 */
    int heartbeat_cycle;    /**< 心跳周期(单位:s) */
    chkfrm_t chkfrm;
    int is_closing;         /**< 准备关闭 */
    addr_t u;               /**< 地址 */
    struct ListNode cas;    /**< 级联终端列表 */
//    struct _connect_t *precon; //暂时不用
} connect_t;

typedef struct
{
    struct ListNode node;
    addr_t u;               /**< 地址 */
} cas_addr_t;

/*-----------------------------------------------------------------------------
 Section: Globals
 ----------------------------------------------------------------------------*/
/* NONE */

/*-----------------------------------------------------------------------------
 Section: Function Prototypes
 ----------------------------------------------------------------------------*/
/* NONE */


#endif /* PARAM_H_ */
/*-----------------------------End of param.h--------------------------------*/
