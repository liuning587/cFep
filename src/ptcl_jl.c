/**
 ******************************************************************************
 * @file      ptcl_jl.c
 * @brief     C Source file of ptcl_jl.c.
 * @details   This file including all API functions's 
 *            implement of ptcl_jl.c.
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
//报文头结构体
typedef struct FrameJLHeaderStruct
{
    unsigned char frameStart;       //0x68
    union
    {
        struct
        {
            unsigned short  ptclFlag    :   2   ,   //D0--D1规约标示
                                                    //D0=0、D1=0，表示：禁用；
                                                    //D0=1、D1=0，表示：本规约使用；
                                                    //D0=0或1、D1=1，为保留。
                            lenDataArea :   14  ;   //D2--D15长度（控制域、地址域、链路用户数据长度总数）
        };
        unsigned short      lenArea;
    };
    unsigned short  lenArea_;       //重复的长度域
    unsigned char   dataAreaStart;  //0x68
    union
    {
        struct
        {
            unsigned char   FuncCode : 4 ,  //D0--D3控制域功能码
                            FCV      : 1 ,  //D4帧计数有效位
                            FCB_ACD  : 1 ,  //D5帧计数位 或者 请求访问位
                            PRM      : 1 ,  //D6启动标志位
                            DIR      : 1 ;  //D7传输方向位
        };
        unsigned char ctrlCodeArea;
    };
    union
    {
        unsigned char deviceAddr[6];//终端逻辑地址
    };
    union
    {
        struct
        {
            unsigned char   groupAddr   :    1 ,//D0 终端组地址
                            hostID      :    7 ;//D1-D7主站地址
        };
        unsigned char hostIDArea;
    };
    unsigned char AFN;
    union
    {
        struct
        {
            unsigned char    frameSeq   :    4 ,//D0--D3帧序号
                                CON     :    1 ,//D4请求确认标志位
                                FIN     :    1 ,//D5结束帧标志
                                FIR     :    1 ,//D6首帧标志
                                TPV     :    1 ;//D7帧时间标签有效位
        };
        unsigned char seqArea;
    };
} jl_header_t;

#pragma pack(pop)

/*-----------------------------------------------------------------------------
 Section: Constant Definitions
 ----------------------------------------------------------------------------*/
#define CHKFRAME_TIMEOUT                5   /**< 报文超时时间默认10秒  */
#define FRAME_NO_DATA_LEN               14   /**< 无数据内容有数据标示的报文头长度 */

/* states for scanning incomming bytes from a bytestream */
#define JL_FRAME_STATES_NULL               0    /**< no synchronisation */
#define JL_FRAME_STATES_FLAG_FIR           1    /**< have the first Flag Byte received */
#define JL_FRAME_STATES_LEN1_1             2    /**< have the length byte */
#define JL_FRAME_STATES_LEN1_2             3    /**< have the length byte */
#define JL_FRAME_STATES_LEN2_1             4    /**< have the confirm length byte */
#define JL_FRAME_STATES_LEN2_2             5    /**< have the confirm length byte */
#define JL_FRAME_STATES_FLAG_SEC           6    /**< have the second Flag Byte received */
#define JL_FRAME_STATES_CONTROL            7    /**< have the control byte */
#define JL_FRAME_STATES_A3                 8    /**< have the A3 byte */
#define JL_FRAME_STATES_LINK_USER_DATA     9    /**< have the link user data bytes */
#define JL_FRAME_STATES_CS                 10   /**< wait for the CS */
#define JL_FRAME_STATES_END                11   /**< wait for the 16H */
#define JL_FRAME_STATES_COMPLETE           12   /**< complete frame */

#define GB_PRPTOCOL_TYPE                0x01 /**< 2005规约标识 */
#define GW_PRPTOCOL_TYPE                0x02 /**< 国网规约标识 */

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
jl_chkfrm_init(chkfrm_t *pchk,
        void (*pfn_frame_in)(void*, const unsigned char*, int))
{
    memset(pchk, 0x00, sizeof(chkfrm_t));
    pchk->frame_state = JL_FRAME_STATES_NULL;
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
jl_chkfrm(void *pc,
        chkfrm_t *pchk,
        const unsigned char *rxBuf,
        int rxLen)
{
    /* 如果已经完成的桢则重新开始 */
    if (pchk->frame_state == JL_FRAME_STATES_COMPLETE)
    {
        pchk->frame_state = JL_FRAME_STATES_NULL;
    }

    /* 如果超时则重新开始 */
    if (((time(NULL) - pchk->update_time) > pchk->overtime)
        || ((pchk->update_time - time(NULL)) > pchk->overtime))
    {
        pchk->frame_state = JL_FRAME_STATES_NULL;
    }

    while (rxLen > 0)
    {
        switch (pchk->frame_state)
        {
            case JL_FRAME_STATES_NULL:

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
                    pchk->frame_state = JL_FRAME_STATES_LEN1_1;
                    pchk->update_time = time(NULL);
                    pchk->dlen = 0;
                }
                break;

            case JL_FRAME_STATES_LEN1_1: /* 检测L1的低字节 */
                pchk->frame_state = JL_FRAME_STATES_LEN1_2;/* 为兼容主站不检测规约类型 */
                pchk->dlen = ((*rxBuf & 0xFCu) >> 2u);
                break;

            case JL_FRAME_STATES_LEN1_2: /* 检测L1的高字节 */
                pchk->dlen += ((unsigned int)*rxBuf << 6u);
                if (pchk->dlen > (the_max_frame_bytes - 8))
                {
                    free(pchk->pbuf);
                    pchk->pbuf = NULL;
                    pchk->frame_state = JL_FRAME_STATES_NULL;
                }
                else
                {
                    pchk->frame_state = JL_FRAME_STATES_LEN2_1;
                }
                break;

            case JL_FRAME_STATES_LEN2_1: /*检测L2的低字节*/
                pchk->frame_state = JL_FRAME_STATES_LEN2_2;
                pchk->cfm_len = ((*rxBuf & 0xFCu) >> 2u);
                break;

            case JL_FRAME_STATES_LEN2_2: /*检测L2的高字节*/
                pchk->cfm_len += ((unsigned int)*rxBuf << 6u);
                if (pchk->cfm_len == pchk->dlen)
                {
                    pchk->frame_state = JL_FRAME_STATES_FLAG_SEC;
                }
                else
                {
                    free(pchk->pbuf);
                    pchk->pbuf = NULL;
                    pchk->frame_state = JL_FRAME_STATES_NULL;
                }
                break;

            case JL_FRAME_STATES_FLAG_SEC:
                if (*rxBuf == 0x68)
                {
                    pchk->frame_state = JL_FRAME_STATES_CONTROL;
                }
                else
                {
                    free(pchk->pbuf);
                    pchk->pbuf = NULL;
                    pchk->frame_state = JL_FRAME_STATES_NULL;
                }
                break;

            case JL_FRAME_STATES_CONTROL:
                pchk->cs = *rxBuf;
                pchk->frame_state = JL_FRAME_STATES_A3;/* 不能检测方向，因为级联有上行报文 */
                break;

            case JL_FRAME_STATES_A3:
                pchk->cs += *rxBuf;
                if (pchk->pbuf_pos == 13)
                {
                    pchk->frame_state = JL_FRAME_STATES_LINK_USER_DATA;
                }
                break;

            case JL_FRAME_STATES_LINK_USER_DATA:
                pchk->cs += *rxBuf;
                if (pchk->pbuf_pos == (5 + pchk->dlen))
                {
                    pchk->frame_state = JL_FRAME_STATES_CS;
                }
                break;

            case JL_FRAME_STATES_CS:
                if (*rxBuf == pchk->cs)
                {
                    pchk->frame_state = JL_FRAME_STATES_END;
                }
                else
                {
                    free(pchk->pbuf);
                    pchk->pbuf = NULL;
                    pchk->frame_state = JL_FRAME_STATES_NULL;
                }
                break;

            case JL_FRAME_STATES_END:
                if (*rxBuf == 0x16)
                {
                    pchk->frame_state = JL_FRAME_STATES_COMPLETE;
                }
                else
                {
                    free(pchk->pbuf);
                    pchk->pbuf = NULL;
                    pchk->frame_state = JL_FRAME_STATES_NULL;
                }
                break;
            default:
                break;
        }

        if (pchk->frame_state != JL_FRAME_STATES_NULL)
        {
            pchk->pbuf[pchk->pbuf_pos] = *rxBuf;
            pchk->pbuf_pos++;
        }

        /* 完整报文，调用处理函数接口 */
        if (pchk->frame_state == JL_FRAME_STATES_COMPLETE)
        {
            if (pchk->pfn_frame_in)
            {
                pchk->pfn_frame_in(pc, pchk->pbuf, pchk->dlen + 8); //这里处理业务
            }

            free(pchk->pbuf);
            pchk->pbuf = NULL;
            pchk->frame_state = JL_FRAME_STATES_NULL;
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
jl_get_dir(const unsigned char* p)
{
    jl_header_t *ph = (jl_header_t *)p;

    return ph->DIR;
}

/**
 ******************************************************************************
 * @brief   根据pnfn偏移获取fn
 * @param[in]  输入2字节fn
 * @retval  fn
 ******************************************************************************
 */
static unsigned short
getPnFn(const unsigned char* pFnPn)
{
    int i;
    unsigned short nFn;
    nFn = pFnPn[3]*8;

    for(i = 0; i < 8; i++)
    {
        if( ((pFnPn[2] >> i) & 0x01) == 0x01)
        {
            nFn += i+1;
        }
    }
    return nFn;
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
jl_frame_type(const unsigned char* p)
{
    jl_header_t *ph = (jl_header_t *)p;

    if (ph->AFN == 0x02) //
    {
        if (ph->lenArea >= 4)
        {
            switch (getPnFn((unsigned char*)&ph[1]))
            {
            case 1:
                return LINK_LOGIN;
            case 2:
                return LINK_EXIT;
            case 3:
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
jl_build_reply_packet(const unsigned char *p,
        unsigned char *po)
{
    jl_header_t *pin = (jl_header_t *)p;
    jl_header_t *psend = (jl_header_t *)po;
    int pos = sizeof(jl_header_t);

    psend->frameStart = 0x68;
    psend->lenArea = 0x52;
    psend->lenArea_ = 0x52;
    psend->dataAreaStart = 0x68;
    psend->ctrlCodeArea = 0x0B;
    memcpy(psend->deviceAddr, pin->deviceAddr, 6);
    psend->hostIDArea = 2; //msa = 2
    psend->AFN = 0;
    psend->TPV = 0;
    psend->FIR = 1;
    psend->FIN = 1; //单帧
    psend->CON = 0; //在所收到的报文中，CON位置“1”，表示需要对该帧报文进行确认；置“0”，表示不需要对该帧报文进行确认。
    psend->frameSeq = pin->frameSeq;
    po[pos + 0] = 0x00;
    po[pos + 1] = 0x00;
    po[pos + 2] = 0x04;
    po[pos + 3] = 0x00;
    po[pos + 4] = 0x02;   //确认afn
    memcpy(&po[pos + 5], &pin[1], 4);
    po[pos + 9] = 0;
    po[pos + 10] = get_cs(po + 6, psend->lenDataArea); //cs
    po[pos + 11] = 0x16;

    return pos + 12;
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
jl_addr_cmp(const addr_t *paddr,
        const unsigned char *p)
{
    jl_header_t *ph = (jl_header_t *)p;

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
jl_addr_get(addr_t *paddr,
        const unsigned char *p)
{
    jl_header_t *ph = (jl_header_t *)p;

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
jl_addr_str(const addr_t *paddr)
{
    //9101-00000001
    static char buf[16];

    snprintf(buf, sizeof(buf), "%02X%02X-%02X%02X%02X%02X",
            paddr->addr_c6[1], paddr->addr_c6[0],
            paddr->addr_c6[5], paddr->addr_c6[4],
            paddr->addr_c6[3], paddr->addr_c6[2]);
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
jl_msa_cmp(const addr_t *paddr,
        const unsigned char *p)
{
    jl_header_t *ph = (jl_header_t *)p;

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
jl_msa_get(addr_t *paddr,
        const unsigned char *p)
{
    jl_header_t *ph = (jl_header_t *)p;

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
jl_is_msa_valid(const addr_t *paddr)
{
    return (paddr->msa) ? 1 : 0;
}

static const ptcl_func_t the_jl_ptcl_func =
{
    "吉林规约",
    0,
    jl_chkfrm_init,
    jl_chkfrm,
    jl_get_dir,
    jl_frame_type,
    jl_build_reply_packet,
    jl_addr_cmp,
    jl_addr_get,
    jl_addr_str,
    jl_msa_cmp,
    jl_msa_get,
    jl_is_msa_valid,
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
jl_ptcl_func_get(void)
{
    return &the_jl_ptcl_func;
}
/*-------------------------------ptcl_gw.c-----------------------------------*/
