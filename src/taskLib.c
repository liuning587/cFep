/**
 ******************************************************************************
 * @file      taskLib.c
 * @brief     C Source file of taskLib.c.
 * @details   This file including all API functions's 
 *            implement of taskLib.c.	
 *
 * @copyright
 ******************************************************************************
 */
 
/*-----------------------------------------------------------------------------
 Section: Includes
 ----------------------------------------------------------------------------*/
#ifdef _WIN32
#include <windows.h>
#endif
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "taskLib.h"

/*-----------------------------------------------------------------------------
 Section: Type Definitions
 ----------------------------------------------------------------------------*/
/* NONE */

/*-----------------------------------------------------------------------------
 Section: Constant Definitions
 ----------------------------------------------------------------------------*/
/* NONE */ 

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
 * @brief   创建任务
 * @param[in]  None
 * @param[out] None
 * @retval     None
 ******************************************************************************
 */
extern TASK_ID
taskSpawn(const char * name, uint32_t priority, uint32_t stackSize,
        OSFUNCPTR entryPt, void *arg)
{
#ifdef _WIN32
    void *pvThread;
    DWORD tid;
    pvThread = CreateThread(NULL, stackSize, (LPTHREAD_START_ROUTINE)entryPt,
            (PVOID)arg, 0, &tid);
    return (TASK_ID)pvThread;
#else
	return NULL;
#endif
}

/**
 ******************************************************************************
 * @brief   创建信号量
 * @param[in]   信号量初值
 *
 * @retval  信号量ID
 ******************************************************************************
 */
extern SEM_ID
semBCreate(uint32_t cnt)
{
#ifdef _WIN32
    return (SEM_ID)CreateSemaphore(NULL, cnt, 10, NULL);
#else
	return NULL;
#endif
}

/**
 *******************************************************************************
 * @brief      This function delete a mutual exclusion semaphore.
 * @param[in]   semId    semaphore ID to delete

 *
 * @details
 *
 * @note
 *******************************************************************************
 */
extern void
semDelete(SEM_ID semId)
{
#ifdef _WIN32
    (void)CloseHandle((HANDLE)semId);
#else
#endif
}

/**
 *******************************************************************************
 * @brief      This function waits for a mutual exclusion semaphore.
 * @param[in]   semId    semaphore ID to delete
 * @param[in]  timeout   timeout in ticks
 * @retval         OK on success, ERROR otherwise
 *
 * @details
 *
 * @note
 *******************************************************************************
 */
extern status_t
semTake(SEM_ID semId, uint32_t timeout)
{
#ifdef _WIN32
    if (!WaitForSingleObject((HANDLE)semId, timeout * 10))
    {
        return ERROR;
    }
#else
#endif
    return OK;
}

/**
 *******************************************************************************
 * @brief      This function signals a semaphore.
 * @param[in]   semId   Semaphore ID to give
 * @retval         OK on success, ERROR otherwise
 *
 * @details
 *
 * @note
 *******************************************************************************
 */
extern status_t
semGive(SEM_ID semId)
{
#ifdef _WIN32
    if (!ReleaseSemaphore((HANDLE)semId, 1, NULL))
    {
        return ERROR;
    }
#else
#endif
    return OK;
}

/**
 ******************************************************************************
 * @brief      delay the current task.
 * @param[in]   ticks : number of ticks to delay task
 *
 * @details  This function is called to delay execution of the currently running
 *  task until the specified number of system ticks expires
 *
 * @note
 ******************************************************************************
 */
extern void
taskDelay(uint32_t ticks)
{
#ifdef _WIN32
    Sleep(ticks * 10);
#else
    usleep(ticks * 10000);
#endif
}

uint32_t
sysClkRateGet(void)
{
    return 100;
}

uint32_t
timerGet()
{
    return clock();
}

/*--------------------------------taskLib.c----------------------------------*/
