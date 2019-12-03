/**
 ******************************************************************************
 * @file      ttynet.c
 * @brief     C Source file of ttynet.c.
 * @details   This file including all API functions's implement of ttynet.c.
 * @copyright Copyrigth(C), 2008-2012,Sanxing Electric Co.,Ltd.
 ******************************************************************************
 */
 
/*-----------------------------------------------------------------------------
 Section: Includes
 ----------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "log.h"
#include "maths.h"
#include "socket.h"
#include "taskLib.h"
#include "listLib.h"

/*-----------------------------------------------------------------------------
 Section: Type Definitions
 ----------------------------------------------------------------------------*/
typedef struct
{
    struct ListNode node;
    int socket;             //socket连接句柄
    time_t connect_time;    //连接时间
    time_t last_time;       //上一次通信时间
    int flag;               //标志: 是否需要发送设备列表
} client_t;

typedef struct
{
    struct ListNode node;
    client_t *pclient;
    int socket;             //socket连接句柄
    time_t connect_time;    //连接时间
    time_t last_time;       //上一次通信时间
    time_t heart_time;      //上一次心跳时间

    uint8_t type;           //设备类型: 0(终端) other
    uint8_t addr[16];       //表地址
    uint8_t mac[6];         //Mac地址
    uint8_t ipaddr[4];      //IPv4地址
    uint8_t err_cnt;        //故障数量
    uint32_t err_val;       //故障标志: 总共32位,每位对应1个外设,置1表示故障
    uint32_t run_t;         //上电运行总时间(单位s)
    uint8_t ver[32];        //软件版本
    uint8_t date[32];       //软件发布日期
} ttynet_dev_t;

typedef struct
{
    int listen;
    uint16_t telnet_port;
    struct ListNode dev_list;
    struct ListNode client_list;
} ttynet_run_t;

/*-----------------------------------------------------------------------------
 Section: Constant Definitions
 ----------------------------------------------------------------------------*/
#ifndef SXUDP_TTYUDP_PORT
# define SXUDP_TTYUDP_PORT      (19000)
#endif

#ifndef SXUDP_FACTORY_PORT
# define SXUDP_FACTORY_PORT     (19001)
#endif

#define SXUDP_CLIENT_PORT_MIN   (19000)
#define SXUDP_CLIENT_PORT_MAX   (20000)

/*-----------------------------------------------------------------------------
 Section: Global Variables
 ----------------------------------------------------------------------------*/
extern unsigned char the_rbuf[2048]; //全局缓存

/*-----------------------------------------------------------------------------
 Section: Local Variables
 ----------------------------------------------------------------------------*/
static ttynet_run_t the_ttynet;

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
 * @brief   删除一个连接对象
 * @param[in]  *pc : 待删除的连接对象
 *
 * @return  None
 ******************************************************************************
 */
static void
delete_client(client_t *pc)
{
    socket_close(pc->socket);
    ListDelNode(&pc->node);
    free(pc);
}

/**
 ******************************************************************************
 * @brief   检测telnet端口是否有新增客户端
 * @param[in]  *prun : ttynet运行参数
 *
 * @return  None
 ******************************************************************************
 */
static void
ttynet_accept_client(ttynet_run_t *prun)
{
    int conn_fd;

    while ((conn_fd = socket_accept(prun->listen)) != -1)
    {
        //创建服务器端口新的连接
        client_t *pc = malloc(sizeof(client_t));
        if (pc)
        {
            memset(pc, 0x00, sizeof(client_t));
            pc->socket = conn_fd;
            pc->connect_time = time(NULL) - 1;
            pc->last_time = pc->connect_time;
            ListAddTail(&pc->node, &prun->client_list); //加入链表
        }
        else
        {
            log_print(L_ERROR, "out of memory!\n");
            socket_close(conn_fd);
        }
    }
}

/**
 ******************************************************************************
 * @brief   处理客户端命令
 * @param[in]  *prun : ttynet运行参数
 *
 * @retval     None
 ******************************************************************************
 */
static void
ttynet_recv_client_cmd(ttynet_run_t *prun)
{
    int len;
    client_t *pc;
    struct ListNode *ptmp;
    struct ListNode *piter;

    LIST_FOR_EACH_SAFE(piter, ptmp, &prun->client_list)
    {
        pc = MemToObj(piter, client_t, node);

        while ((len = socket_recv(pc->socket, the_rbuf, sizeof(the_rbuf))) > 0)
        {
            //todo: 检测命令
        }
        if (len < 0)
        {
            log_print(L_DEBUG, "telnet client 读取失败!\n");
            piter = piter->pPrevNode;
            delete_client(pc);
        }
    }
}

/**
 ******************************************************************************
 * @brief   处理设备链表
 * @param[in]  *prun : ttynet运行参数
 *
 * @retval     None
 ******************************************************************************
 */
static void
ttynet_recv_device_list(ttynet_run_t *prun)
{

}

/**
 ******************************************************************************
 * @brief   ttynet初始化
 * @param[in]  port : telnet监听端口
 *
 * @retval  OK    : 成功
 * @retval  ERROR : 失败
 ******************************************************************************
 */
status_t
ttynet_init(uint16_t port)
{
    memset(&the_ttynet, 0x00, sizeof(the_ttynet));
    the_ttynet.telnet_port = port;
    InitListHead(&the_ttynet.client_list);
    InitListHead(&the_ttynet.dev_list);

    the_ttynet.listen = socket_listen(the_ttynet.telnet_port, E_SOCKET_TCP);
    if (the_ttynet.listen == -1)
    {
        fprintf(stderr, "监听Telnet登录端口:%d失败!\n", the_ttynet.telnet_port);
        return ERROR;
    }

    return OK;
}

/**
 ******************************************************************************
 * @brief   ttynet定时处理方法
 * @return  None
 ******************************************************************************
 */
void
ttynet_do(void)
{
    ttynet_accept_client(&the_ttynet); //接受客户端

    ttynet_recv_client_cmd(&the_ttynet); //处理客户端命令

    ttynet_recv_device_list(&the_ttynet);
}

/**
 ******************************************************************************
 * @brief   ttynet刷新设备列表
 * @return  None
 ******************************************************************************
 */
void
ttynet_refresh_dev(void)
{

}

/**
 ******************************************************************************
 * @brief   ttynet输出信息
 * @return  None
 ******************************************************************************
 */
void
ttynet_print_info(void)
{

}

/*--------------------------------ttynet.c-----------------------------------*/
