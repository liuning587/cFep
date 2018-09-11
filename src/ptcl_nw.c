/**
 ******************************************************************************
 * @file      ptcl_gw.c
 * @brief     C Source file of ptcl_gw.c.
 * @details   This file including all API functions's 
 *            implement of ptcl_gw.c.	
 *
 * @copyright 
 ******************************************************************************
 */
 
/*-----------------------------------------------------------------------------
 Section: Includes
 ----------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "lib/zip/CCEMan.h"
#include "param.h"
#include "lib.h"
#include "ptcl.h"
#include "log.h"

/*-----------------------------------------------------------------------------
 Section: Type Definitions
 ----------------------------------------------------------------------------*/
#pragma pack(push, 1)
//报文头结构体
typedef struct FrameNWHeaderStruct
{
    unsigned char frameStart;       //0x68
    unsigned short lenArea;         //长度（控制域、地址域、链路用户数据长度总数）
    unsigned short lenArea_;        //重复的长度域
    unsigned char dataAreaStart;    //0x68
    union
    {
        struct
        {
            unsigned char FuncCode : 4 ,    //D0--D3控制域功能码
                          FCV      : 1 ,    //D4帧计数有效位
                          FCB_ACD  : 1 ,    //D5帧计数位 或者 请求访问位
                          PRM      : 1 ,    //D6启动标志位
                          DIR      : 1 ;    //D7传输方向位
        };
        unsigned char ctrlCodeArea;
    };
    union
    {
        unsigned char deviceAddr[6];
//        unsigned long long logicaddr0 : 24;//终端逻辑地址0
//        unsigned long long logicaddr1 : 24;//终端逻辑地址1
        struct
        {
            unsigned char canton[3];
            unsigned char addr[3];
        };
    };
    unsigned char hostID;   //MSA主站地址
    unsigned char AFN;
    union
    {
        struct
        {
            unsigned char frameSeq   :     4 ,//D0--D3帧序号
                          CON        :     1 ,//D4请求确认标志位
                          FIN        :     1 ,//D5结束帧标志
                          FIR        :     1 ,//D6首帧标志
                          TPV        :     1 ;//D7帧时间标签有效位
        };
        unsigned char seqArea;
    };
} nw_header_t;

//时间标识结构
struct TPStruct
{
    unsigned char PFC;
    unsigned char sec;
    unsigned char min;
    unsigned char hour;
    unsigned char day;
    unsigned char timeOut;
};
#pragma pack(pop)

/*-----------------------------------------------------------------------------
 Section: Constant Definitions
 ----------------------------------------------------------------------------*/
#define CHKFRAME_TIMEOUT                5   /**< 报文超时时间默认10秒  */
#define FRAME_NO_DATA_LEN               14   /**< 无数据内容有数据标示的报文头长度 */

/* states for scanning incomming bytes from a bytestream */
#define NW_FRAME_STATES_NULL               0    /**< no synchronisation */
#define NW_FRAME_STATES_FLAG_FIR           1    /**< have the first Flag Byte received */
#define NW_FRAME_STATES_LEN1_1             2    /**< have the length byte */
#define NW_FRAME_STATES_LEN1_2             3    /**< have the length byte */
#define NW_FRAME_STATES_LEN2_1             4    /**< have the confirm length byte */
#define NW_FRAME_STATES_LEN2_2             5    /**< have the confirm length byte */
#define NW_FRAME_STATES_FLAG_SEC           6    /**< have the second Flag Byte received */
#define NW_FRAME_STATES_CONTROL            7    /**< have the control byte */
#define NW_FRAME_STATES_A3                 8    /**< have the A3 byte */
#define NW_FRAME_STATES_LINK_USER_DATA     9    /**< have the link user data bytes */
#define NW_FRAME_STATES_CS                 10   /**< wait for the CS */
#define NW_FRAME_STATES_END                11   /**< wait for the 16H */
#define NW_FRAME_STATES_COMPLETE           12   /**< complete frame */

#define NW_COMPRESS_OPER                   13   /**< 加密方式 */
#define NW_COMPRESS_LEN1                   14   /**< len */
#define NW_COMPRESS_LEN2                   15   /**< len */
#define NW_COMPRESS_DATA                   16   /**< data */
#define NW_COMPRESS_END                    17   /**< wait for 77H */

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
 * @brief   国网报文检测初始化
 * @param[in]  *pchk         : 报文检测对象
 * @param[in]  *pfn_frame_in : 当收到合法报文后执行的回调函数
 *
 * @return  None
 ******************************************************************************
 */
static void
nw_chkfrm_init(chkfrm_t *pchk,
        void (*pfn_frame_in)(void*, const unsigned char*, int))
{
    memset(pchk, 0x00, sizeof(chkfrm_t));
    pchk->frame_state = NW_FRAME_STATES_NULL;
    pchk->update_time = time(NULL);
    pchk->overtime = CHKFRAME_TIMEOUT;
    pchk->pfn_frame_in = pfn_frame_in;
}

/**
 ******************************************************************************
 * @brief   国网报文检测
 * @param[in]  *pc      : 连接对象(fixme : 是否需要每次传入?)
 * @param[in]  *pchk    : 报文检测对象
 * @param[in]  *rxBuf   : 输入数据
 * @param[in]  rxLen    : 输入数据长度
 *
 * @return  None
 ******************************************************************************
 */
static void
nw_chkfrm(void *pc,
        chkfrm_t *pchk,
        const unsigned char *rxBuf,
        int rxLen)
{
    /* 如果已经完成的桢则重新开始 */
    if (pchk->frame_state == NW_FRAME_STATES_COMPLETE)
    {
        pchk->frame_state = NW_FRAME_STATES_NULL;
    }

    /* 如果超时则重新开始 */
    if (((time(NULL) - pchk->update_time) > pchk->overtime)
        || ((pchk->update_time - time(NULL)) > pchk->overtime))
    {
        pchk->frame_state = NW_FRAME_STATES_NULL;
    }

    while (rxLen > 0)
    {
        switch (pchk->frame_state)
        {
            case NW_FRAME_STATES_NULL:

                if ((*rxBuf == 0x68) || (*rxBuf == 0x88))
                {
                    if (!pchk->pbuf)
                    {
                        pchk->pbuf = malloc(the_max_frame_bytes);
                        if (!pchk->pbuf)
                        {
                            return; //
                        }
                    }
                    pchk->pbuf_pos = 0;
                    if (*rxBuf == 0x88)
                    {
                        pchk->frame_state = NW_COMPRESS_OPER; //密文
                    }
                    else
                    {
                        pchk->frame_state = NW_FRAME_STATES_LEN1_1;
                    }
                    pchk->update_time = time(NULL);
                    pchk->dlen = 0;
                }
                break;

            case NW_FRAME_STATES_LEN1_1: /* 检测L1的低字节 */
                pchk->frame_state = NW_FRAME_STATES_LEN1_2;/* 为兼容主站不检测规约类型 */
                pchk->dlen = *rxBuf;
                break;

            case NW_FRAME_STATES_LEN1_2: /* 检测L1的高字节 */
                pchk->dlen += ((unsigned int)*rxBuf << 8u);
                if (pchk->dlen > (the_max_frame_bytes - 8))
                {
                    free(pchk->pbuf);
                    pchk->pbuf = NULL;
                    pchk->frame_state = NW_FRAME_STATES_NULL;
                }
                else
                {
                    pchk->frame_state = NW_FRAME_STATES_LEN2_1;
                }
                break;

            case NW_FRAME_STATES_LEN2_1: /*检测L2的低字节*/
                pchk->frame_state = NW_FRAME_STATES_LEN2_2;
                pchk->cfm_len = *rxBuf;
                break;

            case NW_FRAME_STATES_LEN2_2: /*检测L2的高字节*/
                pchk->cfm_len += ((unsigned int)*rxBuf << 8u);
                if (pchk->cfm_len == pchk->dlen)
                {
                    pchk->frame_state = NW_FRAME_STATES_FLAG_SEC;
                }
                else
                {
                    free(pchk->pbuf);
                    pchk->pbuf = NULL;
                    pchk->frame_state = NW_FRAME_STATES_NULL;
                }
                break;

            case NW_FRAME_STATES_FLAG_SEC:
                if (*rxBuf == 0x68)
                {
                    pchk->frame_state = NW_FRAME_STATES_CONTROL;
                }
                else
                {
                    free(pchk->pbuf);
                    pchk->pbuf = NULL;
                    pchk->frame_state = NW_FRAME_STATES_NULL;
                }
                break;

            case NW_FRAME_STATES_CONTROL:
                pchk->cs = *rxBuf;
                pchk->frame_state = NW_FRAME_STATES_A3;/* 不能检测方向，因为级联有上行报文 */
                break;

            case NW_FRAME_STATES_A3:
                pchk->cs += *rxBuf;
                if (pchk->pbuf_pos == 13)
                {
                    pchk->frame_state = NW_FRAME_STATES_LINK_USER_DATA;
                }
                break;

            case NW_FRAME_STATES_LINK_USER_DATA:
                pchk->cs += *rxBuf;
                if (pchk->pbuf_pos == (5 + pchk->dlen))
                {
                    pchk->frame_state = NW_FRAME_STATES_CS;
                }
                break;

            case NW_FRAME_STATES_CS:
                if (*rxBuf == pchk->cs)
                {
                    pchk->frame_state = NW_FRAME_STATES_END;
                }
                else
                {
                    free(pchk->pbuf);
                    pchk->pbuf = NULL;
                    pchk->frame_state = NW_FRAME_STATES_NULL;
                }
                break;

            case NW_FRAME_STATES_END:
                if (*rxBuf == 0x16)
                {
                    pchk->frame_state = NW_FRAME_STATES_COMPLETE;
                }
                else
                {
                    free(pchk->pbuf);
                    pchk->pbuf = NULL;
                    pchk->frame_state = NW_FRAME_STATES_NULL;
                }
                break;

            case NW_COMPRESS_OPER:
                if (*rxBuf == 0x01)
                {
                    pchk->frame_state = NW_COMPRESS_LEN1;
                }
                else
                {
                    free(pchk->pbuf);
                    pchk->pbuf = NULL;
                    pchk->frame_state = NW_FRAME_STATES_NULL;
                }
                break;

            case NW_COMPRESS_LEN1:
                pchk->frame_state = NW_COMPRESS_LEN2;
                pchk->dlen = ((unsigned int)*rxBuf << 8u);
                break;

            case NW_COMPRESS_LEN2:
                pchk->dlen += *rxBuf;
                if (pchk->dlen > (the_max_frame_bytes - 8))
                {
                    free(pchk->pbuf);
                    pchk->pbuf = NULL;
                    pchk->frame_state = NW_FRAME_STATES_NULL;
                }
                else
                {
                    pchk->frame_state = NW_COMPRESS_DATA;
                }
                break;

            case NW_COMPRESS_DATA:
                if (pchk->pbuf_pos == (3 + pchk->dlen))
                {
                    pchk->frame_state = NW_COMPRESS_END;
                }
                break;

            case NW_COMPRESS_END:
                if (*rxBuf == 0x77)
                {
                    int delen;
                    pchk->frame_state = NW_FRAME_STATES_COMPLETE;
                    pchk->pbuf[pchk->pbuf_pos] = *rxBuf;
                    log_buf(L_DEBUG, "RM: ", pchk->pbuf, pchk->pbuf_pos + 1);
                    delen = DeData(pchk->pbuf, pchk->pbuf_pos + 1);
                    if (0 < delen)
                    {
                        free(pchk->pbuf);
                        pchk->pbuf = NULL;
                        pchk->frame_state = NW_FRAME_STATES_NULL;
                        log_buf(L_DEBUG, "JM: ", RecvBuf, delen);
                        nw_chkfrm(pc, pchk, RecvBuf, delen);
                    }
                }
                else
                {
                    free(pchk->pbuf);
                    pchk->pbuf = NULL;
                    pchk->frame_state = NW_FRAME_STATES_NULL;
                }
                break;

            default:
                break;
        }

        if (pchk->frame_state != NW_FRAME_STATES_NULL)
        {
            pchk->pbuf[pchk->pbuf_pos] = *rxBuf;
            pchk->pbuf_pos++;
        }

        /* 完整报文，调用处理函数接口 */
        if (pchk->frame_state == NW_FRAME_STATES_COMPLETE)
        {
            if (pchk->pfn_frame_in)
            {
                pchk->pfn_frame_in(pc, pchk->pbuf, pchk->dlen + 8); //这里处理业务
            }

            free(pchk->pbuf);
            pchk->pbuf = NULL;
            pchk->frame_state = NW_FRAME_STATES_NULL;
            pchk->pbuf_pos = 0;
            pchk->dlen = 0;
        }
        rxLen--;
        rxBuf++;
    }
}

/**
 ******************************************************************************
 * @brief   获取报文传输方向,0:主站-->终端, 1:终端-->主站
 * @param[in]  *p : 报文缓存
 *
 * @retval  0 : 主站-->终端
 * @retval  1 : 终端-->主站
 ******************************************************************************
 */
static int
nw_get_dir(const unsigned char* p)
{
    nw_header_t *ph = (nw_header_t *)p;

    return ph->DIR;
}

/**
 ******************************************************************************
 * @brief   根据pnfn偏移获取fn
 * @param[in]  输入2字节fn
 * @retval  fn
 ******************************************************************************
 */
static unsigned int
getPnFn(const unsigned char* pFnPn)
{
    return *(unsigned int*) (pFnPn + 2);
}

/**
 ******************************************************************************
 * @brief   获取报文类型
 * @param[in]  *p : 报文缓存
 *
 * @retval  报文类型
 ******************************************************************************
 */
static func_type_e
nw_frame_type(const unsigned char* p)
{
    nw_header_t *ph = (nw_header_t *)p;

    if (ph->AFN == 0x02) //
    {
        if (ph->lenArea >= 6)
        {
            switch (getPnFn((unsigned char*)&ph[1]))
            {
            case 0xE0001000:
                return LINK_LOGIN;
            case 0xE0001002:
                return LINK_EXIT;
            case 0xE0001001:
                return LINK_HAERTBEAT;
            default:
                break;
            }
        }
    }
    return OTHER;
}

/**
 ******************************************************************************
 * @brief   心跳、登陆包回应
 * @param[in]  *p  : 输入报文缓存
 * @param[in]  *po : 输出报文缓存
 *
 * @return  输出报文长度
 ******************************************************************************
 */
static int
nw_build_reply_packet(const unsigned char *p,
        unsigned char *po)
{
    nw_header_t *pin = (nw_header_t *)p;
    nw_header_t *psend = (nw_header_t *)po;
    int pos = sizeof(nw_header_t);

    psend->frameStart = 0x68;
    psend->lenArea = 0x11;
    psend->lenArea_ = 0x11;
    psend->dataAreaStart = 0x68;
    psend->ctrlCodeArea = 0x0B;
    memcpy(psend->deviceAddr, pin->deviceAddr, 6);
    psend->hostID = pin->hostID; //msa = 2
    psend->AFN = 0;
    psend->TPV = 0;
    psend->FIR = 1;
    psend->FIN = 1; //单帧
    psend->CON = 0; //在所收到的报文中，CON位置“1”，表示需要对该帧报文进行确认；置“0”，表示不需要对该帧报文进行确认。
    psend->frameSeq = pin->frameSeq;
    po[pos + 0] = 0x00;
    po[pos + 1] = 0x00;
    po[pos + 2] = 0x00;
    po[pos + 3] = 0x00;
    po[pos + 4] = 0x00;
    po[pos + 5] = 0xE0;
    po[pos + 6] = 0x00;
    po[pos + 7] = get_cs(po + 6, psend->lenArea); //cs
    po[pos + 8] = 0x16;

    return pos + 9;
}

/**
 ******************************************************************************
 * @brief   终端地址和报文中的终端地址比较
 * @param[in]  *paddr : 输入终端地址
 * @param[in]  *p     : 输入报文
 *
 * @retval  1 : 不相同
 * @retval  0 :   相同
 ******************************************************************************
 */
static int
nw_addr_cmp(const addr_t *paddr,
        const unsigned char *p)
{
    nw_header_t *ph = (nw_header_t *)p;

    return memcmp(ph->deviceAddr, paddr, 6) ? 1 : 0;
}

/**
 ******************************************************************************
 * @brief   从报文中取出终端地址
 * @param[in]  *paddr : 返回终端地址
 * @param[in]  *p     : 输入报文
 *
 * @retval  1 : 不相同
 * @retval  0 :   相同
 ******************************************************************************
 */
static void
nw_addr_get(addr_t *paddr,
        const unsigned char *p)
{
    nw_header_t *ph = (nw_header_t *)p;

    memset(paddr, 0x00, sizeof(addr_t));
    memcpy(paddr, ph->deviceAddr, 6);   //6 bytes
}

/**
 ******************************************************************************
 * @brief   获取终端地址字符串
 * @param[in]  *paddr : 终端地址
 *
 * @retval  终端地址字符串
 ******************************************************************************
 */
static const char *
nw_addr_str(const addr_t *paddr)
{
    //910100-000100
    static char buf[14];

    sprintf(buf, "%02X%02X%02X-%02X%02X%02X",
            paddr->addr_c6[2], paddr->addr_c6[1], paddr->addr_c6[0],
            paddr->addr_c6[5], paddr->addr_c6[4], paddr->addr_c6[3]);
    return buf;
}

/**
 ******************************************************************************
 * @brief   主站MSA地址和报文中的主站MSA地址比较
 * @param[in]  *paddr : 输入主站MSA地址
 * @param[in]  *p     : 输入报文
 *
 * @retval  1 : 不相同
 * @retval  0 :   相同
 ******************************************************************************
 */
static int
nw_msa_cmp(const addr_t *paddr,
        const unsigned char *p)
{
    nw_header_t *ph = (nw_header_t *)p;

    return (ph->hostID == paddr->msa) ? 0 : 1;
}

/**
 ******************************************************************************
 * @brief   从报文中取出主站MSA地址
 * @param[in]  *paddr : 返回主站MSA地址
 * @param[in]  *p     : 输入报文
 *
 * @retval  1 : 不相同
 * @retval  0 :   相同
 ******************************************************************************
 */
static void
nw_msa_get(addr_t *paddr,
        const unsigned char *p)
{
    nw_header_t *ph = (nw_header_t *)p;

    memset(paddr, 0x00, sizeof(addr_t));
    paddr->msa = ph->hostID;
}

/**
 ******************************************************************************
 * @brief   判断主站发出的MSA是否有效
 * @param[in]  *paddr : 返回主站MSA地址
 *
 * @retval  1 : 有效
 * @retval  0 : 无效
 ******************************************************************************
 */
static int
nw_is_msa_valid(const addr_t *paddr)
{
    return (paddr->msa) ? 1 : 0;
}

static const ptcl_func_t the_nw_ptcl_func =
{
    "南网2013",
    0,
    nw_chkfrm_init,
    nw_chkfrm,
    nw_get_dir,
    nw_frame_type,
    nw_build_reply_packet,
    nw_addr_cmp,
    nw_addr_get,
    nw_addr_str,
    nw_msa_cmp,
    nw_msa_get,
    nw_is_msa_valid,
    NULL,
    NULL,
};
/**
 ******************************************************************************
 * @brief   获取国网协议处理接口
 * @retval  国网协议处理接口
 ******************************************************************************
 */
const ptcl_func_t *
nw_ptcl_func_get(void)
{
    return &the_nw_ptcl_func;
}
/*-------------------------------ptcl_gw.c-----------------------------------*/
