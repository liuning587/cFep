/**
 ******************************************************************************
 * @file       ptcl.h
 * @brief      API include file of ptcl.h.
 * @details    This file including all API functions's declare of ptcl.h.
 * @copyright  
 *
 ******************************************************************************
 */
#ifndef PTCL_H_
#define PTCL_H_ 

/*-----------------------------------------------------------------------------
 Section: Includes
 ----------------------------------------------------------------------------*/
/* NONE */

/*-----------------------------------------------------------------------------
 Section: Macro Definitions
 ----------------------------------------------------------------------------*/
/* NONE */

/*-----------------------------------------------------------------------------
 Section: Type Definitions
 ----------------------------------------------------------------------------*/
typedef enum
{
    E_PTCL_GW,  /**< 国网2013规约 */
    E_PTCL_NW,   /**< 南网新规约 */
    E_PTCL_END,
} ptcl_type_e;

/** 报文类型 */
typedef enum FUNCTYPE
{
    LINK_LOGIN = 0,
    LINK_EXIT,
    LINK_HAERTBEAT,
    OTHER,
    FUNCERROR,
    ONLINE
} func_type_e;

#pragma pack(push, 1)

/** 主站、终端通用地址结构6字节 */
typedef union
{
    int msa;
    int addr;                   /**< 终端地址 */
    unsigned char addr_c4[4];
    unsigned char addr_c6[6];
    unsigned char addr_c16[16];
} addr_t;

typedef struct
{
    void (*pfn_frame_in)(void*, const unsigned char*, int);
    unsigned char frame_state;  /* 报文状态 */
    time_t update_time;         /* 更新时间 */
    unsigned char overtime;     /* 报文等待超时时间 */
    unsigned int dlen;          /* 用户数据长度,解析自报文长度域 */
    unsigned int cfm_len;       /* 用户数据确认长度，用于解析数据长度临时用 */

    unsigned char *pbuf;        /* 报文池 */
    unsigned int pbuf_pos;      /* 报文接收字节偏移位置 */
    unsigned char cs;           /* 校验和 */
} chkfrm_t;

/** 协议处理接口 */
typedef struct
{
    const char *pname; /**< 协议名称 */

    char support_app_heart; /**< 是否支持后台软件登陆、心跳 */

    /**
     * 报文检测初始化
     */
    void (*pfn_chkfrm_init)(chkfrm_t *,
            void (*pfn_frame_in)(void*, const unsigned char*, int));

    /**
     * 报文检测
     */
    void (*pfn_chfrm)(void*, chkfrm_t*, const unsigned char*, int);

    /**
     *  获取报文传输方向,0:主站-->终端, 1:终端-->主站
     */
    int (*pfn_get_dir)(const unsigned char*);

    /**
     * 获取报文类型
     */
    func_type_e (*pfn_frame_type)(const unsigned char*);

    /**
     * 打包登陆、心跳回复包
     */
    int (*pfn_build_reply_packet)(const unsigned char*, unsigned char*);

    /**
     * 终端地址比较
     */
    int (*pfn_addr_cmp)(const addr_t*, const unsigned char*);

    /**
     * 从报文中取出终端地址
     */
    void (*pfn_addr_get)(addr_t*, const unsigned char*);

    /**
     * 获取终端字符串
     */
    const char* (*pfn_addr_str)(const addr_t*);

    /**
     * 主站MSA地址比较
     */
    int (*pfn_msa_cmp)(const addr_t*, const unsigned char*);

    /**
     * 从报文中取出主站MSA地址
     */
    void (*pfn_msa_get)(addr_t*, const unsigned char*);

    /**
     * 判断主站发出的msa是否有效
     */
    int (*pfn_is_msa_valid)(const addr_t*);

    /**
     * 获取主站发出查询是否在线终端地址
     */
    int (*pfn_get_oline_addr)(addr_t*, const unsigned char *);

    /**
     * 创建查询online回复报文
     */
    int (*pfn_build_online_packet)(const unsigned char*, unsigned char*, int);
} ptcl_func_t;

#pragma pack(pop)

/*-----------------------------------------------------------------------------
 Section: Globals
 ----------------------------------------------------------------------------*/
extern int the_max_frame_bytes;

/*-----------------------------------------------------------------------------
 Section: Function Prototypes
 ----------------------------------------------------------------------------*/
extern const ptcl_func_t *
gw_ptcl_func_get(void);

extern const ptcl_func_t *
nw_ptcl_func_get(void);

extern const ptcl_func_t *
zj_ptcl_func_get(void);

extern const ptcl_func_t *
jl_ptcl_func_get(void);

extern const ptcl_func_t *
p47_ptcl_func_get(void);

extern const ptcl_func_t *
p698_ptcl_func_get(void);

#endif /* PTCL_H_ */
/*------------------------------End of ptcl.h--------------------------------*/
