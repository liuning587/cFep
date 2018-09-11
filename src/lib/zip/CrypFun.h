

#ifndef _CRYPFUN_H_
#define _CRYPFUN_H_




#define MAXBUFFER		20000	//最大申请内存长度
#define MAXBC				4
#define MAXKC				4
#define MAXROUNDS			10
#define ROUNDS              10
#define BUF_DEF				1024

//int OK;//用于验证hash值

//
#ifndef DATAZIP
	typedef unsigned char BYTE;
#endif


typedef unsigned char	word8;
typedef unsigned short  word16;
typedef unsigned long	word32;

#define   BIAOSHI1  0xDD//period压缩标识
#define   BIAOSHI2  0xBB//RAY压缩标识

#define   COMPARE_CHAR  0x9A//为寻找周期设置的比较值
#define   GUESS_SIZE  30

typedef struct
{
	word8 *x;
	word32 length;
}DATA;

struct vlong_value {
  unsigned * a; // 单位数组
  unsigned z; // 分配的单位数
  unsigned n; // 已用的单位 (只读)
};

struct prime_factory
{
  unsigned np;
  unsigned *pl;  
};

struct vlong {
	struct vlong_value value;
	unsigned negative;
};

extern void *rt_malloc(int size);
extern void rt_free(void *addr);

int RDDecrypt(BYTE a[4][MAXBC], BYTE rk[MAXROUNDS+1][4][MAXBC]);
void RD_DeMain(DATA *buffer,BYTE mainKey[4][MAXKC]);
BYTE mul(word8 a, word8 b);
void InvMixColumn(BYTE a[4][MAXBC],BYTE BC);
void MixColumn(BYTE a[4][MAXBC],BYTE BC);
void ShiftRow(BYTE a[4][MAXBC], BYTE d,BYTE BC);
void KeyAddition(BYTE a[4][MAXBC], BYTE rk[4][MAXBC], BYTE BC);
int RDEncrypt(BYTE a[4][MAXBC], BYTE rk[MAXROUNDS+1][4][MAXBC]);
BYTE MColumn(BYTE a, int n);
int RDKeySched(BYTE k[4][MAXKC], BYTE W[ROUNDS+1][4][MAXBC]);
void RD_EnMain(DATA *buffer,BYTE mainKey[4][MAXKC]);


#endif // !defined(AFX_CRYPFUN_H__1EC6029B_0E82_49BF_A978_9F0CBDDAE293__INCLUDED_)

