/**
 ******************************************************************************
 * @file      ptcl_zj.c
 * @brief     C Source file of ptcl_zj.c.
 * @details   This file including all API functions's 
 *            implement of ptcl_zj.c.
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
#include "param.h"
#include "lib.h"
#include "ptcl.h"

/*-----------------------------------------------------------------------------
 Section: Type Definitions
 ----------------------------------------------------------------------------*/
#pragma pack(push, 1)
/*  structure of normal frame from the Data Link Layer

    Flag          ( 1 Byte 68H)
    Rtua          ( 4 Byte rtu logical address )
    MSTA          ( 2 Byte master station address)
    Flag          ( 1 Byte 68H)
    Control       ( 1 Byte )
    Length        ( 2 Bytes)
    DATA          ( n Bytes, depend only from the length)
    CS            ( 1 Byte )
    Flag          ( 1 Byte 16H)
*/
//报文头结构体
typedef struct FrameZJHeaderStruct
{
    unsigned char frameStart;//0x68
    union
    {
        unsigned char deviceAddr[4];
        unsigned int logicaddr;//终端逻辑地址
        struct
        {
            unsigned char canton[2];
            unsigned short addr;
        };
    };
    union
    {
        struct
        {
            unsigned short int    hostID        :    6 ,
                                  frameSeq      :    7 ,
                                  seqInFrame    :    3 ;
        };
        unsigned short int hostIDArea;//D0--D5主站地址  D6--D12帧序号号 D13-D15帧内序
    };
    unsigned char dataAreaStart;//0x68
    union
    {
        struct
        {
            unsigned char    AFN                :    6 ,
                             exception          :    1 ,
                             DIR                :    1 ;
        };
        unsigned char ctrlCodeArea;//D0--D5功能码  D6异常标志 D7传输方向
    };
    unsigned short int lenDataArea;//帧头之后，到检验之前的数据域长度
} zj_header_t;

struct PSWStruct
{
    unsigned char level;
    unsigned char psw[3];
};
#pragma pack(pop)



/*-----------------------------------------------------------------------------
 Section: Constant Definitions
 ----------------------------------------------------------------------------*/
#define CHKFRAME_TIMEOUT                5   /**< 报文超时时间默认10秒  */
#define FRAME_NO_DATA_LEN               14   /**< 无数据内容有数据标示的报文头长度 */

/* states for scanning incomming bytes from a bytestream */
#define ZJ_FRAME_STATES_NULL               0    /**< no synchronisation */
#define ZJ_FRAME_STATES_FLAG_FIR           1    /**< 1帧起始符 */
#define ZJ_FRAME_STATES_RTUA               2    /**< 4终端逻辑地址 */
#define ZJ_FRAME_STATES_MSTA               3    /**< 2主站地址与命令序号 */
#define GW_FRAME_STATES_FLAG_SEC           4    /**< 0x16 */
#define ZJ_FRAME_STATES_CONTROL            5    /**< 1控制码 */
#define ZJ_FRAME_STATES_LEN1               6    /**< 数据长度1 */
#define ZJ_FRAME_STATES_LEN2               7    /**< 数据长度2 */
#define ZJ_FRAME_STATES_LINK_USER_DATA     8    /**< 数据域 */
#define ZJ_FRAME_STATES_CS                 9    /**< wait for the CS */
#define ZJ_FRAME_STATES_END                10   /**< wait for the 16H */
#define ZJ_FRAME_STATES_COMPLETE           11   /**< complete frame */

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
zj_chkfrm_init(chkfrm_t *pchk,
        void (*pfn_frame_in)(void*, const unsigned char*, int))
{
    memset(pchk, 0x00, sizeof(chkfrm_t));
    pchk->frame_state = ZJ_FRAME_STATES_NULL;
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
zj_chkfrm(void *pc,
        chkfrm_t *pchk,
        const unsigned char *rxBuf,
        int rxLen)
{
    /* 如果已经完成的桢则重新开始 */
    if (pchk->frame_state == ZJ_FRAME_STATES_COMPLETE)
    {
        pchk->frame_state = ZJ_FRAME_STATES_NULL;
    }

    /* 如果超时则重新开始 */
    if (((time(NULL) - pchk->update_time) > pchk->overtime)
        || ((pchk->update_time - time(NULL)) > pchk->overtime))
    {
        pchk->frame_state = ZJ_FRAME_STATES_NULL;
    }

    while (rxLen > 0)
    {
        switch (pchk->frame_state)
        {
            case ZJ_FRAME_STATES_NULL:
                if (*rxBuf == 0x68)
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
                    pchk->frame_state = ZJ_FRAME_STATES_RTUA;
                    pchk->update_time = time(NULL);
                    pchk->dlen = 0;
                    pchk->cs = 0x68;
                }
                break;

            case ZJ_FRAME_STATES_RTUA:
            case ZJ_FRAME_STATES_MSTA:
            case GW_FRAME_STATES_FLAG_SEC:
                pchk->cs += *rxBuf;
                if (pchk->pbuf_pos == 7)
                {
                    if (*rxBuf == 0x68)
                    {
                        pchk->frame_state = ZJ_FRAME_STATES_CONTROL;
                    }
                    else
                    {
                        free(pchk->pbuf);
                        pchk->pbuf = NULL;
                        pchk->frame_state = ZJ_FRAME_STATES_NULL;
                    }
                }
                break;

            case ZJ_FRAME_STATES_CONTROL:
                pchk->cs += *rxBuf;
                pchk->frame_state = ZJ_FRAME_STATES_LEN1;/* 不能检测方向，因为级联有上行报文 */
                break;

            case ZJ_FRAME_STATES_LEN1: /* 检测L1的低字节 */
                pchk->cs += *rxBuf;
                pchk->frame_state = ZJ_FRAME_STATES_LEN2;/* 为兼容主站不检测规约类型 */
                pchk->dlen = *rxBuf;
                break;

            case ZJ_FRAME_STATES_LEN2: /* 检测L1的高字节 */
                pchk->dlen += ((unsigned int)*rxBuf << 8u);
                if (pchk->dlen > (the_max_frame_bytes - 13)) //fixme:
                {
                    free(pchk->pbuf);
                    pchk->pbuf = NULL;
                    pchk->frame_state = ZJ_FRAME_STATES_NULL;
                }
                else
                {
                    if (pchk->dlen)
                    {
                        pchk->frame_state = ZJ_FRAME_STATES_LINK_USER_DATA;
                    }
                    else
                    {
                        pchk->frame_state = ZJ_FRAME_STATES_CS;
                    }
                }
                break;

            case ZJ_FRAME_STATES_LINK_USER_DATA:
                pchk->cs += *rxBuf;
                if (pchk->pbuf_pos == (10 + pchk->dlen))
                {
                    pchk->frame_state = ZJ_FRAME_STATES_CS;
                }
                break;

            case ZJ_FRAME_STATES_CS:
                if (*rxBuf == pchk->cs)
                {
                    pchk->frame_state = ZJ_FRAME_STATES_END;
                }
                else
                {
                    free(pchk->pbuf);
                    pchk->pbuf = NULL;
                    pchk->frame_state = ZJ_FRAME_STATES_NULL;
                }
                break;

            case ZJ_FRAME_STATES_END:
                if (*rxBuf == 0x16)
                {
                    pchk->frame_state = ZJ_FRAME_STATES_COMPLETE;
                }
                else
                {
                    free(pchk->pbuf);
                    pchk->pbuf = NULL;
                    pchk->frame_state = ZJ_FRAME_STATES_NULL;
                }
                break;

            default:
                break;
        }

        if (pchk->frame_state != ZJ_FRAME_STATES_NULL)
        {
            pchk->pbuf[pchk->pbuf_pos] = *rxBuf;
            pchk->pbuf_pos++;
        }

        /* 完整报文，调用处理函数接口 */
        if (pchk->frame_state == ZJ_FRAME_STATES_COMPLETE)
        {
            if (pchk->pfn_frame_in)
            {
                pchk->pfn_frame_in(pc, pchk->pbuf, pchk->pbuf_pos); //这里处理业务
            }

            free(pchk->pbuf);
            pchk->pbuf = NULL;
            pchk->frame_state = ZJ_FRAME_STATES_NULL;
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
zj_get_dir(const unsigned char* p)
{
    zj_header_t *ph = (zj_header_t *)p;

    return ph->DIR;
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
zj_frame_type(const unsigned char* p)
{
    zj_header_t *ph = (zj_header_t *)p;

    switch (ph->AFN)
    {
        case 0x21:
            if ((ph->lenDataArea == 3) || (ph->lenDataArea == 8))
            {
                return LINK_LOGIN;
            }
            break;

        case 0x22:
            if (!ph->lenDataArea)
            {
                return LINK_EXIT;
            }
            break;

        case 0x24:
            if (!ph->lenDataArea)
            {
                return LINK_HAERTBEAT;
            }
            break;

        default:
            break;
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
zj_build_reply_packet(const unsigned char *p,
        unsigned char *po)
{
    zj_header_t *pin = (zj_header_t *)p;
    zj_header_t *psend = (zj_header_t *)po;
    int pos = sizeof(zj_header_t);

    psend->frameStart = 0x68;
    psend->logicaddr = pin->logicaddr;
    psend->seqInFrame = 0;
    psend->frameSeq = pin->frameSeq;
    psend->hostIDArea = 40;

    psend->dataAreaStart = 0x68;

    psend->ctrlCodeArea = pin->ctrlCodeArea;
    psend->exception = 0;
    psend->DIR = 0; //0 : 主站-->终端

    psend->lenDataArea = 0;

    po[pos + 0] = get_cs(po, psend->lenDataArea + 11); //cs;
    po[pos + 1] = 0x16;

    return pos + 2;
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
zj_addr_cmp(const addr_t *paddr,
        const unsigned char *p)
{
    zj_header_t *ph = (zj_header_t *)p;

    return (ph->logicaddr == paddr->addr) ? 0 : 1;
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
zj_addr_get(addr_t *paddr,
        const unsigned char *p)
{
    zj_header_t *ph = (zj_header_t *)p;

    memset(paddr, 0x00, sizeof(addr_t));
    paddr->addr = ph->logicaddr;
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
zj_addr_str(const addr_t *paddr)
{
    //9101-0001
    static char buf[10];

    sprintf(buf, "%02X%02X-%02X%02X", paddr->addr_c4[1], paddr->addr_c4[0],
            paddr->addr_c4[3], paddr->addr_c4[2]);
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
zj_msa_cmp(const addr_t *paddr,
        const unsigned char *p)
{
    zj_header_t *ph = (zj_header_t *)p;

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
zj_msa_get(addr_t *paddr,
        const unsigned char *p)
{
    zj_header_t *ph = (zj_header_t *)p;

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
zj_is_msa_valid(const addr_t *paddr)
{
    return (paddr->msa) ? 1 : 0;
}

static const ptcl_func_t the_zj_ptcl_func =
{
    "广东、浙江",
    1,
    zj_chkfrm_init,
    zj_chkfrm,
    zj_get_dir,
    zj_frame_type,
    zj_build_reply_packet,
    zj_addr_cmp,
    zj_addr_get,
    zj_addr_str,
    zj_msa_cmp,
    zj_msa_get,
    zj_is_msa_valid,
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
zj_ptcl_func_get(void)
{
    return &the_zj_ptcl_func;
}
/*-------------------------------ptcl_zj.c-----------------------------------*/
