/**
 *******************************************************************************
 * @file      taskLib.h
 * @brief     多任务内核模块函数接口.
 * @details   本文件封装了所有任务、信号量、消息队列的相关操作接口
 *
 * @copyright      Copyright(C), 2008-2012,Sanxing Electric Co.,Ltd.
 *
 ******************************************************************************
 */
#ifndef _TASKLIB_H_
#define _TASKLIB_H_


/*-----------------------------------------------------------------------------
 Section: Includes
 -----------------------------------------------------------------------------*/
#include <stdint.h>

/*-----------------------------------------------------------------------------
 Section: Macro Definitions
 -----------------------------------------------------------------------------*/
#ifndef OK
#define OK      0
#endif
#ifndef ERROR
#define ERROR       (-1)
#endif

/*-----------------------------------------------------------------------------
 Section: Type Definitions
 -----------------------------------------------------------------------------*/
typedef void * TASK_ID;
typedef void * SEM_ID;
typedef void * MSG_Q_ID;

typedef int    status_t;
typedef void        (*OSFUNCPTR) (void *);     /* ptr to function returning int */

/*-----------------------------------------------------------------------------
 Section: Globals
 -----------------------------------------------------------------------------*/
/* NONE */

/*-----------------------------------------------------------------------------
 Section: Function Prototypes
 -----------------------------------------------------------------------------*/

extern TASK_ID
taskSpawn(const char * name, uint32_t priority, uint32_t stackSize,
        OSFUNCPTR entryPt, uint32_t arg);

extern void
taskDelete(TASK_ID tid);

extern void
taskSuspend(TASK_ID tid);

extern void
taskResume(TASK_ID tid);

extern void
taskLock(void);

extern void
taskUnlock(void);

extern void
taskDelay(uint32_t ticks);

extern char *
taskName(TASK_ID tid);

extern status_t
taskIdVerify(TASK_ID tid);

extern TASK_ID
taskIdSelf();

extern uint32_t
tickGet(void);

extern uint32_t
sysClkRateGet(void);

extern SEM_ID
semBCreate(uint32_t cnt);

extern void
semDelete(SEM_ID semId);

extern status_t
semTake(SEM_ID semId, uint32_t timeout);

extern status_t
semGive(SEM_ID semId);

extern MSG_Q_ID
msgQCreate(uint32_t msgQLen);

extern void
msgQDelete(MSG_Q_ID msgQId);

extern status_t
msgQSend(MSG_Q_ID msgQId, void *pmsg);

extern status_t
msgQReceive(MSG_Q_ID msgQId, uint32_t timeout, void **pmsg);

extern int
msgQNumMsgs(MSG_Q_ID msgQId);


#endif /* _TASKLIB_H_ */
/*--------------------------End of taskLib.h----------------------------*/
