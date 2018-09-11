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

#define VERSION     "1.2.2"
#define SOFTNAME    "cFep"

static const ptcl_func_t *ptcl = NULL;
static SEM_ID the_sem = NULL;
static prun_t the_prun;
int the_max_frame_bytes = 2048;
unsigned char the_rbuf[2048]; //ȫ�ֻ���

static void
print_ver_info(void);

static int
default_on_exit(void);

extern int
wupdate_start(const char *psoftname,
        const char *psoftver,
        const char *purlini);

/**
 ******************************************************************************
 * @brief   ��ʾ�����ն�������Ϣ
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
    printf("�ն������б�(TCP)\n");
    printf("%-5s%-33s%-22s%-19s\n", "���", "�ն˵�ַ", "IP:port", "���ͨ��ʱ��");
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
    printf("\n�ն������б�(UDP)\n");
    printf("%-5s%-33s%-22s%-19s\n", "���", "�ն˵�ַ", "IP:port", "���ͨ��ʱ��");
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
 * @brief   ��ʾ���ߺ�̨��Ϣ
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
    printf("��̨�����б�\n");
    printf("%-8s%-16s%-24s%-24s\n", "���", "MSA", "IP:port", "���ͨ��ʱ��");
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
 * @brief   �û����봦���߳�
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
                printf("\n��������Լ���(0~2)��");
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
                    printf("���óɹ�!��ǰ���Լ���:%d\n", the_prun.pcfg.default_debug_level);
                }
                else
                {
                    printf("����ʧ��!��ǰ���Լ���:%d\n", the_prun.pcfg.default_debug_level);
                }
                getchar();
                continue;

            case '5':
                //todo:
                printf("\n���ܴ����ƣ���������\n");
                continue;

            case '6':
                //todo:
                printf("\n���ܴ����ƣ���������\n");
                continue;

            case '7':
                printf("\nȷ�ϳ�������[����](Y/N)��");
                do
                {
                    input = getchar();
                } while ((input == '\r') || (input == '\n'));
                if ((input == 'y') || (input == 'Y'))
                {
                    if (OK == semTake(the_sem, 100))
                    {
                        log_print(L_ERROR, "��������������\n");
                        if (!wupdate_start("cFep",
                                VERSION,
                                "http://121.40.80.159/lnsoft/cFep/cFep.ini"))
                        {
                            printf("�Ѿ������°汾!\n");
                        }
                    }
                    semGive(the_sem);
                }
                getchar();
                continue;

            case '8':
                printf("\nȷ���˳���(Y/N)��");
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
               "1. ��ʾ�����ն��б�\n"
               "2. ��ʾ���ߺ�̨�б�\n"
               "3. ��ʾ�汾��Ϣ\n"
               "4. ���õ��Լ���\n"
               "5. ��������\n"
               "6. �޳��ն�\n"
               "7. ��������\n"
               "8. �˳�\n"
               "~~~~~~~~~~~~~~~~~~~\n");
        socket_msleep(100u);
    }
}

/**
 ******************************************************************************
 * @brief   �ж��Ƿ���ڼ������ն�
 * @param[in]  *pc   : ���Ӷ���
 * @param[in]  *addr : ���ն˵�ַ
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
            return 1; //�Ѵ���
        }
    }

    return 0;
}

/**
 ******************************************************************************
 * @brief   ����һ�����ն�
 * @param[in]  *pc   : ���Ӷ���
 * @param[in]  *addr : ���ն˵�ַ
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
 * @brief   ɾ��һ�����ն�
 * @param[in]  *pc   : ���Ӷ���
 * @param[in]  *addr : ���ն˵�ַ
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
 * @brief   ɾ��һ�����Ӷ���
 * @param[in]  *pc : ��ɾ�������Ӷ���
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

    LIST_FOR_EACH_SAFE(piter, ptmp, &pc->cas)  //�ͷż������ն�
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
 * @brief   ���ն˻ظ�����
 * @param[in]  *pc : ���Ӷ���
 * @param[in]  *ph : ���뱨��
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
        pc->is_closing = 1; //����ʧ����Ҫ�ر�
    }
    else
    {
        log_buf(L_NORMAL, "H: ", the_rbuf, sendlen);
    }
}

/**
 ******************************************************************************
 * @brief   ����̨�ظ��Ƿ����߱���
 * @param[in]  *pc : ���Ӷ���
 * @param[in]  *ph : ���뱨��
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
        pc->is_closing = 1; //����ʧ����Ҫ�ر�
    }
    else
    {
        log_buf(L_DEBUG, "O: ", the_rbuf, sendlen);
    }
}

/**
 ******************************************************************************
 * @brief   �ն�ҵ���Ĵ���ص�����
 * @param[in]  *pc  : ����
 * @param[in]  *pbuf: ���Ļ�����
 * @param[in]  len  : ����
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
    struct ListNode *piter;
    connect_t *pc = (connect_t *)p;

    if ((the_prun.pcfg.ptcl_type != 4) && !ptcl->pfn_get_dir(pbuf))    //0��վ-->�ն�   1�ն�-->��վ
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
    ptcl->pfn_addr_get(&addr, pbuf); //��ȡ�ն˵�ַ

    switch (ptcl->pfn_frame_type(pbuf))
    {
        case LINK_LOGIN:
            if (the_prun.pcfg.support_cas_link && ptcl->pfn_addr_cmp(&pc->u, pbuf))
            {
                if (!mem_equal(&pc->u, 0x00, sizeof(pc->u)))
                {
                    ptcl->pfn_addr_get(&pc->u, pbuf); //�״��յ���½��
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
                /* ��ֹ�ն��ظ���¼ */
                struct ListNode *node[] = {
                        &the_prun.terminal_tcp.node,
                        &the_prun.terminal_udp.node};
                for (i = 0; i < ARRAY_SIZE(node); i++)
                {
                    LIST_FOR_EACH(piter, node[i])
                    {
                        pct = MemToObj(piter, connect_t, node);
                        if ((pct != pc) && (!ptcl->pfn_addr_cmp(&pct->u, pbuf)))
                        {
                            log_print(L_DEBUG, "�ն�[%s]�ظ���¼!\n",
                                    ptcl->pfn_addr_str(&pc->u));
                            piter = piter->pPrevNode;
                            delete_pc(pct);
                        }
                        else if (pct != pc)
                        {
                            cas_del(pct, &addr); //����ɾ���ظ��������ն�
                        }
                    }
                }
            }
            log_print(L_DEBUG, "�ն�[%s]��¼\n", ptcl->pfn_addr_str(&pc->u));
            build_reply_packet(pc, pbuf);
            return;

        case LINK_HAERTBEAT:
            if (the_prun.pcfg.is_cfep_reply_heart)
            {
                if ((the_prun.pcfg.support_cas_link && cas_exist(pc, &addr))
                        || !ptcl->pfn_addr_cmp(&pc->u, pbuf))
                {
                    log_print(L_DEBUG, "�ն�[%s]����\n",
                            ptcl->pfn_addr_str(&pc->u));
                    build_reply_packet(pc, pbuf);
                }
                else
                {
                    log_print(L_DEBUG, "�ն�[%s]��¼��ַ��������ַ��ƥ��!\n",
                            ptcl->pfn_addr_str(&pc->u));
                    //��ѡ�ر�����
                    pc->is_closing = 1;
                }
                return;
            }
            break;

        case LINK_EXIT:
            log_print(L_DEBUG, "�ն�[%s]�˳�\n", ptcl->pfn_addr_str(&pc->u));
            build_reply_packet(pc, pbuf);
            if (the_prun.pcfg.support_cas_link && cas_exist(pc, &addr))
            {
                cas_del(pc, &addr); //�������ն˲��ر�socket
            }
            else
            {
                pc->is_closing = 1;
            }
            return;

        default:
            break;
    }

    //47Э�鲻�жϵ�ַ
    if ((the_prun.pcfg.ptcl_type != 4) && ptcl->pfn_addr_cmp(&pc->u, pbuf))
    {
        if (the_prun.pcfg.support_cas)
        {
            cas_add(pc, &addr); //��Ӽ����ն�
        }
        else
        {
            log_print(L_DEBUG, "�ն�[%s]���ĵ�ַ���¼��ַ��ƥ��!\n",
                    ptcl->pfn_addr_str(&pc->u));
            pc->is_closing = 1; //�Ƿ����ֱ�ӹر�����?
            return;
        }
    }
    /* ��ȡmsa */
    ptcl->pfn_msa_get(&msa, pbuf);

    LIST_FOR_EACH(piter, &the_prun.app_tcp.node)
    {
        pct = MemToObj(piter, connect_t, node);
        /* ֻҪ��̨���Ӿ��ϱ���������û�м�¼MSA */
        /**
         * 1. �ն������ϱ�msa==0,���к�̨��ת��
         * 2. ��̨msaΪƥ��Ҫת��
         * 3. 47Э���Զ�����
         */
        if ((the_prun.pcfg.ptcl_type == 4)
                || !ptcl->pfn_is_msa_valid(&msa)
                || !ptcl->pfn_msa_cmp(&pct->u, pbuf))
        {
            sendlen = socket_send(pct->socket, pbuf, len);
            if (sendlen < 0)    //ת��������
            {
                log_print(L_DEBUG, "����ʧ��!\n");
                piter = piter->pPrevNode;
                delete_pc(pct);
            }
        }
    }
}

/**
 ******************************************************************************
 * @brief   ��̨ҵ���Ĵ���ص�����
 * @param[in]  *pc  : ����
 * @param[in]  *pbuf: ���Ļ�����
 * @param[in]  len  : ����
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
    struct ListNode *piter;
    connect_t *pc = (connect_t *)p;
    struct ListNode *node[] = {
            &the_prun.terminal_tcp.node,
            &the_prun.terminal_udp.node};

    if ((the_prun.pcfg.ptcl_type != 4) && ptcl->pfn_get_dir(pbuf))    //0��վ-->�ն�   1�ն�-->��վ
    {
        return;
    }

    ptcl->pfn_msa_get(&pc->u, pbuf);    //������վ��ַ
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
        LIST_FOR_EACH(piter, node[i])
        {
            pct = MemToObj(piter, connect_t, node);
            ptcl->pfn_addr_get(&addr, pbuf);
            if ((the_prun.pcfg.ptcl_type == 4) || cas_exist(pct, &addr)
                    || !ptcl->pfn_addr_cmp(&pct->u, pbuf))
            {
                if (the_prun.pcfg.support_compress)
                {
                    len = EnData((BYTE *)pbuf, len, EXE_COMPRESS_NEW);  //����
                    sendlen = socket_send(pct->socket, SendBuf, len);
                    log_buf(L_DEBUG, "SM: ", SendBuf, len);
                }
                else
                {
                    sendlen = socket_send(pct->socket, pbuf, len);
                }
                if (sendlen < 0)
                {   //ת������
                    piter = piter->pPrevNode;
                    delete_pc(pct);
                }
            }
        }
    }
}

/**
 ******************************************************************************
 * @brief   ���TCP�˿�
 * @return  None
 ******************************************************************************
 */
static void
tcp_accept(slist_t *pslist)
{
    int conn_fd;

    while ((conn_fd = socket_accept(pslist->listen)) != -1)
    {
        //�����������˿��µ�����
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
            ListAddTail(&pc->node, &pslist->node); //��������
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
 * @brief   ���TCP����
 * @return  None
 ******************************************************************************
 */
static void
tcp_read(slist_t *pslist)
{
    int len;
    connect_t *pc;
    struct ListNode *piter;

    LIST_FOR_EACH(piter, &pslist->node)
    {
        pc = MemToObj(piter, connect_t, node);

        while ((len = socket_recv(pc->socket, the_rbuf, sizeof(the_rbuf))) > 0)
        {
            ptcl->pfn_chfrm(pc, &pc->chkfrm, (unsigned char*)the_rbuf, len);
        }
        if (len < 0)
        {
            log_print(L_DEBUG, "addr:%s ��ȡʧ��!\n", ptcl->pfn_addr_str(&pc->u));
            piter = piter->pPrevNode;
            delete_pc(pc);
        }
    }
}

/**
 ******************************************************************************
 * @brief   ���UDP����
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
        //Ѱ����ͬ��IP:port��������chkfrm
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
            //�����, �����ӽڵ�, ��chkfrm
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
                ListAddTail(&pc->node, &pslist->node); //��������
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
 * @brief   ά������
 * @retval  None
 ******************************************************************************
 */
static void
daemo_task(slist_t *pslist)
{
    connect_t *pc;
    struct ListNode *piter;
    time_t cur_time = time(NULL);

    LIST_FOR_EACH(piter, &pslist->node)
    {
        pc = MemToObj(piter, connect_t, node);

        if (((the_prun.pcfg.timeout * 60) < abs(pc->last_time - cur_time))
                /* ���ӳɹ�1����δ��������, �޳� */
                || ((60 < abs(pc->last_time - cur_time) && (pc->last_time == pc->connect_time)))
                || pc->is_closing)
        {
            log_print(L_DEBUG, "addr:%s ��ʱ!\n", ptcl->pfn_addr_str(&pc->u));
            piter = piter->pPrevNode;
            delete_pc(pc);
        }
    }
}

/**
 ******************************************************************************
 * @brief   Ĭ���˳�������
 * @return  None
 ******************************************************************************
 */
static int
default_on_exit(void)
{
    int i;
    connect_t *pc;
    struct ListNode *piter;
    struct ListNode *node[] = {
            &the_prun.app_tcp.node,
            &the_prun.terminal_tcp.node,
            &the_prun.terminal_udp.node};

    log_print(L_ERROR, "cFep�˳�����,�ر���������\n");

    for (i = 0; i < ARRAY_SIZE(node); i++)
    {
        LIST_FOR_EACH(piter, node[i])
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
 * @brief   ��ӡ�汾��Ϣ
 * @return  None
 ******************************************************************************
 */
static void
print_ver_info(void)
{
    log_print(L_ERROR, "*****************************************************\n");
    log_print(L_ERROR, "SanXing cFep [Version %s] Author : LiuNing\n", VERSION);
    log_print(L_ERROR, "Э������       : %s\n", ptcl->pname);
    log_print(L_ERROR, "��̨TCP��¼�˿�: %d\n", the_prun.pcfg.app_tcp_port);
    log_print(L_ERROR, "�ն�TCP��¼�˿�: %d\n", the_prun.pcfg.terminal_tcp_port);
    log_print(L_ERROR, "�ն�UDP��¼�˿�: %d\n", the_prun.pcfg.terminal_udp_port);
//    log_print(L_ERROR, "TCP���ӳ�ʱʱ��:%d����\n", the_prun.pcfg.timeout);
    log_print(L_ERROR, "��������ֽ��� : %d\n", the_prun.pcfg.max_frame_bytes);
    log_print(L_ERROR, "�ն��ظ�����   : %s\n", the_prun.pcfg.support_comm_terminal ? "��" : "��");
    log_print(L_ERROR, "cFepά������   : %s\n", the_prun.pcfg.is_cfep_reply_heart ? "��" : "��");
    log_print(L_ERROR, "���Լ���       : %d\n", the_prun.pcfg.default_debug_level);
    log_print(L_ERROR, "*****************************************************\n");
    log_print(L_ERROR, "* WARNING : DO NOT DRAG THE RIGHT BAR WHEN LOGGING  *\n");
    log_print(L_ERROR, "*           IT WILL BLOCK THE APPLICATION WORKING   *\n");
    log_print(L_ERROR, "*****************************************************\n");
}

/**
 ******************************************************************************
 * @brief   ������
 * @param[in]  None
 * @param[out] None
 * @retval     None
 ******************************************************************************
 */
int main(int argc, char **argv)
{
    int count = 0;

    /* 1. ���ݽṹ��ʼ�� */
    memset(&the_prun, 0x00, sizeof(the_prun));
    InitListHead(&the_prun.app_tcp.node);
    InitListHead(&the_prun.terminal_tcp.node);
    InitListHead(&the_prun.terminal_udp.node);

    /* 2. ������ʼ�� */
    if (ini_get_info(&the_prun.pcfg))
    {
        fprintf(stderr, "�������ļ���ȡ����ʧ��!���������ļ�\n");
        getchar();
        goto __cFep_end;
    }
    the_max_frame_bytes = the_prun.pcfg.max_frame_bytes;
    (void)log_init();
    log_set_level(the_prun.pcfg.default_debug_level);

    switch (the_prun.pcfg.ptcl_type)
    {
        case 0: /* ���� */
            ptcl = gw_ptcl_func_get();
            break;

        case 1: /* ���� */
            ptcl = nw_ptcl_func_get();
            break;

        case 2: /* �㶫���㽭��Լ */
            ptcl = zj_ptcl_func_get();
            break;

        case 3: /* ���ֹ�Լ */
            ptcl = jl_ptcl_func_get();
            break;

        case 4: /* 62056-47 */
            ptcl = p47_ptcl_func_get();
            break;

        case 5: /* 698���� */
            ptcl = p698_ptcl_func_get();
            break;

        default:
            goto __cFep_end;
    }

    if (!ptcl)
    {
        fprintf(stderr, "�޷���ȡЭ�鴦��ӿ�!\n");
        goto __cFep_end;
    }

    /* 3. socket��ʼ�� */
    if (socket_init())
    {
        fprintf(stderr, "socket��ʼ������!\n");
        goto __cFep_end;
    }

    /* 4. ��app�˿ڼ��� */
    the_prun.app_tcp.type = E_TYPE_APP;
    the_prun.app_tcp.listen = socket_listen(the_prun.pcfg.app_tcp_port, E_SOCKET_TCP);
    if (the_prun.app_tcp.listen == -1)
    {
        fprintf(stderr, "������̨��¼�˿�:%dʧ��!\n", the_prun.pcfg.app_tcp_port);
        goto __cFep_end;
    }

    /* 5. ��terminal�˿ڼ��� */
    the_prun.terminal_tcp.type = E_TYPE_TERMINAL;
    the_prun.terminal_tcp.listen = socket_listen(the_prun.pcfg.terminal_tcp_port, E_SOCKET_TCP);
    if (the_prun.terminal_tcp.listen == -1)
    {
        fprintf(stderr, "�����ն˵�¼�˿�:%dʧ��!\n", the_prun.pcfg.terminal_tcp_port);
        goto __cFep_end;
    }

    /* 6. ��terminal��UDP�˿ڼ��� */
    the_prun.terminal_udp.type = E_TYPE_TERMINAL;
    the_prun.terminal_udp.listen = socket_listen(the_prun.pcfg.terminal_udp_port, E_SOCKET_UDP);
    if (the_prun.terminal_udp.listen == -1)
    {
        fprintf(stderr, "�����ն˵�¼UDP�˿�:%dʧ��!\n", the_prun.pcfg.terminal_udp_port);
        goto __cFep_end;
    }

    print_ver_info();
    _onexit(default_on_exit);  //ע��Ĭ���˳�����
    the_sem = semBCreate(0);
    (void)taskSpawn("USR_INPUT", 0, 1024, user_input_thread, 0);

    while (1)
    {
        /* ����̨�˿� */
        tcp_accept(&the_prun.app_tcp);

        /* ����ն˶˿� */
        tcp_accept(&the_prun.terminal_tcp);

        /* ��ȡ��̨���� */
        tcp_read(&the_prun.app_tcp);

        /* ��ȡ�ն����� */
        tcp_read(&the_prun.terminal_tcp);

        /* ��ȡUDP���� */
        udp_read(&the_prun.terminal_udp);

        /* ����ά�� */
        if (!(count & 0x0f))
        {
            daemo_task(&the_prun.terminal_udp);
            daemo_task(&the_prun.terminal_tcp);
            daemo_task(&the_prun.app_tcp);
        }

        if (!(count & 0x03))
        {
            semGive(the_sem);
            socket_msleep(1u);
            semTake(the_sem, 0);
        }
        count++;
    }

__cFep_end:
    socket_exit();
    getchar();

    return 0;
}
