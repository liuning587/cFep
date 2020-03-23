/**
 ******************************************************************************
 * @file      ptcl_698.c
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
#include "maths.h"
#include "log.h"
#include "param.h"
#include "lib.h"
#include "ptcl.h"

/*-----------------------------------------------------------------------------
 Section: Type Definitions
 ----------------------------------------------------------------------------*/
#pragma pack(push, 1)
//报文头结构体
typedef struct Frame698HeaderStruct
{
    unsigned char flag;                 /**< 0x68   */

    /* 长度 */
    struct
    {
        unsigned short v: 14;              /**< 长度L BIN编码，除0x68和0x16外的字节数   */
        unsigned short re: 2;              /**< 保留   */
    } L;

    /* 功能码 */
    struct
    {
        unsigned char fc:  3;             /**< 功能码    */
        unsigned char re:  2;             /**< 保留   */
        unsigned char ft:  1;             /**< 0：完整APDU 1：APDU片断  */
        unsigned char prm: 1;             /**< 1：来自客户机 0：来自服务器    */
        unsigned char dir: 1;             /**< 0：客户机发出 1：服务器发出    */
    } C;

    /* 服务器地址域 */
    struct
    {
        unsigned char L:   4;             /**< 服务器地址长度  0~15表示1~16个字节  */
        unsigned char la:  2;             /**< 逻辑地址   */
        unsigned char typ: 2;             /**< 服务器地址类别 0-单地址 1-通配地址 2-组地址 3-广播地址*/
        unsigned char a[16];             /**< 单地址最大16Bytes; 通配地址最大16Bytes 广播地址1Bytes */
    }SA;

    unsigned char others[3];             /**< 其他 */
} p698_header_t;
#pragma pack(pop)

typedef struct
{
    unsigned char year[2];
    unsigned char month;
    unsigned char day_of_month;
    unsigned char day_of_week;
    unsigned char hh;
    unsigned char mm;
    unsigned char ss;
    unsigned short milliseconds;
}date_time_t;


/*-----------------------------------------------------------------------------
 Section: Constant Definitions
 ----------------------------------------------------------------------------*/
#define CHKFRAME_TIMEOUT                5   /**< 报文超时时间默认10秒  */
#define FRAME_NO_DATA_LEN               14   /**< 无数据内容有数据标示的报文头长度 */

#define P698_HEAD_LEN_WITHOUT_SA        7   /**< 除了服务器地址长度外的报文头长度 */

/* states for scanning incomming bytes from a bytestream */
#define P698_FRAME_STATES_NULL               0    /**< no synchronisation */
#define P698_FRAME_STATES_FLAG_FIR           1    /**< have the first Flag Byte received */
#define P698_FRAME_STATES_LEN1_1             2    /**< have the length byte */
#define P698_FRAME_STATES_LEN1_2             3    /**< have the length byte */
#define P698_FRAME_STATES_CONTROL            4    /**< have the confirm length byte */
#define P698_FRAME_STATES_AF                 5    /**< have the confirm length byte */
#define P698_FRAME_STATES_SA                 6    /**< have the second Flag Byte received */
#define P698_FRAME_STATES_CA                 7    /**< have the control byte */
#define P698_FRAME_STATES_HCS1               8    /**< have the A3 byte */
#define P698_FRAME_STATES_HCS2               9    /**< have the A3 byte */
#define P698_FRAME_STATES_LINK_USER_DATA     10    /**< have the link user data bytes */
#define P698_FRAME_STATES_FCS1               11   /**< wait for the CS */
#define P698_FRAME_STATES_FCS2               12   /**< wait for the CS */
#define P698_FRAME_STATES_END                13   /**< wait for the 16H */
#define P698_FRAME_STATES_COMPLETE           14   /**< complete frame */

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
p698_chkfrm_init(chkfrm_t *pchk,
        void (*pfn_frame_in)(void*, const unsigned char*, int))
{
    memset(pchk, 0x00, sizeof(chkfrm_t));
    pchk->frame_state = P698_FRAME_STATES_NULL;
    pchk->update_time = time(NULL);
    pchk->overtime = CHKFRAME_TIMEOUT;
    pchk->pfn_frame_in = pfn_frame_in;
}

/**
 ******************************************************************************
 * @brief   698报文检测
 * @param[in]  *pc      : 连接对象(fixme : 是否需要每次传入?)
 * @param[in]  *pchk    : 报文检测对象
 * @param[in]  *rxBuf   : 输入数据
 * @param[in]  rxLen    : 输入数据长度
 *
 * @return  None
 ******************************************************************************
 */
static void
p698_chkfrm(void *pc,
        chkfrm_t *pchk,
        const unsigned char *rxBuf,
        int rxLen)
{
    static unsigned char saLen = 0;

    /* 如果已经完成的桢则重新开始 */
    if (pchk->frame_state == P698_FRAME_STATES_COMPLETE)
    {
        pchk->frame_state = P698_FRAME_STATES_NULL;
    }

    /* 如果超时则重新开始 */
    if (((time(NULL) - pchk->update_time) > pchk->overtime)
        || ((pchk->update_time - time(NULL)) > pchk->overtime))
    {
        pchk->frame_state = P698_FRAME_STATES_NULL;
    }

    while (rxLen > 0)
    {
        switch (pchk->frame_state)
        {
            case P698_FRAME_STATES_NULL:

                if (*rxBuf == 0x68)
                {
                    if (!pchk->pbuf)
                    {
                        pchk->pbuf = malloc(the_max_frame_bytes);
                        if (!pchk->pbuf)
                        {
                            return;
                        }
                    }
                    pchk->pbuf_pos = 0;
                    pchk->frame_state = P698_FRAME_STATES_LEN1_1;
                    pchk->update_time = time(NULL);
                    pchk->dlen = 0;
                }
                break;

            case P698_FRAME_STATES_LEN1_1: /* 检测L1的低字节 */
                pchk->frame_state = P698_FRAME_STATES_LEN1_2;
                pchk->dlen = *rxBuf;
                break;

            case P698_FRAME_STATES_LEN1_2: /* 检测L1的高字节 */
                pchk->dlen += ((unsigned int)*rxBuf << 8u);
                if (pchk->dlen > (the_max_frame_bytes - 20))
                {
                    free(pchk->pbuf);
                    pchk->pbuf = NULL;
                    pchk->frame_state = P698_FRAME_STATES_NULL;
                }
                else
                {
                    pchk->frame_state = P698_FRAME_STATES_CONTROL;
                }
                break;

            case P698_FRAME_STATES_CONTROL:
                pchk->frame_state = P698_FRAME_STATES_AF;/* 不能检测方向，因为级联有上行报文 */
                break;

            case P698_FRAME_STATES_AF:
                saLen =  ((*rxBuf) & 0xf) + pchk->pbuf_pos + 1;
                pchk->frame_state = P698_FRAME_STATES_SA;
                break;

            case P698_FRAME_STATES_SA:
                if (pchk->pbuf_pos == saLen)
                {
                    pchk->frame_state = P698_FRAME_STATES_CA;
                }
                break;

            case P698_FRAME_STATES_CA:
                pchk->frame_state = P698_FRAME_STATES_HCS1;
                break;

            case P698_FRAME_STATES_HCS1:
                pchk->frame_state = P698_FRAME_STATES_HCS2;
                break;

            case P698_FRAME_STATES_HCS2:
                pchk->frame_state = P698_FRAME_STATES_LINK_USER_DATA;
                break;

            case P698_FRAME_STATES_LINK_USER_DATA:
                if (pchk->pbuf_pos == (pchk->dlen - 2))
                {
                    pchk->frame_state = P698_FRAME_STATES_FCS1;
                }
                break;

            case P698_FRAME_STATES_FCS1:
                pchk->frame_state = P698_FRAME_STATES_FCS2;
                break;

            case P698_FRAME_STATES_FCS2:
                pchk->frame_state = P698_FRAME_STATES_END;
                break;

            case P698_FRAME_STATES_END:
                if (*rxBuf == 0x16)
                {
                    pchk->frame_state = P698_FRAME_STATES_COMPLETE;
                }
                else
                {
                    free(pchk->pbuf);
                    pchk->pbuf = NULL;
                    pchk->frame_state = P698_FRAME_STATES_NULL;
                }
                break;

            default:
                break;
        }

        if (pchk->frame_state != P698_FRAME_STATES_NULL)
        {
            pchk->pbuf[pchk->pbuf_pos] = *rxBuf;
            pchk->pbuf_pos++;
        }

        /* 完整报文，调用处理函数接口 */
        if (pchk->frame_state == P698_FRAME_STATES_COMPLETE)
        {
            if (pchk->pfn_frame_in)
            {
                pchk->pfn_frame_in(pc, pchk->pbuf, pchk->dlen + 2); //这里处理业务
            }

            free(pchk->pbuf);
            pchk->pbuf = NULL;
            pchk->frame_state = P698_FRAME_STATES_NULL;
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
p698_get_dir(const unsigned char* p)
{
    p698_header_t *ph = (p698_header_t *)p;

    return ph->C.dir;
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
p698_frame_type(const unsigned char* p)
{
    p698_header_t *phead = (p698_header_t *)p;

    if (phead->C.fc == 0x01)
    {
        unsigned char *pbuf = (unsigned char *)phead;
        unsigned char login_type = pbuf[P698_HEAD_LEN_WITHOUT_SA + phead->SA.L + 2 + 2];
//        log_print(L_DEBUG, "phead->SA.L:%d\n", phead->SA.L);
        switch (login_type)
        {
            case 0:
                return LINK_LOGIN;
            case 1:
                return LINK_HAERTBEAT;
            case 2:
                return LINK_EXIT;
            default:
                log_print(L_DEBUG, "login_type err:%d\n", login_type);
                break;
        }
    }
    return OTHER;
}

/**
 ******************************************************************************
 * @brief   时间转换
 * @param[in]
 * @param[in]
 *
 * @return
 ******************************************************************************
 */
void
time_to_date_time_t(const time_t *value, void *data)
{
    short year = 0;
    struct tm daytime;
    time_t t;

    if ((value == NULL) || (*value == 0))
    {
        t = time(NULL);
    }
    else
    {
        t = *value;
    }

    date_time_t *pdata = (date_time_t*)data;
    daytime = *localtime(&t);
    pdata->milliseconds = 0;
    pdata->ss = daytime.tm_sec;
    pdata->mm = daytime.tm_min;
    pdata->hh = daytime.tm_hour;
    pdata->day_of_month = daytime.tm_mday;
    pdata->day_of_week = daytime.tm_wday;
    pdata->month = daytime.tm_mon + 1; /* 0...11 */

    year = 1900 + daytime.tm_year;
    pdata->year[0] = (year >> 8);
    pdata->year[1] = year;
}

/**
 ******************************************************************************
 * @brief   获取校验码
 * @param[in]
 * @param[in]
 *
 * @return
 ******************************************************************************
 */
unsigned short get_crc16(unsigned char *inbuf, int len)
{
    const unsigned fcstab[] =
    {   0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf, 0x8c48,
        0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7, 0x1081, 0x0108,
        0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e, 0x9cc9, 0x8d40, 0xbfdb,
        0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876, 0x2102, 0x308b, 0x0210, 0x1399,
        0x6726, 0x76af, 0x4434, 0x55bd, 0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e,
        0xfae7, 0xc87c, 0xd9f5, 0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e,
        0x54b5, 0x453c, 0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd,
        0xc974, 0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
        0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3, 0x5285,
        0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a, 0xdecd, 0xcf44,
        0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72, 0x6306, 0x728f, 0x4014,
        0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9, 0xef4e, 0xfec7, 0xcc5c, 0xddd5,
        0xa96a, 0xb8e3, 0x8a78, 0x9bf1, 0x7387, 0x620e, 0x5095, 0x411c, 0x35a3,
        0x242a, 0x16b1, 0x0738, 0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862,
        0x9af9, 0x8b70, 0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e,
        0xf0b7, 0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
        0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036, 0x18c1,
        0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e, 0xa50a, 0xb483,
        0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5, 0x2942, 0x38cb, 0x0a50,
        0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd, 0xb58b, 0xa402, 0x9699, 0x8710,
        0xf3af, 0xe226, 0xd0bd, 0xc134, 0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7,
        0x6e6e, 0x5cf5, 0x4d7c, 0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1,
        0xa33a, 0xb2b3, 0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72,
        0x3efb, 0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
        0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a, 0xe70e,
        0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1, 0x6b46, 0x7acf,
        0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9, 0xf78f, 0xe606, 0xd49d,
        0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330, 0x7bc7, 0x6a4e, 0x58d5, 0x495c,
        0x3de3, 0x2c6a, 0x1ef1, 0x0f78
    };
    unsigned short fcs = 0xffff;
    while (len--)
    {
        fcs = (unsigned short)((fcs >> 8) ^ fcstab[(fcs ^ *inbuf++) & 0xff]);
    }
    fcs ^= 0xffff;
    return fcs;
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
p698_build_reply_packet(const unsigned char *p,
        unsigned char *po)
{
    p698_header_t *pin = (p698_header_t *)p;
    p698_header_t *psend = (p698_header_t *)po;
    int offset = 0, crc16_h_pos, in_offset = 0;

    psend->flag = 0x68;
    unsigned char temp_c = 0x01;
    memcpy(&psend->C, &temp_c, 1);
    memcpy(&psend->SA, &pin->SA, pin->SA.L + 3);  //服务器地址、客户机地址
    offset += 4 + pin->SA.L + 3;    //起始1、长度2、控制域1、地址
    crc16_h_pos = offset;
    offset += 2;    //头校验


    po[offset++] = 0x81;
    po[offset++] = 0x00;
    po[offset++] = 0x00;

    in_offset = offset + 2; // 心跳周期

    memcpy(po + offset, p + in_offset, 10);
    offset += 10;
    time_to_date_time_t(NULL, &po[offset]);
    offset += 10;
    time_to_date_time_t(NULL, &po[offset]);
    offset += 10;

    psend->L.v = offset + 3 - 2;

    short crc16 = get_crc16(po + 1, crc16_h_pos - 1);
//    log_buf(L_DEBUG, "crc16h:", po + 1, crc16_h_pos - 1);
    po[crc16_h_pos] = crc16 & 0xff;
    po[crc16_h_pos + 1] = (crc16>>8) & 0xff;    //头校验

    crc16 = get_crc16(po + 1, offset - 1);
    po[offset++] = crc16 & 0xff;
    po[offset++] = (crc16>>8) & 0xff;    //帧校验

    po[offset++] = 0x16;
    return offset;
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
p698_addr_cmp(const addr_t *paddr,
        const unsigned char *p)
{
    p698_header_t *ph = (p698_header_t *)p;


    if ((ph->SA.a[0] == 0xAA) && (16 == ph->SA.L + 1))
    {
        return 0;
    }

    return memcmp(ph->SA.a, &paddr->addr_c16,
            ph->SA.L + 1 > sizeof(addr_t) ? sizeof(addr_t) : ph->SA.L + 1) == 0 ? 0 : 1;
}

/**
 ******************************************************************************
 * @brief   从报文中取出终端地址
 * @param[in]  *paddr : 返回终端地址
 * @param[in]  *p     : 输入报文
 *
 ******************************************************************************
 */
static void
p698_addr_get(addr_t *paddr,
        const unsigned char *p)
{
    p698_header_t *ph = (p698_header_t *)p;

    memset(paddr, 0xFF, sizeof(addr_t));
    memcpy(&paddr->addr, ph->SA.a, ph->SA.L + 1 > sizeof(*paddr) ? sizeof(*paddr) : ph->SA.L + 1);
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
p698_addr_str(const addr_t *paddr)
{
    static char buf[32];
    int count = 0;
    int addr_end_pos = sizeof(addr_t) - 1;
//    log_buf(L_DEBUG, "addr_t buf:", paddr->addr_c12, sizeof(*paddr));
    while(paddr->addr_c16[addr_end_pos] == 0xff){addr_end_pos -= 1;}
//    log_print(L_DEBUG, "addr_end_pos:%d\n", addr_end_pos);
    for (count = 0; count <= addr_end_pos; count++)
    {
        sprintf(&buf[count * 2], "%02X", paddr->addr_c16[addr_end_pos - count]);
    }
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
p698_msa_cmp(const addr_t *paddr,
        const unsigned char *p)
{
    p698_header_t *ph = (p698_header_t *)p;

    return *(ph->SA.a + ph->SA.L + 1) == paddr->msa ? 0 : 1;
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
p698_msa_get(addr_t *paddr,
        const unsigned char *p)
{
    p698_header_t *ph = (p698_header_t *)p;

    memset(paddr, 0x00, sizeof(addr_t));
    paddr->msa = *(ph->SA.a + ph->SA.L + 1);
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
p698_is_msa_valid(const addr_t *paddr)
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
p698_get_oline_addr(addr_t *paddr,
        const unsigned char *p)
{
    p698_header_t *ph = (p698_header_t *)p;
    memset(paddr, 0x00, sizeof(addr_t));
    memcpy(paddr, ph->SA.a, ph->SA.L);
    return 1;
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
p698_build_online_packet(const unsigned char *p,
        unsigned char *po,
        int sec)
{
    return 0;
}

static const ptcl_func_t the_698_ptcl_func =
{
    "698",
    0,
    p698_chkfrm_init,
    p698_chkfrm,
    p698_get_dir,
    p698_frame_type,
    p698_build_reply_packet,
    p698_addr_cmp,
    p698_addr_get,
    p698_addr_str,
    p698_msa_cmp,
    p698_msa_get,
    p698_is_msa_valid,
    p698_get_oline_addr,
    p698_build_online_packet,
};
/**
 ******************************************************************************
 * @brief   获取国网协议处理接口
 * @retval  国网协议处理接口
 ******************************************************************************
 */
const ptcl_func_t *
p698_ptcl_func_get(void)
{
    return &the_698_ptcl_func;
}
/*-------------------------------ptcl_gw.c-----------------------------------*/
