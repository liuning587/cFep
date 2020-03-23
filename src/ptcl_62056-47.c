/**
 ******************************************************************************
 * @file      ptcl_62056-47.c
 * @brief     C Source file of ptcl_62056-47.c.
 * @details   This file including all API functions's 
 *            implement of ptcl_62056-47.c.
 *
 * @copyright 
 ******************************************************************************
 */
 
/**
 * 集中器登录、心跳报文
 * 00 01 FF FF 00 11 00 14 0C 41 55 58 30 30 30 30 30 30 30 30 36 35 35 33 35 0C 00 05
 *
 * 00 01 //协议版本
 * FF FF //源端口
 * 00 11 //目的
 * 00 14 //apdu长度
 *
 * 0C //ctrl
 * 41 55 58 30 30 30 30 30 30 30 30 36 35 35 33 35 //逻辑设备名16bytes
 *
 * 0C 00 //载波编号
 * 05 //心跳时间
 * =========================================
 * 主站确认
 * 00 01 FF FF 00 0D 00 01 0C
 *
=========================================
招测gprs参数

1. 后台发送AARQ
原始报文
00 01 FF FF FF FF 00 38 60 36 A1 09 06 07 60 85 74 05 08 01 01 8A 02 07 80 8B 07 60
85 74 05 08 02 01 AC 0A 80 08 31 32 33 34 35 36 37 38 BE 10 04 0E 01 00 00 00 06 5F
1F 04 00 00 7E 1F 04 B0

解析
00 01 FF FF FF FF 00 38

60 //AARQ
36 //LEN

A1 // application-context-name
09 // len
  06 // [UNIVERSAL 6] OBJECT IDENTIFIER
  07 // len
     60 85 74 05 08 01 01

8A 02 07 80 //ACSE-requirements

8B 07 //mechanism-name
   60 85 74 05 08 02 01

AC 0A //Authentication-value
   80 08
      31 32 33 34 35 36 37 38   //密码12345678

BE 10 //Association-information
    04 //[UNIVERSAL 4] OCTET STRING
    0E // len 15
    01 00 00 00 06 5F 1F 04 00 00 7E 1F


04 B0 //PDU-Size  0x04*256 + 0xb0 = 1200字节


==================================================
AARE
原始报文
00 01 FF FF FF FF 00 44 61 42 A1 09 06 07 60 85 74 05 08 01 01 A2 03 02 01 00 A3 05
A1 03 02 01 00 88 02 07 80 89 07 60 85 74 05 08 02 01 AA 0A 80 08 31 32 33 34 35 36
37 38 BE 10 04 0E 08 00 06 5F 1F 04 00 00 18 1F 02 00 00 07

解析
00 01 FF FF FF FF 00 44

61
42

A1 09 06 07 60 85 74 05 08 01 01

A2 // result
03 len
   02 // [UNIVERSAL 2] INTEGER
   01 // len
   00 // accepted

A3 // result-source-diagnostic
05 // len
   A1 // Associate-source-diagnostic
   03 // len
      02 //[UNIVERSAL 2] INTEGER
      01 //len
      00 //null  (acse-service-user)


88 //ACSE-requirements
02 //len
   07 80

89 //Mechanism-name
07 //len
60 85 74 05 08 02 01

AA //Authentication-value
0A //len 10
   80 //GraphicString
   08 //len
      31 32 33 34 35 36 37 38 //密码12345678

BE //30 user-information
10 //len 16
    04 0E 08 00 06 5F 1F 04 00 00 18 1F
    02 00 //pdu size: 512
    00 07 //

==========================================
主站请求GPRS参数
原始报文
00 01 FF FF 00 11 00 0D C0 01 81 00 01 00 00 19 04 80 FF 02 00

解析
00 01 FF FF 00 11 00 0D

C0 // GET-Request
01 // Get-Request-Normal
{
  81 // Invoke-Id-And-Priority
  {
    00 01 //Cosem-Class-Id
    00 00 19 04 80 FF //Cosem-Object-Instance-Id
    02 //Cosem-Object-Attribute-Id
  }
  00  // 无 Selective-Access-Descriptor
}

接收:2015-09-30 14:01:59:253
原始报文
00 01 00 11 FF FF 00 F4 C4 01 81 00 02 14 11 01 04 01 00 09 04
79 28 50 9F 09 02 1C 20 09 04 0A 02 ED 87 09 02 22 B8 09 04 FF
FF FF 00 09 02 1F 44 09 04 DE B8 ED 0B 09 02 1F 44 0A 32 68 74
74 70 3A 2F 2F 6F 61 2E 61 75 78 67 72 6F 75 70 2E 63 6F 6D 00
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 09 04 DE DE DE A8 09 04 C0 A8 60 01 0A 21 43
4D 4E 45 54 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00 00 0A 21 43 4D 4E 45 54 00 00 00
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
00 00 00 00 0A 21 43 4D 4E 45 54 00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 0A 10 2A
39 39 2A 2A 2A 31 23 00 00 00 00 00 00 00 00 11 05 11 0A 11 05

解析
00 01 00 11 FF FF 00 F4

C4 //GET-Response
01 //Get-Response-Normal
{
  81 // Invoke-Id-And-Priority
  00 //Data
  02 14 //struct 12 members
  {
     11 01 Unsigned8
     04 01 00
     09 04 79 28 50 9F
     09 02 1C 20
     09 04 0A 02 ED 87
     09 02 22 B8
     09 04 FF FF FF 00
     09 02 1F 44
     09 04 DE B8 ED 0B
     09 02 1F 44
     0A 32 68 74 74 70 3A 2F 2F 6F 61 2E 61 75 78 67 72 6F 75 70 2E 63 6F 6D 00 00 00 00 00 00 00 00 00 00 00
     00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
     09 04 DE DE DE A8
     09 04 C0 A8 60 01
     0A 21 43 4D 4E 45 54 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
     0A 21 43 4D 4E 45 54 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
     0A 21 43 4D 4E 45 54 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
     0A 10 2A 39 39 2A 2A 2A 31 23 00 00 00 00 00 00 00 00
     11 05
     11 0A
     11 05
  }
}
 *
 */
/*-----------------------------------------------------------------------------
 Section: Includes
 ----------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "maths.h"
#include "param.h"
#include "lib.h"
#include "ptcl.h"

/*-----------------------------------------------------------------------------
 Section: Type Definitions
 ----------------------------------------------------------------------------*/
#pragma pack(push, 1)
//报文头结构体
typedef struct
{
    unsigned char version0;  //0x00
    unsigned char version1;  //0x01

    unsigned char src0;
    unsigned char src1;

    unsigned char dst0;
    unsigned char dst1;

    unsigned char len0;
    unsigned char len1;

    unsigned char AFN;  //0A: 登录 AA: 登录确认 0C: 心跳 CC: 心跳确认 Other:62056
    unsigned char type; //1: 集中器 2: 表 3: 主站
} p47_header_t;
#pragma pack(pop)

/*-----------------------------------------------------------------------------
 Section: Constant Definitions
 ----------------------------------------------------------------------------*/
#define CHKFRAME_TIMEOUT                5   /**< 报文超时时间默认10秒  */
#define FRAME_NO_DATA_LEN               14  /**< 无数据内容有数据标示的报文头长度 */

/* states for scanning incomming bytes from a bytestream */
#define P47_FRAME_STATES_NULL           0   /**< no synchronisation */
#define P47_FRAME_STATES_VERSION        1   /**< 协议版本 */
#define P47_FRAME_STATES_SRC_PORT0      2   /**< 源端口,对应主站地址,区分上位机 */
#define P47_FRAME_STATES_SRC_PORT1      3   /**< 源端口,对应主站地址,区分上位机 */
#define P47_FRAME_STATES_DST_PORT0      4   /**< 目的端口,1管理逻辑设备 */
#define P47_FRAME_STATES_DST_PORT1      5   /**< 目的端口,1管理逻辑设备 */
#define P47_FRAME_STATES_LEN_1          6   /**< have the length byte */
#define P47_FRAME_STATES_LEN_2          7   /**< have the length byte */
#define P47_FRAME_STATES_AFN            8   /**< AFN */
#define P47_FRAME_STATES_DATA           9   /**< DATA */

#define GW_FRAME_STATES_COMPLETE        10  /**< complete frame */

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
 * @brief   62056-47报文检测初始化
 * @param[in]  *pchk         : 报文检测对象
 * @param[in]  *pfn_frame_in : 当收到合法报文后执行的回调函数
 *
 * @return  None
 ******************************************************************************
 */
static void
p47_chkfrm_init(chkfrm_t *pchk,
        void (*pfn_frame_in)(void*, const unsigned char*, int))
{
    memset(pchk, 0x00, sizeof(chkfrm_t));
    pchk->frame_state = P47_FRAME_STATES_NULL;
    pchk->update_time = time(NULL);
    pchk->overtime = CHKFRAME_TIMEOUT;
    pchk->pfn_frame_in = pfn_frame_in;
}

/**
 ******************************************************************************
 * @brief   62056-47报文检测
 * @param[in]  *pc      : 连接对象(fixme : 是否需要每次传入?)
 * @param[in]  *pchk    : 报文检测对象
 * @param[in]  *rxBuf   : 输入数据
 * @param[in]  rxLen    : 输入数据长度
 *
 * @return  None
 ******************************************************************************
 */
static void
p47_chkfrm(void *pc,
        chkfrm_t *pchk,
        const unsigned char *rxBuf,
        int rxLen)
{
    /* 如果已经完成的桢则重新开始 */
    if (pchk->frame_state == GW_FRAME_STATES_COMPLETE)
    {
        pchk->frame_state = P47_FRAME_STATES_NULL;
    }

    /* 如果超时则重新开始 */
    if (((time(NULL) - pchk->update_time) > pchk->overtime)
        || ((pchk->update_time - time(NULL)) > pchk->overtime))
    {
        pchk->frame_state = P47_FRAME_STATES_NULL;
    }

    while (rxLen > 0)
    {
        switch (pchk->frame_state)
        {
            case P47_FRAME_STATES_NULL:
                if (*rxBuf == 0x00)
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
                    pchk->frame_state = P47_FRAME_STATES_VERSION;
                    pchk->update_time = time(NULL);
                    pchk->dlen = 0;
                }
                break;

            case P47_FRAME_STATES_VERSION:
                if (*rxBuf == 0x01)
                {
                    pchk->frame_state = P47_FRAME_STATES_SRC_PORT0;
                }
                else
                {
                    free(pchk->pbuf);
                    pchk->pbuf = NULL;
                    pchk->frame_state = P47_FRAME_STATES_NULL;
                }
                break;

            case P47_FRAME_STATES_SRC_PORT0:
                pchk->frame_state = P47_FRAME_STATES_SRC_PORT1;
                break;

            case P47_FRAME_STATES_SRC_PORT1:
                pchk->frame_state = P47_FRAME_STATES_DST_PORT0;
                break;

            case P47_FRAME_STATES_DST_PORT0:
                pchk->frame_state = P47_FRAME_STATES_DST_PORT1;
                break;

            case P47_FRAME_STATES_DST_PORT1:
                pchk->frame_state = P47_FRAME_STATES_LEN_1;
                break;

            case P47_FRAME_STATES_LEN_1: /* 检测L1的低字节 */
                pchk->frame_state = P47_FRAME_STATES_LEN_2;/* 为兼容主站不检测规约类型 */
                pchk->dlen = *rxBuf << 8;
                break;

            case P47_FRAME_STATES_LEN_2: /* 检测L1的高字节 */
                pchk->dlen += ((unsigned int)*rxBuf);
                if (pchk->dlen > (the_max_frame_bytes - 8)) //fixme: 8
                {
                    free(pchk->pbuf);
                    pchk->pbuf = NULL;
                    pchk->frame_state = P47_FRAME_STATES_NULL;
                }
                else
                {
                    pchk->frame_state = P47_FRAME_STATES_DATA;
                }
                break;

            case P47_FRAME_STATES_DATA:
                pchk->cs += *rxBuf;
                if (pchk->pbuf_pos == (7 + pchk->dlen))
                {
                    pchk->frame_state = GW_FRAME_STATES_COMPLETE;
                }
                break;

            default:
                pchk->frame_state = P47_FRAME_STATES_NULL;
                break;
        }

        if (pchk->frame_state != P47_FRAME_STATES_NULL)
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
            pchk->frame_state = P47_FRAME_STATES_NULL;
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
p47_get_dir(const unsigned char* p)
{
    p47_header_t *ph = (p47_header_t *)p;

    return ((ph->type == 1) || (ph->type == 2)) ? 1 : 0;
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
p47_frame_type(const unsigned char* p)
{
    p47_header_t *ph = (p47_header_t *)p;

    switch (ph->AFN)
    {
        case 0x0A:
            return LINK_LOGIN;

        case 0x0C:
            return LINK_HAERTBEAT;

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
p47_build_reply_packet(const unsigned char *p,
        unsigned char *po)
{
    p47_header_t *pin = (p47_header_t *)p;
    p47_header_t *psend = (p47_header_t *)po;
    int pos = sizeof(p47_header_t);
    unsigned short crc;
    int len;

    psend->version0 = 0x00;
    psend->version1 = 0x01;
    psend->src0 = 0x00;//pin->dst0;    //00
    psend->src1 = 0x01;//pin->dst1;    //01
    psend->dst0 = 0x00;//pin->src0;    //00
    psend->dst1 = 0x01;//pin->src0;    //01
    psend->len0 = 0x00;//pin->len0;
//    psend->len1 = pin->len1;

    len = MIN(13, p[sizeof(p47_header_t)]);
    memcpy(po + pos, &psend[1], len + 1);    //表地址在这里
    pos += len + 1;

    if (pin->AFN == 0x0A)
    {
        psend->AFN = 0xAA;
        psend->type = 0x03;
        po[pos] = 0x00;
        pos++;
        /* CRC16校验 */
        crc = crc16_calculate(po + 9, pos - 9);
        po[pos++] = (unsigned char)(crc & 0xFF);
        po[pos++] = (unsigned char)(crc >> 8);
        psend->len1 = pos - 8;

        return pos;  //固定
    }
    if (pin->AFN == 0x0C)
    {
        psend->AFN = 0xCC;
        psend->type = 0x03;

        /* CRC16校验 */
        crc = crc16_calculate(po + 9, pos - 9);
        po[pos++] = (unsigned char)(crc & 0xFF);
        po[pos++] = (unsigned char)(crc >> 8);
        psend->len1 = pos - 8;

        return pos;
    }

    return 0;
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
p47_addr_cmp(const addr_t *paddr,
        const unsigned char *p)
{
    addr_t addr;
//    p47_header_t *ph = (p47_header_t *)p;
    int i;
    const unsigned char *pa = p + sizeof(p47_header_t);

    memset(&addr, 0x00, sizeof(addr));

    for (i = 0; i < MIN(12, pa[0] & 0xFE); i += 2)
    {
        addr.addr_c6[i >> 1] = ((pa[i + 1] - 0x30) << 4) | (pa[i + 2] - 0x30);
    }
    if (pa[0] & 1)
    {
        addr.addr_c6[i >> 1] = (pa[i + 1] - 0x30) << 4;
    }

    return memcmp(&addr, paddr, 6) ? 1 : 0;
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
p47_addr_get(addr_t *paddr,
        const unsigned char *p)
{
//    p47_header_t *ph = (p47_header_t *)p;
    int i;
    const unsigned char *pa = p + sizeof(p47_header_t);

    memset(paddr, 0x00, sizeof(*paddr));
    for (i = 0; i < MIN(12, pa[0] & 0xFE); i += 2)
    {
        paddr->addr_c6[i >> 1] = ((pa[i + 1] - 0x30) << 4) + (pa[i + 2] - 0x30);
    }
    if (pa[0] & 1)
    {
        paddr->addr_c6[i >> 1] = (pa[i + 1] - 0x30) << 4;
    }
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
p47_addr_str(const addr_t *paddr)
{
    //910100-000100
    static char buf[14];

    sprintf(buf, "%02X%02X%02X-%02X%02X%02X",
            paddr->addr_c6[0], paddr->addr_c6[1], paddr->addr_c6[2],
            paddr->addr_c6[3], paddr->addr_c6[4], paddr->addr_c6[5]);
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
p47_msa_cmp(const addr_t *paddr,
        const unsigned char *p)
{
    p47_header_t *ph = (p47_header_t *)p;

    return ((ph->src0 | (ph->src1 << 8)) == paddr->msa) ? 0 : 1;
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
p47_msa_get(addr_t *paddr,
        const unsigned char *p)
{
    p47_header_t *ph = (p47_header_t *)p;

    memset(paddr, 0x00, sizeof(addr_t));
    paddr->msa = ph->src0 | (ph->src1 << 8);
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
p47_is_msa_valid(const addr_t *paddr)
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
p47_get_oline_addr(addr_t *paddr,
        const unsigned char *p)
{
//    p47_header_t *ph = (p47_header_t *)p;
    return 0;
}

static const ptcl_func_t the_p47_ptcl_func =
{
    "62056-47",
    0,
    p47_chkfrm_init,
    p47_chkfrm,
    p47_get_dir,
    p47_frame_type,
    p47_build_reply_packet,
    p47_addr_cmp,
    p47_addr_get,
    p47_addr_str,
    p47_msa_cmp,
    p47_msa_get,
    p47_is_msa_valid,
    p47_get_oline_addr,
    NULL,
};
/**
 ******************************************************************************
 * @brief   获取62056-47协议处理接口
 * @retval  62056-47协议处理接口
 ******************************************************************************
 */
const ptcl_func_t *
p47_ptcl_func_get(void)
{
    return &the_p47_ptcl_func;
}

/*----------------------------ptcl_62056-47.c--------------------------------*/
