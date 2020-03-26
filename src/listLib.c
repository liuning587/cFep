/**
 ******************************************************************************
 * @file      listLib.c
 * @brief     通用链表处理.
 * @details   This file including all API functions's implement of list.c.
 * @copyright
 *
 ******************************************************************************
 */
 
/*-----------------------------------------------------------------------------
Section: Includes
-----------------------------------------------------------------------------*/
#include "listLib.h"

/*-----------------------------------------------------------------------------
Section: Type Definitions
-----------------------------------------------------------------------------*/
/* NONE */

/*-----------------------------------------------------------------------------
Section: Constant Definitions
-----------------------------------------------------------------------------*/
/* NONE */ 

/*-----------------------------------------------------------------------------
Section: Global Variables
-----------------------------------------------------------------------------*/
/* NONE */

/*-----------------------------------------------------------------------------
Section: Local Variables
-----------------------------------------------------------------------------*/
/* NONE */

/*-----------------------------------------------------------------------------
Section: Local Function Prototypes
-----------------------------------------------------------------------------*/
/* NONE */

/*-----------------------------------------------------------------------------
Section: Function Definitions
-----------------------------------------------------------------------------*/
/**
 ******************************************************************************
 * @brief      链表初始化
 * @param[in]  *pHead   :
 * @retval     None
 *
 * @details
 *
 * @note
 ******************************************************************************
 */
void
InitListHead(struct ListNode *pHead)
{
    pHead->pNextNode = pHead;
    pHead->pPrevNode = pHead;
}

/**
 ******************************************************************************
 * @brief      .
 * @param[in]  None
 * @param[out] None
 * @retval     None
 *
 * @details
 *
 * @note
 ******************************************************************************
 */
static inline void
__list_add(struct ListNode *pNew,
            struct ListNode *pPrev,
            struct ListNode *pNext)
{
    pNext->pPrevNode = pNew;
    pNew->pNextNode  = pNext;
    pNew->pPrevNode  = pPrev;
    pPrev->pNextNode = pNew;
}

/**
 ******************************************************************************
 * @brief  将节点插入链表头
 * @param[in]  None
 * @param[out] None
 * @retval     None
 *
 * @details
 *
 * @note
 ******************************************************************************
 */
void
ListAddHead(struct ListNode *pNew,
        struct ListNode *pHead)
{
    __list_add(pNew, pHead, pHead->pNextNode);
}

/**
 ******************************************************************************
 * @brief   将节点插入链表尾
 * @param[in]  None
 * @param[out] None
 * @retval     None
 *
 * @details
 *
 * @note
 ******************************************************************************
 */
void
ListAddTail(struct ListNode *pNew,
        struct ListNode *pHead)
{
    __list_add(pNew, pHead->pPrevNode, pHead);
}

/**
 ******************************************************************************
 * @brief   将链表插入链表尾
 * @param[in]  None
 * @param[out] None
 * @retval     None
 *
 * @details
 *
 * @note
 ******************************************************************************
 */
void
ListAddTailList(struct ListNode *pNew,
        struct ListNode *pHead)
{
    pHead->pPrevNode->pNextNode = pNew->pNextNode;
    pNew->pNextNode->pPrevNode = pHead->pPrevNode;

    pNew->pPrevNode->pNextNode = pHead;
    pHead->pPrevNode = pNew->pPrevNode;
}

/**
 ******************************************************************************
 * @brief      .
 * @param[in]  None
 * @param[out] None
 * @retval     None
 *
 * @details
 *
 * @note
 ******************************************************************************
 */
static inline void
__list_del(struct ListNode *pPrev,
        struct ListNode *pNext)
{
    pNext->pPrevNode = pPrev;
    pPrev->pNextNode = pNext;
}

/**
 ******************************************************************************
 * @brief      删除节点
 * @param[in]  *pHead   :
 * @retval     None
 *
 * @details
 *
 * @note
 ******************************************************************************
 */
void
ListDelNode(struct ListNode *pNode)
{
    __list_del(pNode->pPrevNode, pNode->pNextNode);
}

/**
 ******************************************************************************
 * @brief      判断链表是否为空
 * @param[in]  *pHead   :
 *
 * @retval     None
 ******************************************************************************
 */
int
ListIsEmpty(const struct ListNode *pHead)
{
    return pHead->pNextNode == pHead;
}
/*------------------------------ listLib.c ----------------------------------*/
