/*
 ============================================================================
 Name        : cFep.c
 Author      : LiuNing
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "lib/zip/CCEMan.h"
#include "maths.h"
#include "socket.h"
#include "param.h"
#include "ptcl.h"
#include "lib.h"
#include "ini.h"
#include "log.h"
#include "taskLib.h"
#include "ttynet.h"

#define VERSION     "1.2.5"
#define SOFTNAME    "cFep"

static const ptcl_func_t *ptcl = NULL;
static SEM_ID the_sem = NULL;
static prun_t the_prun;
int the_max_frame_bytes = 2048;
unsigned char the_rbuf[2048]; //全局缓存

static void
print_ver_info(void);

#ifdef _WIN32
static int
default_on_exit(void);

extern int
wupdate_start(const char *psoftname,
        const char *psoftver,
        const char *purlini);
#endif

/**
 ******************************************************************************
 * @brief   显示在线终端连接信息
 * @retval  None
 ******************************************************************************
 */
static void
show_terminal_list_info(void)
{
    int count = 0;
    connect_t *pc;
    cas_addr_t *pca;
    struct ListNode *piter;
    struct ListNode *piter_cas;

    printf("------------------------------------------------------------------------------\n");
    printf("终端连接列表(TCP)\n");
    printf("%-5s%-33s%-22s%-19s\n", "序号", "终端地址", "IP:port", "最后通信时间");
    LIST_FOR_EACH(piter, &the_prun.terminal_tcp.node)
    {
        count++;
        pc = MemToObj(piter, connect_t, node);
        printf("%-5d%-33s%-22s%-19s\n", count, ptcl->pfn_addr_str(&pc->u),
                socket_get_ip_port_str(pc->socket),
                time_str(pc->last_time));
        LIST_FOR_EACH(piter_cas, &pc->cas)
        {
            pca = MemToObj(piter_cas, cas_addr_t, node);
            printf("%-5d%-33s\n", count, ptcl->pfn_addr_str(&pca->u));
        }
    }
    count = 0;
    printf("\n终端连接列表(UDP)\n");
    printf("%-5s%-33s%-22s%-19s\n", "序号", "终端地址", "IP:port", "最后通信时间");
    LIST_FOR_EACH(piter, &the_prun.terminal_udp.node)
    {
        count++;
        pc = MemToObj(piter, connect_t, node);
        printf("%-5d%-33s%-22s%-19s\n", count, ptcl->pfn_addr_str(&pc->u),
                socket_get_ip_port_str(pc->socket),
                time_str(pc->last_time));
        LIST_FOR_EACH(piter_cas, &pc->cas)
        {
            pca = MemToObj(piter_cas, cas_addr_t, node);
            printf("%-5d%-33s\n", count, ptcl->pfn_addr_str(&pca->u));
        }
    }
    printf("------------------------------------------------------------------------------\n");
}

/**
 ******************************************************************************
 * @brief   显示在线后台信息
 * @retval  None
 ******************************************************************************
 */
static void
show_app_list_info(void)
{
    int count = 0;
    connect_t *pc;
    struct ListNode *piter;

    printf("-------------------------------------------------------------------\n");
    printf("后台连接列表\n");
    printf("%-8s%-16s%-24s%-24s\n", "序号", "MSA", "IP:port", "最后通信时间");
    LIST_FOR_EACH(piter, &the_prun.app_tcp.node)
    {
        count++;
        pc = MemToObj(piter, connect_t, node);
        printf("%-8d%-16d%-24s%-24s\n", count, pc->u.msa,
                socket_get_ip_port_str(pc->socket),
                time_str(pc->last_time));
    }
    printf("-------------------------------------------------------------------\n");
}

/**
 ******************************************************************************
 * @brief   用户输入处理线程
 * @retval  None
 ******************************************************************************
 */
static void
user_input_thread(void *p)
{
    char input = '0';
    (void)p;

    while (1)
    {
        do
        {
            input = getchar();
        } while (input == '\r');
        switch (input)
        {
            case '1':
                if (OK == semTake(the_sem, 100))
                {
                    show_terminal_list_info();
                    semGive(the_sem);
                }
                getchar();
                continue;

            case '2':
                if (OK == semTake(the_sem, 100))
                {
                    show_app_list_info();
                    semGive(the_sem);
                }
                getchar();
                continue;

            case '3':
                if (OK == semTake(the_sem, 100))
                {
                    print_ver_info();
                    semGive(the_sem);
                }
                getchar();
                continue;

            case '4':
                printf("\n请输入调试级别(0~2)？");
                do
                {
                    input = getchar();
                } while ((input == '\r') || (input == '\n'));
                if ((input >= '0') && (input <= '2'))
                {
                    if (OK == semTake(the_sem, 100))
                    {
                        the_prun.pcfg.default_debug_level = input - '0';
                        log_set_level(the_prun.pcfg.default_debug_level);
                        semGive(the_sem);
                    }
                    printf("设置成功!当前调试级别:%d\n", the_prun.pcfg.default_debug_level);
                }
                else
                {
                    printf("设置失败!当前调试级别:%d\n", the_prun.pcfg.default_debug_level);
                }
                getchar();
                continue;

            case '5':
                //todo:
                printf("\n功能待完善！后续开发\n");
                continue;

            case '6':
                //todo:
                printf("\n功能待完善！后续开发\n");
                continue;

            case '7':
                printf("\n确认尝试升级[谨慎](Y/N)？");
                do
                {
                    input = getchar();
                } while ((input == '\r') || (input == '\n'));
                if ((input == 'y') || (input == 'Y'))
                {
                    if (OK == semTake(the_sem, 100))
                    {
                        log_print(L_ERROR, "尝试升级。。。\n");
                        #ifdef _WIN32
                        if (!wupdate_start("cFep",
                                VERSION,
                                "http://121.40.80.159/lnsoft/cFep/cFep.ini"))
                        {
                            printf("已经是最新版本!\n");
                        }
                        #endif
                    }
                    semGive(the_sem);
                }
                getchar();
                continue;

            case '8':
                printf("\n确认退出吗(Y/N)？");
                do
                {
                    input = getchar();
                } while ((input == '\r') || (input == '\n'));
                if ((input == 'y') || (input == 'Y'))
                {
                    exit(0);
                }
                getchar();
                continue;

            default:
                break;
        }
        printf("~~~~~~~~~~~~~~~~~~~\n"
               "1. 显示在线终端列表\n"
               "2. 显示在线后台列表\n"
               "3. 显示版本信息\n"
               "4. 设置调试级别\n"
               "5. 屏蔽心跳\n"
               "6. 剔除终端\n"
               "7. 尝试升级\n"
               "8. 退出\n"
               "~~~~~~~~~~~~~~~~~~~\n");
        socket_msleep(100u);
    }
}

/**
 ******************************************************************************
 * @brief   判断是否存在级联从终端
 * @param[in]  *pc   : 连接对象
 * @param[in]  *addr : 从终端地址
 *
 * @retval  0
 * @retval  1
 ******************************************************************************
 */
static int
cas_exist(const connect_t *pc,
        const addr_t *addr)
{
    cas_addr_t *pca;
    struct ListNode *piter;

    LIST_FOR_EACH(piter, &pc->cas)
    {
        pca = MemToObj(piter, cas_addr_t, node);
        if (!memcmp(&pca->u, addr, sizeof(addr_t)))
        {
            return 1; //已存在
        }
    }

    return 0;
}

/**
 ******************************************************************************
 * @brief   新增一个从终端
 * @param[in]  *pc   : 连接对象
 * @param[in]  *addr : 从终端地址
 *
 * @return  None
 ******************************************************************************
 */
static void
cas_add(connect_t *pc,
        const addr_t *addr)
{
    cas_addr_t *pca;

    if (!cas_exist(pc, addr))
    {
        pca = malloc(sizeof(cas_addr_t));
        if (pca)
        {
            memset(pca, 0x00, sizeof(cas_addr_t));
            memcpy(&pca->u, addr, sizeof(addr_t));
            ListAddTail(&pca->node, &pc->cas);
        }
    }
}

/**
 ******************************************************************************
 * @brief   删除一个从终端
 * @param[in]  *pc   : 连接对象
 * @param[in]  *addr : 从终端地址
 *
 * @return  None
 ******************************************************************************
 */
static void
cas_del(connect_t *pc,
        const addr_t *addr)
{
    cas_addr_t *pca;
    struct ListNode *ptmp;
    struct ListNode *piter;

    LIST_FOR_EACH_SAFE(piter, ptmp, &pc->cas)
    {
        pca = MemToObj(piter, cas_addr_t, node);
        if (!memcmp(&pca->u, addr, sizeof(addr_t)))
        {
            ListDelNode(&pca->node);
            free(pca);
        }
    }
}

/**
 ******************************************************************************
 * @brief   删除一个连接对象
 * @param[in]  *pc : 待删除的连接对象
 *
 * @return  None
 ******************************************************************************
 */
static void
delete_pc(connect_t *pc)
{
    cas_addr_t *pca;
    struct ListNode *ptmp;
    struct ListNode *piter;

    socket_close(pc->socket);
    if (pc->chkfrm.pbuf)
    {
        free(pc->chkfrm.pbuf);
    }

    LIST_FOR_EACH_SAFE(piter, ptmp, &pc->cas)  //释放级联从终端
    {
        pca = MemToObj(piter, cas_addr_t, node);
        ListDelNode(&pca->node);
        free(pca);
    }
    ListDelNode(&pc->node);
    free(pc);
}

/**
 ******************************************************************************
 * @brief   给终端回复报文
 * @param[in]  *pc : 连接对象
 * @param[in]  *ph : 输入报文
 *
 * @return  None
 ******************************************************************************
 */
static void
build_reply_packet(connect_t *pc,
        const unsigned char *ph)
{
    int sendlen;

    sendlen = ptcl->pfn_build_reply_packet(ph, the_rbuf);
    if (0 > socket_send(pc->socket, the_rbuf, sendlen))
    {
        pc->is_closing = 1; //发送失败需要关闭
    }
    else
    {
        log_buf(L_NORMAL, "H: ", the_rbuf, sendlen);
    }
}

/**
 ******************************************************************************
 * @brief   给后台回复是否在线报文
 * @param[in]  *pc : 连接对象
 * @param[in]  *ph : 输入报文
 *
 * @return  None
 ******************************************************************************
 */
static void
build_online_packet(connect_t *pc,
        const unsigned char *ph)
{
    int sec = 0xffffffff;
    int sendlen;
    addr_t addr;
    connect_t *pct;
    struct ListNode *piter;

    if (!ptcl->pfn_build_online_packet || !ptcl->pfn_get_oline_addr)
    {
        return;
    }
    if (!ptcl->pfn_get_oline_addr(&addr, ph))
    {
        return;
    }

    LIST_FOR_EACH(piter, &the_prun.terminal_tcp.node)
    {
        pct = MemToObj(piter, connect_t, node);
        if (!memcmp(&pct->u, &addr, sizeof(addr)))
        {
            sec = abs(time(NULL) - pct->last_time);
            break;
        }
    }
    if (0xffffffff == sec)
    {
        LIST_FOR_EACH(piter, &the_prun.terminal_udp.node)
        {
            pct = MemToObj(piter, connect_t, node);
            if (!memcmp(&pct->u, &addr, sizeof(addr)))
            {
                sec = abs(time(NULL) - pct->last_time);
                break;
            }
        }
    }

    sendlen = ptcl->pfn_build_online_packet(ph, the_rbuf, sec);
    if (0 > socket_send(pc->socket, the_rbuf, sendlen))
    {
        pc->is_closing = 1; //发送失败需要关闭
    }
    else
    {
        log_buf(L_DEBUG, "O: ", the_rbuf, sendlen);
    }
}

/**
 ******************************************************************************
 * @brief   终端业务报文处理回调函数
 * @param[in]  *pc  : 连接
 * @param[in]  *pbuf: 报文缓存区
 * @param[in]  len  : 长度
 *
 * @return  None
 ******************************************************************************
 */
static void
terminal_frame_in_cb(void *p,
        const unsigned char *pbuf,
        int len)
{
    int i;
    addr_t msa;
    addr_t addr;
    int sendlen;
    connect_t *pct;
    struct ListNode *ptmp;
    struct ListNode *piter;
    connect_t *pc = (connect_t *)p;

    if ((the_prun.pcfg.ptcl_type != 4) && !ptcl->pfn_get_dir(pbuf))    //0主站-->终端   1终端-->主站
    {
        return;
    }

    pc->last_time = time(NULL);
    if (socket_type(pc->socket) == E_SOCKET_UDP)
    {
        log_buf(L_NORMAL, "T[UDP]: ", pbuf, len);
    }
    else
    {
        log_buf(L_NORMAL, "T: ", pbuf, len);
    }
    ptcl->pfn_addr_get(&addr, pbuf); //获取终端地址

    switch (ptcl->pfn_frame_type(pbuf))
    {
        case LINK_LOGIN:
            if (the_prun.pcfg.support_cas_link && ptcl->pfn_addr_cmp(&pc->u, pbuf))
            {
                if (!mem_equal(&pc->u, 0x00, sizeof(pc->u)))
                {
                    ptcl->pfn_addr_get(&pc->u, pbuf); //首次收到登陆包
                }
                else
                {
                    cas_add(pc, &addr);
                }
            }
            else
            {
                ptcl->pfn_addr_get(&pc->u, pbuf);
            }

            if (!the_prun.pcfg.support_comm_terminal)
            {
                /* 防止终端重复登录 */
                struct ListNode *node[] = {
                        &the_prun.terminal_tcp.node,
                        &the_prun.terminal_udp.node};
                for (i = 0; i < ARRAY_SIZE(node); i++)
                {
                    LIST_FOR_EACH_SAFE(piter, ptmp, node[i])
                    {
                        pct = MemToObj(piter, connect_t, node);
                        if ((pct != pc) && (!ptcl->pfn_addr_cmp(&pct->u, pbuf)))
                        {
                            log_print(L_DEBUG, "终端[%s]重复登录!\n",
                                    ptcl->pfn_addr_str(&pc->u));
                            piter = piter->pPrevNode;
                            delete_pc(pct);
                        }
                        else if (pct != pc)
                        {
                            cas_del(pct, &addr); //遍历删除重复级联从终端
                        }
                    }
                }
            }
            log_print(L_DEBUG, "终端[%s]登录\n", ptcl->pfn_addr_str(&pc->u));
            build_reply_packet(pc, pbuf);
            return;

        case LINK_HAERTBEAT:
            if (the_prun.pcfg.is_cfep_reply_heart)
            {
                if ((the_prun.pcfg.support_cas_link && cas_exist(pc, &addr))
                        || !ptcl->pfn_addr_cmp(&pc->u, pbuf))
                {
                    log_print(L_DEBUG, "终端[%s]心跳\n",
                            ptcl->pfn_addr_str(&pc->u));
                    build_reply_packet(pc, pbuf);
                }
                else
                {
                    log_print(L_DEBUG, "终端[%s]登录地址与心跳地址不匹配!\n",
                            ptcl->pfn_addr_str(&pc->u));
                    //可选关闭连接
                    pc->is_closing = 1;
                }
                return;
            }
            break;

        case LINK_EXIT:
            log_print(L_DEBUG, "终端[%s]退出\n", ptcl->pfn_addr_str(&pc->u));
            build_reply_packet(pc, pbuf);
            if (the_prun.pcfg.support_cas_link && cas_exist(pc, &addr))
            {
                cas_del(pc, &addr); //级联从终端不关闭socket
            }
            else
            {
                pc->is_closing = 1;
            }
            return;

        default:
            break;
    }

    //47协议不判断地址
    if ((the_prun.pcfg.ptcl_type != 4) && ptcl->pfn_addr_cmp(&pc->u, pbuf))
    {
        if (the_prun.pcfg.support_cas)
        {
            cas_add(pc, &addr); //添加级联终端
        }
        else
        {
            log_print(L_DEBUG, "终端[%s]报文地址与登录地址不匹配!\n",
                    ptcl->pfn_addr_str(&pc->u));
            pc->is_closing = 1; //是否可以直接关闭连接?
            return;
        }
    }
    /* 获取msa */
    ptcl->pfn_msa_get(&msa, pbuf);

    LIST_FOR_EACH_SAFE(piter, ptmp, &the_prun.app_tcp.node)
    {
        pct = MemToObj(piter, connect_t, node);
        /* 只要后台连接就上报，不管有没有记录MSA */
        /**
         * 1. 终端主动上报msa==0,所有后台都转发
         * 2. 后台msa为匹配要转发
         * 3. 47协议自动发送
         */
        if ((the_prun.pcfg.ptcl_type == 4)
                || !ptcl->pfn_is_msa_valid(&msa)
                || !ptcl->pfn_msa_cmp(&pct->u, pbuf))
        {
            sendlen = socket_send(pct->socket, pbuf, len);
            if (sendlen < 0)    //转发出错处理
            {
                log_print(L_DEBUG, "发送失败!\n");
                piter = piter->pPrevNode;
                delete_pc(pct);
            }
        }
    }
}

/**
 ******************************************************************************
 * @brief   后台业务报文处理回调函数
 * @param[in]  *pc  : 连接
 * @param[in]  *pbuf: 报文缓存区
 * @param[in]  len  : 长度
 *
 * @return  None
 ******************************************************************************
 */
static void
app_frame_in_cb(void *p,
        const unsigned char *pbuf,
        int len)
{
    int i;
    int sendlen;
    addr_t addr;
    connect_t *pct;
    struct ListNode *ptmp;
    struct ListNode *piter;
    connect_t *pc = (connect_t *)p;
    struct ListNode *node[] = {
            &the_prun.terminal_tcp.node,
            &the_prun.terminal_udp.node};

    if (the_prun.front_socket != NULL)
    {
        sendlen = socket_send(the_prun.front_socket, pbuf, len);
        if (sendlen < 0)
        {
            socket_close(the_prun.front_socket);
            the_prun.front_socket = NULL;
        }
        else if (sendlen == len)
        {
            if (the_prun.pcfg.front_timeout > 0)
            {
                socket_msleep(the_prun.pcfg.front_timeout / 1000);
            }
        }
        else
        {
            //do nothing
        }
    }

    if ((the_prun.pcfg.ptcl_type != 4) && ptcl->pfn_get_dir(pbuf))    //0主站-->终端   1终端-->主站
    {
        return;
    }

    ptcl->pfn_msa_get(&pc->u, pbuf);    //更新主站地址
    pc->last_time = time(NULL);

//    if (ptcl->support_app_heart)
    {
        switch (ptcl->pfn_frame_type(pbuf))
        {
            case ONLINE:
                log_buf(L_DEBUG, "AO: ", pbuf, len);
                build_online_packet(pc, pbuf);
                return;

            default:
                break;
        }
    }
    log_buf(L_NORMAL, "A: ", pbuf, len);

    for (i = 0; i < ARRAY_SIZE(node); i++)
    {
        LIST_FOR_EACH_SAFE(piter, ptmp, node[i])
        {
            pct = MemToObj(piter, connect_t, node);
            ptcl->pfn_addr_get(&addr, pbuf);
            if ((the_prun.pcfg.ptcl_type == 4) || cas_exist(pct, &addr)
                    || !ptcl->pfn_addr_cmp(&pct->u, pbuf))
            {
                if (the_prun.pcfg.support_compress)
                {
                    len = EnData((BYTE *)pbuf, len, EXE_COMPRESS_NEW);  //加密
                    sendlen = socket_send(pct->socket, SendBuf, len);
                    log_buf(L_DEBUG, "SM: ", SendBuf, len);
                }
                else
                {
                    sendlen = socket_send(pct->socket, pbuf, len);
                }
                if (sendlen < 0)
                {   //转发出错
                    piter = piter->pPrevNode;
                    delete_pc(pct);
                }
            }
        }
    }
}

/**
 ******************************************************************************
 * @brief   检测TCP端口
 * @return  None
 ******************************************************************************
 */
static void
tcp_accept(slist_t *pslist)
{
    void *conn_fd;

    while ((conn_fd = socket_accept(pslist->listen)) != NULL)
    {
        //创建服务器端口新的连接
        connect_t *pc = malloc(sizeof(connect_t));
        if (pc)
        {
            memset(pc, 0x00, sizeof(connect_t));
            pc->socket = conn_fd;
            pc->connect_time = time(NULL) - 1;
            pc->last_time = pc->connect_time;
            InitListHead(&pc->cas);
            switch (pslist->type)
            {
                case E_TYPE_APP:
                    ptcl->pfn_chkfrm_init(&pc->chkfrm, app_frame_in_cb);
                    break;
                case E_TYPE_TERMINAL:
                    ptcl->pfn_chkfrm_init(&pc->chkfrm, terminal_frame_in_cb);
                    break;
                default:
                    delete_pc(pc);
                    continue;   //goto while
            }
            memset(&pc->u, 0x00, sizeof(pc->u));
            ListAddTail(&pc->node, &pslist->node); //加入链表
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
 * @brief   检测TCP数据
 * @return  None
 ******************************************************************************
 */
static void
tcp_read(slist_t *pslist)
{
    int len;
    connect_t *pc;
    struct ListNode *ptmp;
    struct ListNode *piter;

    LIST_FOR_EACH_SAFE(piter, ptmp, &pslist->node)
    {
        pc = MemToObj(piter, connect_t, node);

        while ((len = socket_recv(pc->socket, the_rbuf, sizeof(the_rbuf))) > 0)
        {
            ptcl->pfn_chfrm(pc, &pc->chkfrm, (unsigned char*)the_rbuf, len);
        }
        if (len < 0)
        {
            log_print(L_DEBUG, "addr:%s 读取失败!\n", ptcl->pfn_addr_str(&pc->u));
            piter = piter->pPrevNode;
            delete_pc(pc);
        }
    }
}

/**
 ******************************************************************************
 * @brief   检测UDP数据
 * @return  None
 ******************************************************************************
 */
static void
udp_read(slist_t *pslist)
{
    int len;
    int ip;
    unsigned short port;
    connect_t *pc;
    struct ListNode *piter;
    const socket_info_t *pinfo;

    while (0 < (len = socket_recvfrom(pslist->listen, the_rbuf,
            sizeof(the_rbuf), &ip, &port)))
    {
        if (!ip || !port) continue;
        pc = NULL;
        //寻找相同的IP:port，存在则chkfrm
        LIST_FOR_EACH(piter, &pslist->node)
        {
            pc = MemToObj(piter, connect_t, node);
            pinfo = socket_info_get(pc->socket);
            if (pinfo && (pinfo->remote_ip == ip) && (pinfo->remote_port == port))
            {
                break;
            }
            pc = NULL;
        }

        if (!pc)
        {
            //若不存在, 先增加节点, 再chkfrm
            pc = malloc(sizeof(connect_t));
            if (pc)
            {
                memset(pc, 0x00, sizeof(connect_t));
                pc->socket = socket_info_set(pslist->listen, ip, port);
                pc->connect_time = time(NULL) - 1;
                pc->last_time = pc->connect_time;
                ptcl->pfn_chkfrm_init(&pc->chkfrm, terminal_frame_in_cb);
                memset(&pc->u, 0x00, sizeof(pc->u));
                InitListHead(&pc->cas);
                ListAddTail(&pc->node, &pslist->node); //加入链表
            }
        }

        if (pc)
        {
            ptcl->pfn_chfrm(pc, &pc->chkfrm, (unsigned char*)the_rbuf, len);
        }
        else
        {
            log_print(L_ERROR, "out of memory!\n");
        }
    }
}

/**
 ******************************************************************************
 * @brief   维护任务
 * @retval  None
 ******************************************************************************
 */
static void
daemo_task(slist_t *pslist)
{
    connect_t *pc;
    struct ListNode *ptmp;
    struct ListNode *piter;
    time_t cur_time = time(NULL);

    LIST_FOR_EACH_SAFE(piter, ptmp, &pslist->node)
    {
        pc = MemToObj(piter, connect_t, node);

        if (((the_prun.pcfg.timeout * 60) < abs(pc->last_time - cur_time))
                /* 连接成功1分钟未发送数据, 剔除 */
                || ((60 < abs(pc->last_time - cur_time) && (pc->last_time == pc->connect_time)))
                || pc->is_closing)
        {
            log_print(L_DEBUG, "addr:%s 超时!\n", ptcl->pfn_addr_str(&pc->u));
            piter = piter->pPrevNode;
            delete_pc(pc);
        }
    }
}

#ifdef _WIN32
/**
 ******************************************************************************
 * @brief   默认退出处理方法
 * @return  None
 ******************************************************************************
 */
static int
default_on_exit(void)
{
    int i;
    connect_t *pc;
    struct ListNode *ptmp;
    struct ListNode *piter;
    struct ListNode *node[] = {
            &the_prun.app_tcp.node,
            &the_prun.terminal_tcp.node,
            &the_prun.terminal_udp.node};

    log_print(L_ERROR, "cFep退出处理,关闭所有连接\n");

    for (i = 0; i < ARRAY_SIZE(node); i++)
    {
        LIST_FOR_EACH_SAFE(piter, ptmp, node[i])
        {
            pc = MemToObj(piter, connect_t, node);
            piter = piter->pPrevNode;
            delete_pc(pc);
        }
    }

    socket_exit();

    return 0;
}

/**
 ******************************************************************************
 * @brief   前置通信线程
 * @retval  None
 ******************************************************************************
 */
static void
front_thread(void *p)
{
    (void)p;
    void *socket;

    while (1)
    {
        if (the_prun.front_socket == NULL)
        {
            socket = socket_connect(the_prun.pcfg.front_ip, the_prun.pcfg.front_tcp_port, E_SOCKET_TCP, NULL);
            if (socket > 0)
            {
                semTake(the_sem, 0);
                the_prun.front_socket = socket;
                semGive(the_sem);
            }
        }
        socket_msleep(1000u * 10);
    }
}

/**
 ******************************************************************************
 * @brief   前置数据接收
 * @retval  None
 ******************************************************************************
 */
static void
front_recv(void)
{
    int len;
    int sendlen;
    connect_t *pct;
    struct ListNode *ptmp;
    struct ListNode *piter;

    if (the_prun.front_socket > 0)
    {
        len = socket_recv(the_prun.front_socket, the_rbuf, sizeof(the_rbuf));
        if (len > 0)
        {
            LIST_FOR_EACH_SAFE(piter, ptmp, &the_prun.app_tcp.node)
            {
                pct = MemToObj(piter, connect_t, node);
                /* 只要后台连接就上报，不管有没有记录MSA */
                sendlen = socket_send(pct->socket, the_rbuf, len);
                if (sendlen < 0)    //转发出错处理
                {
                    log_print(L_DEBUG, "前置转发APP失败!\n");
                    piter = piter->pPrevNode;
                    delete_pc(pct);
                }
            }
        }
        else if (len < 0)
        {
            socket_close(the_prun.front_socket);
            the_prun.front_socket = NULL;
        }
        else
        {
            //do nothing
        }
    }
}
#endif

/**
 ******************************************************************************
 * @brief   打印版本信息
 * @return  None
 ******************************************************************************
 */
static void
print_ver_info(void)
{
    log_print(L_ERROR, "*****************************************************\n");
    log_print(L_ERROR, "SanXing cFep [Version %s] Author : LiuNing\n", VERSION);
    log_print(L_ERROR, "协议类型       : %s\n", ptcl ? ptcl->pname : "未知!");
    if (the_prun.pcfg.support_front)
    {
        log_print(L_ERROR, "前置通信       : %s:%d%s\n", the_prun.pcfg.front_ip, the_prun.pcfg.front_tcp_port, the_prun.front_socket > 0 ? " OK": "");
        log_print(L_ERROR, "前置超时       : %d(us)\n", the_prun.pcfg.front_timeout);
    }
    log_print(L_ERROR, "后台TCP登录端口: %d\n", the_prun.pcfg.app_tcp_port);
    log_print(L_ERROR, "终端TCP登录端口: %d\n", the_prun.pcfg.terminal_tcp_port);
    log_print(L_ERROR, "终端UDP登录端口: %d\n", the_prun.pcfg.terminal_udp_port);
//    log_print(L_ERROR, "TCP连接超时时间:%d分钟\n", the_prun.pcfg.timeout);
    log_print(L_ERROR, "报文最大字节数 : %d\n", the_prun.pcfg.max_frame_bytes);
    log_print(L_ERROR, "终端重复上线   : %s\n", the_prun.pcfg.support_comm_terminal ? "是" : "否");
    log_print(L_ERROR, "cFep维护心跳   : %s\n", the_prun.pcfg.is_cfep_reply_heart ? "是" : "否");
    log_print(L_ERROR, "调试级别       : %d\n", the_prun.pcfg.default_debug_level);
    log_print(L_ERROR, "*****************************************************\n");
    log_print(L_ERROR, "* WARNING : DO NOT DRAG THE RIGHT BAR WHEN LOGGING  *\n");
    log_print(L_ERROR, "*           IT WILL BLOCK THE APPLICATION WORKING   *\n");
    log_print(L_ERROR, "*****************************************************\n");
}

/**
 ******************************************************************************
 * @brief   主函数
 * @param[in]  None
 * @param[out] None
 * @retval     None
 ******************************************************************************
 */
int main(int argc, char **argv)
{
    int count = 0;

    /* 1. 数据结构初始化 */
    memset(&the_prun, 0x00, sizeof(the_prun));
    InitListHead(&the_prun.app_tcp.node);
    InitListHead(&the_prun.terminal_tcp.node);
    InitListHead(&the_prun.terminal_udp.node);
    the_prun.front_socket = NULL;

    /* 2. 参数初始化 */
    if (ini_get_info(&the_prun.pcfg))
    {
        fprintf(stderr, "从配置文件获取配置失败!请检查配置文件\n");
        getchar();
        goto __cFep_end;
    }
    the_max_frame_bytes = the_prun.pcfg.max_frame_bytes;
    (void)log_init();
    log_set_level(the_prun.pcfg.default_debug_level);

    switch (the_prun.pcfg.ptcl_type)
    {
        case 0: /* 国网 */
            ptcl = gw_ptcl_func_get();
            break;

        case 1: /* 南网 */
            ptcl = nw_ptcl_func_get();
            break;

        case 2: /* 广东、浙江规约 */
            ptcl = zj_ptcl_func_get();
            break;

        case 3: /* 吉林规约 */
            ptcl = jl_ptcl_func_get();
            break;

        case 4: /* 62056-47 */
            ptcl = p47_ptcl_func_get();
            break;

        case 5: /* 698测试 */
            ptcl = p698_ptcl_func_get();
            break;

        default:
            goto __cFep_end;
    }

    if (!ptcl)
    {
        fprintf(stderr, "无法获取协议处理接口!\n");
        goto __cFep_end;
    }

    /* 3. socket初始化 */
    if (socket_init())
    {
        fprintf(stderr, "socket初始化出错!\n");
        goto __cFep_end;
    }

    /* 4. 启动app端口监听 */
    the_prun.app_tcp.type = E_TYPE_APP;
    the_prun.app_tcp.listen = socket_listen(the_prun.pcfg.app_tcp_port, E_SOCKET_TCP);
    if (the_prun.app_tcp.listen == NULL)
    {
        fprintf(stderr, "监听后台登录端口:%d失败!\n", the_prun.pcfg.app_tcp_port);
        goto __cFep_end;
    }

    /* 5. 启动terminal端口监听 */
    the_prun.terminal_tcp.type = E_TYPE_TERMINAL;
    the_prun.terminal_tcp.listen = socket_listen(the_prun.pcfg.terminal_tcp_port, E_SOCKET_TCP);
    if (the_prun.terminal_tcp.listen == NULL)
    {
        fprintf(stderr, "监听终端登录端口:%d失败!\n", the_prun.pcfg.terminal_tcp_port);
        goto __cFep_end;
    }

    /* 6. 启动terminal的UDP端口监听 */
    the_prun.terminal_udp.type = E_TYPE_TERMINAL;
    the_prun.terminal_udp.listen = socket_listen(the_prun.pcfg.terminal_udp_port, E_SOCKET_UDP);
    if (the_prun.terminal_udp.listen == NULL)
    {
        fprintf(stderr, "监听终端登录UDP端口:%d失败!\n", the_prun.pcfg.terminal_udp_port);
        goto __cFep_end;
    }

    print_ver_info();
    the_sem = semBCreate(0);
#ifdef _WIN32
    _onexit(default_on_exit);  //注册默认退出函数
    (void)taskSpawn("USR_INPUT", 0, 1024, user_input_thread, 0);
    if (the_prun.pcfg.support_front)
    {
        (void)taskSpawn("front", 0, 1024, front_thread, 0);
    }
#endif

    while (1)
    {
#ifdef _WIN32
        /* 读取前置数据 */
        front_recv();
#endif

        /* 检测后台端口 */
        tcp_accept(&the_prun.app_tcp);

        /* 检测终端端口 */
        tcp_accept(&the_prun.terminal_tcp);

        /* 读取后台数据 */
        tcp_read(&the_prun.app_tcp);

        /* 读取终端数据 */
        tcp_read(&the_prun.terminal_tcp);

        /* 读取UDP数据 */
        udp_read(&the_prun.terminal_udp);

        /* 连接维护 */
        if (!(count & 0x0f))
        {
            daemo_task(&the_prun.terminal_udp);
            daemo_task(&the_prun.terminal_tcp);
            daemo_task(&the_prun.app_tcp);
        }

        if (!(count & 0x03))
        {
            semGive(the_sem);
            socket_msleep(10u);
            semTake(the_sem, 0);
        }
        count++;
    }

__cFep_end:
    socket_exit();
    getchar();

    return 0;
}
