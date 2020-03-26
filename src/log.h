/**
 ******************************************************************************
 * @file       log.h
 * @brief      日志记录模块
 * @details    This file including all API functions's declare of log.h.
 * @copyright
 *
 ******************************************************************************
 */
#ifndef LOG_H_
#define LOG_H_ 

#ifdef __cplusplus             /* Maintain C++ compatibility */
extern "C" {
#endif /* __cplusplus */
/*-----------------------------------------------------------------------------
 Section: Includes
 ----------------------------------------------------------------------------*/
/* NONE */

/*-----------------------------------------------------------------------------
 Section: Macro Definitions
 ----------------------------------------------------------------------------*/
/**
 * 默认调试级别
 * 0 : 打印出错信息
 * 1 : 打印出错信息 + 报文日志
 * 2 : 打印出错信息 + 报文日志 + 调试打印信息
 */
#define L_ERROR     0
#define L_NORMAL    1
#define L_DEBUG     2

/*-----------------------------------------------------------------------------
 Section: Type Definitions
 ----------------------------------------------------------------------------*/
/* NONE */

/*-----------------------------------------------------------------------------
 Section: Globals
 ----------------------------------------------------------------------------*/
extern int the_log_level;

/*-----------------------------------------------------------------------------
 Section: Function Prototypes
 ----------------------------------------------------------------------------*/
extern void
log_set_level(int level);

extern int
log_init(void);

extern void
log_exit(void);

extern void
log_buf(int level,
        const char *pformat,
        const unsigned char *pbuffer,
        int len);

extern void
log_print(int level,
        const char *fmt, ...);

extern void
log_sync(void);

#ifdef __cplusplus      /* Maintain C++ compatibility */
}
#endif /* __cplusplus */
#endif /* LOG_H_ */
/*--------------------------End of log.h-----------------------------*/
