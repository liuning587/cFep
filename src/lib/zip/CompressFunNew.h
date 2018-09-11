

#ifndef _COMPRESS_NEW_
#define _COMPRESS_NEW_


#define N       4096    /* buffer size */
#define F       60  /* lookahead buffer size */
#define THRESHOLD   2
#define NIL     N   /* leaf of tree */

#define N_CHAR      (256 - THRESHOLD + F)
                /* kinds of characters (character code = 0..N_CHAR-1) */
#define T       (N_CHAR * 2 - 1)    /* size of table */
#define R       (T - 1)         /* position of root */
#define MAX_FREQ    0x8000      /* updates tree when the */



#include "CrypFun.h"



int Expand(DATA * buffer);
int Compress(DATA * buffer);


#endif
