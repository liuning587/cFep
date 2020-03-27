/**
 ******************************************************************************
 * @file       listLib.h
 * @brief      list模块.
 * @details    提供统一的链表操作方法.
 * @copyright
 *
 ******************************************************************************
 */
#ifndef __LISTLIB_H__
#define __LISTLIB_H__

/*-----------------------------------------------------------------------------
 Section: Type Definitions
 ----------------------------------------------------------------------------*/
struct ListNode
{
	struct ListNode *pNextNode;
	struct ListNode *pPrevNode;
};

#define LIST_FOR_EACH(iter, pHead) \
	for (iter = (pHead)->pNextNode; iter != (pHead); iter = iter->pNextNode)

#define LIST_FOR_EACH_SAFE(iter, n, pHead) \
    for (iter = (pHead)->pNextNode, n = iter->pNextNode; iter != (pHead); \
    iter = n, n = iter->pNextNode)

#define MemToObj(ptr, type, member) \
    (type *)((char *)ptr - (char *)(&((type *)0)->member))

/*-----------------------------------------------------------------------------
 Section: Function Prototypes
 ----------------------------------------------------------------------------*/
extern void
InitListHead(struct ListNode *pHead);

extern void
ListAddHead(struct ListNode *pNew,
        struct ListNode *pHead);
extern void
ListAddTail(struct ListNode *pNew,
        struct ListNode *pHead);

extern void
ListAddTailList(struct ListNode *pNew,
        struct ListNode *pHead);

extern void
ListDelNode(struct ListNode *pNode);

extern int
ListIsEmpty(const struct ListNode *pHead);

#endif /* __LISTLIB_H__ */
/*-----------------------------End of listLib.h------------------------------*/
