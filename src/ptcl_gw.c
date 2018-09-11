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
 
/**
 * 扩展协议:
 * A1(0)
 * A2(0)
 * AFN(0xFE)
 * FN(0)PN(0)查询终端最近通信经历的秒数
 *
 * 后台 --> 前置机格式:
 * 终端地址(4字节)
 * 68
 *
 * 前置机 --> 后台格式:
 * 终端地址(4字节)
 * 最近通信经历的秒数(4字节BIN)(若不在线，值为0xFFFFFFFF)
 * 湖南配变自动化系统主站系统访问前置机各终端状态的协议：
 *
 * 举例：
 *   主站下发：68 42 00 42 00 68 40 00 00 00 00 C8 FE 71 00 00 00 00 01 91 01 00 0A 16（代表查询91010001终端在线状态）
 *   终端返回一：68 52 00 52 00 68 C0 00 00 00 00 C8 FE 71 00 00 00 00 01 91 01 00 FF FF FF FF 86 16（代表91010001终端不在线）
 *   终端返回二：68 52 00 52 00 68 C0 00 00 00 00 C8 FE 71 00 00 00 00 01 91 01 00 10 00 00 00 9A 16（代表91010001终端最后一次和前置机通信的时间是16s前）
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
typedef struct FrameGBHeaderStruct
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
} gw_header_t;

//事件计数器结构
struct ECStruct
{
    unsigned char impEC;
    unsigned char norEC;
};

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

//AFN10_F1
struct TransparentSend
{
    unsigned char Comm;
};
#pragma pack(pop)



/*-----------------------------------------------------------------------------
 Section: Constant Definitions
 ----------------------------------------------------------------------------*/
#define CHKFRAME_TIMEOUT                5   /**< 报文超时时间默认10秒  */
#define FRAME_NO_DATA_LEN               14   /**< 无数据内容有数据标示的报文头长度 */

/* states for scanning incomming bytes from a bytestream */
#define GW_FRAME_STATES_NULL               0    /**< no synchronisation */
#define GW_FRAME_STATES_FLAG_FIR           1    /**< have the first Flag Byte received */
#define GW_FRAME_STATES_LEN1_1             2    /**< have the length byte */
#define GW_FRAME_STATES_LEN1_2             3    /**< have the length byte */
#define GW_FRAME_STATES_LEN2_1             4    /**< have the confirm length byte */
#define GW_FRAME_STATES_LEN2_2             5    /**< have the confirm length byte */
#define GW_FRAME_STATES_FLAG_SEC           6    /**< have the second Flag Byte received */
#define GW_FRAME_STATES_CONTROL            7    /**< have the control byte */
#define GW_FRAME_STATES_A3                 8    /**< have the A3 byte */
#define GW_FRAME_STATES_LINK_USER_DATA     9    /**< have the link user data bytes */
#define GW_FRAME_STATES_CS                 10   /**< wait for the CS */
#define GW_FRAME_STATES_END                11   /**< wait for the 16H */
#define GW_FRAME_STATES_COMPLETE           12   /**< complete frame */

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
gw_chkfrm_init(chkfrm_t *pchk,
        void (*pfn_frame_in)(void*, const unsigned char*, int))
{
    memset(pchk, 0x00, sizeof(chkfrm_t));
    pchk->frame_state = GW_FRAME_STATES_NULL;
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
gw_chkfrm(void *pc,
        chkfrm_t *pchk,
        const unsigned char *rxBuf,
        int rxLen)
{
    /* 如果已经完成的桢则重新开始 */
    if (pchk->frame_state == GW_FRAME_STATES_COMPLETE)
    {
        pchk->frame_state = GW_FRAME_STATES_NULL;
    }

    /* 如果超时则重新开始 */
    if (((time(NULL) - pchk->update_time) > pchk->overtime)
        || ((pchk->update_time - time(NULL)) > pchk->overtime))
    {
        pchk->frame_state = GW_FRAME_STATES_NULL;
    }

    while (rxLen > 0)
    {
        switch (pchk->frame_state)
        {
            case GW_FRAME_STATES_NULL:

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
                    pchk->frame_state = GW_FRAME_STATES_LEN1_1;
                    pchk->update_time = time(NULL);
                    pchk->dlen = 0;
                }
                break;

            case GW_FRAME_STATES_LEN1_1: /* 检测L1的低字节 */
                pchk->frame_state = GW_FRAME_STATES_LEN1_2;/* 为兼容主站不检测规约类型 */
                pchk->dlen = ((*rxBuf & 0xFCu) >> 2u);
                break;

            case GW_FRAME_STATES_LEN1_2: /* 检测L1的高字节 */
                pchk->dlen += ((unsigned int)*rxBuf << 6u);
                if (pchk->dlen > (the_max_frame_bytes - 8))
                {
                    free(pchk->pbuf);
                    pchk->pbuf = NULL;
                    pchk->frame_state = GW_FRAME_STATES_NULL;
                }
                else
                {
                    pchk->frame_state = GW_FRAME_STATES_LEN2_1;
                }
                break;

            case GW_FRAME_STATES_LEN2_1: /*检测L2的低字节*/
                pchk->frame_state = GW_FRAME_STATES_LEN2_2;
                pchk->cfm_len = ((*rxBuf & 0xFCu) >> 2u);
                break;

            case GW_FRAME_STATES_LEN2_2: /*检测L2的高字节*/
                pchk->cfm_len += ((unsigned int)*rxBuf << 6u);
                if (pchk->cfm_len == pchk->dlen)
                {
                    pchk->frame_state = GW_FRAME_STATES_FLAG_SEC;
                }
                else
                {
                    free(pchk->pbuf);
                    pchk->pbuf = NULL;
                    pchk->frame_state = GW_FRAME_STATES_NULL;
                }
                break;

            case GW_FRAME_STATES_FLAG_SEC:
                if (*rxBuf == 0x68)
                {
                    pchk->frame_state = GW_FRAME_STATES_CONTROL;
                }
                else
                {
                    free(pchk->pbuf);
                    pchk->pbuf = NULL;
                    pchk->frame_state = GW_FRAME_STATES_NULL;
                }
                break;

            case GW_FRAME_STATES_CONTROL:
                pchk->cs = *rxBuf;
                pchk->frame_state = GW_FRAME_STATES_A3;/* 不能检测方向，因为级联有上行报文 */
                break;

            case GW_FRAME_STATES_A3:
                pchk->cs += *rxBuf;
                if (pchk->pbuf_pos == 11)
                {
                    pchk->frame_state = GW_FRAME_STATES_LINK_USER_DATA;
                }
                break;

            case GW_FRAME_STATES_LINK_USER_DATA:
                pchk->cs += *rxBuf;
                if (pchk->pbuf_pos == (5 + pchk->dlen))
                {
                    pchk->frame_state = GW_FRAME_STATES_CS;
                }
                break;

            case GW_FRAME_STATES_CS:
                if (*rxBuf == pchk->cs)
                {
                    pchk->frame_state = GW_FRAME_STATES_END;
                }
                else
                {
                    free(pchk->pbuf);
                    pchk->pbuf = NULL;
                    pchk->frame_state = GW_FRAME_STATES_NULL;
                }
                break;

            case GW_FRAME_STATES_END:
                if (*rxBuf == 0x16)
                {
                    pchk->frame_state = GW_FRAME_STATES_COMPLETE;
                }
                else
                {
                    free(pchk->pbuf);
                    pchk->pbuf = NULL;
                    pchk->frame_state = GW_FRAME_STATES_NULL;
                }
                break;
            default:
                break;
        }

        if (pchk->frame_state != GW_FRAME_STATES_NULL)
        {
            pchk->pbuf[pchk->pbuf_pos] = *rxBuf;
            pchk->pbuf_pos++;
        }

        /* 完整报文，调用处理函数接口 */
        if (pchk->frame_state == GW_FRAME_STATES_COMPLETE)
        {
            if (pchk->pfn_frame_in)
            {
                pchk->pfn_frame_in(pc, pchk->pbuf, pchk->dlen + 8); //这里处理业务
            }

            free(pchk->pbuf);
            pchk->pbuf = NULL;
            pchk->frame_state = GW_FRAME_STATES_NULL;
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
gw_get_dir(const unsigned char* p)
{
    gw_header_t *ph = (gw_header_t *)p;

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
gw_frame_type(const unsigned char* p)
{
    gw_header_t *ph = (gw_header_t *)p;

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
    else if (ph->AFN == 0xFE)
    {
        if ((ph->lenArea == 0x42) && (!ph->logicaddr) && ((*(int*)&ph[1]) == 0))
        {
            return ONLINE;
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
gw_build_reply_packet(const unsigned char *p,
        unsigned char *po)
{
    gw_header_t *pin = (gw_header_t *)p;
    gw_header_t *psend = (gw_header_t *)po;
    int pos = sizeof(gw_header_t);

    psend->frameStart = 0x68;
    psend->lenArea = 0x48 | (pin->lenArea & 0x03);
    psend->lenArea_ = 0x48 | (pin->lenArea & 0x03);;
    psend->dataAreaStart = 0x68;
    psend->ctrlCodeArea = 0x0B;
    psend->logicaddr = pin->logicaddr;
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
gw_addr_cmp(const addr_t *paddr,
        const unsigned char *p)
{
    gw_header_t *ph = (gw_header_t *)p;

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
gw_addr_get(addr_t *paddr,
        const unsigned char *p)
{
    gw_header_t *ph = (gw_header_t *)p;

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
gw_addr_str(const addr_t *paddr)
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
gw_msa_cmp(const addr_t *paddr,
        const unsigned char *p)
{
    gw_header_t *ph = (gw_header_t *)p;

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
gw_msa_get(addr_t *paddr,
        const unsigned char *p)
{
    gw_header_t *ph = (gw_header_t *)p;

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
gw_is_msa_valid(const addr_t *paddr)
{
    return paddr->msa ? 1 : 0;
}

/**
 ******************************************************************************
 * @brief   获取主站发出查询是否在线终端地址
 * @param[out] *paddr : 返回主站MSA地址
 * @param[in]  *p     : 输入报文
 *
 * @retval  1 : 成功
 * @retval  0 : 失败
 ******************************************************************************
 */
static int
gw_get_oline_addr(addr_t *paddr,
        const unsigned char *p)
{
    gw_header_t *ph = (gw_header_t *)p;
    if (ph->lenArea == 0x42)
    {
        memset(paddr, 0x00, sizeof(addr_t));
        memcpy(paddr, &p[18], 4);
//        gw_addr_get(paddr, (unsigned char*)&p[1 + 4]);
        return 1;
    }
    return 0;
}

/**
 ******************************************************************************
 * @brief   创建查询online回复报文
 * @param[in]  *p  : 输入报文缓存
 * @param[in]  *po : 输出报文缓存
 * @param[in]  sec : 最近一次通信经历的秒数
 *
 * @return  输出报文长度
 ******************************************************************************
 */
static int
gw_build_online_packet(const unsigned char *p,
        unsigned char *po,
        int sec)
{
    gw_header_t *pin = (gw_header_t *)p;
    gw_header_t *psend = (gw_header_t *)po;
    int pos = sizeof(gw_header_t);

    psend->frameStart = 0x68;
    psend->lenArea = 0x52;
    psend->lenArea_ = 0x52;
    psend->dataAreaStart = 0x68;
    psend->ctrlCodeArea = 0xC0;
    psend->logicaddr = 0;
    psend->hostIDArea = pin->hostIDArea;
    psend->AFN = 0xFE;
    psend->TPV = 0;
    psend->FIR = 1;
    psend->FIN = 1; //单帧
    psend->CON = 0; //在所收到的报文中，CON位置“1”，表示需要对该帧报文进行确认；置“0”，表示不需要对该帧报文进行确认。
    psend->frameSeq = pin->frameSeq;
    memcpy(&po[pos + 8], &sec, 4);
    po[pos + 12] = get_cs(po + 6, psend->lenDataArea); //cs
    po[pos + 13] = 0x16;

    return pos + 14;
}

static const ptcl_func_t the_gw_ptcl_func =
{
    "国网1376.1",
    0,
    gw_chkfrm_init,
    gw_chkfrm,
    gw_get_dir,
    gw_frame_type,
    gw_build_reply_packet,
    gw_addr_cmp,
    gw_addr_get,
    gw_addr_str,
    gw_msa_cmp,
    gw_msa_get,
    gw_is_msa_valid,
    gw_get_oline_addr,
    gw_build_online_packet,
};
/**
 ******************************************************************************
 * @brief   获取国网协议处理接口
 * @retval  国网协议处理接口
 ******************************************************************************
 */
const ptcl_func_t *
gw_ptcl_func_get(void)
{
    return &the_gw_ptcl_func;
}
/*-------------------------------ptcl_gw.c-----------------------------------*/
