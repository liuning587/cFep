

#ifndef _COMPRESSFUN_H_
#define _COMPRESSFUN_H_


	
#include "CrypFun.h"


//	int SHA_1(DATA *buffer);
	int CheckHash(DATA * buffer);
	word8 SHA_K(int t, int i);
	word8 SHA_F(word8 * B, word8 * C, word8 * D, int t, word8 * a);
	void SHA_CZJ(word8 *a,word8 *b);
	int ExpendRAY(DATA * buffer);
	int ExpendPeriod(DATA *buffer);
	int SHA_64(DATA * buffer);
	void SHA_PAD(DATA * buffer,DATA * y);
	int RAYCompression(DATA * buffer);
	int PeriodCompression(DATA *buffer);


/*
	void set_one(unsigned char *x, int a);
	int get_bit(unsigned char *x, int a);
*/

#endif // !defined(AFX_COMPRESSFUN_H__5F639936_7850_43A2_B4AC_4852661D05F2__INCLUDED_)
