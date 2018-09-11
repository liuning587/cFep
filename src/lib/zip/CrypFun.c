

#include <stdlib.h>
#include "CrypFun.h"
#include "RD.h"



void Substitution(word8 a[4][MAXBC], word8 box[256], word8 BC) 
{
	/* Replace every byte of the input by the byte at that place
	 * in the nonlinear S-box
	 */
	int i, j;
	
	for(i = 0; i < 4; i++)
		for(j = 0; j < BC; j++) a[i][j] = box[a[i][j]] ;
}

void RD_EnMain(DATA * buffer,word8 K[4][MAXKC]) 
{
	word32 i;
	int	k,j;
	word32 count,leng;
	word8 a_Sub[4][MAXBC];
	word8 m;
	word8 rk[ROUNDS+1][4][MAXBC];
	RDKeySched(K,rk);

	//////////////////////////////
	///////////////////////////////
	leng=buffer->length;
	m=buffer->length%16;
//	if ((buffer->x = realloc(buffer->x, buffer->length+16)) == NULL)
//		exit(0);
	buffer->length=leng+16-m;
	for(i=0;i<leng;i++)
		buffer->x[leng-i]=buffer->x[leng-1-i];
	for(i=leng+1;i<buffer->length;i++)
		buffer->x[i]=0;
	buffer->x[0]=m;
	//////////////////////////////
	//////////////////////////////

	count=buffer->length/16;
	for(i=0;i<count;i++)
	{
		for(j=0;j<4;j++)
			for(k=0;k<4;k++)
				a_Sub[j][k]=buffer->x[i*16+j*4+k];
		RDEncrypt(a_Sub,rk);
		for(j=0;j<4;j++)
			for(k=0;k<4;k++)
				buffer->x[i*16+j*4+k]=a_Sub[j][k];
	}
//	return buffer;
	return;
}

int RDKeySched (word8 k[4][MAXKC], word8 W[ROUNDS+1][4][MAXBC])
{
	/* Calculate the necessary round keys
	 * The number of calculations depends on keyBits and blockBits
	 */
	int KC,BC;
	int i, j, t, rconpointer = 0;
	word8 tk[4][MAXKC]; 
	KC=4;
	BC=4; 

	for(j = 0; j < KC; j++)
		for(i = 0; i < 4; i++)
			tk[i][j] = k[i][j];
	t = 0;
	/* copy values into round key array */
	for(i = 0; i < 4; i++)
		for(j = 0; j < BC; j++)
		{
			W[0][i][j] = tk[i][(j)%4]
				^MColumn(tk[i][(j+1)%4],cTable[0])
				^ tk[i][(j+2)%4]
				^ MColumn(tk[i][(j+3)%4],cTable[1]);
		} 
	for(t=1;t<=ROUNDS;t++)
	{  /* while not enough round key material calculated */
		/* calculate new values */
		for(i = 0; i < 4; i++)
			tk[i][0] ^= S[tk[(i+1)%4][KC-1]];
		tk[0][0] ^= rcon[rconpointer++];

		for(j = 1; j < KC; j++)
			for(i = 0; i < 4; i++) tk[i][j] ^= tk[i][j-1];
		
		for(i = 0; i < 4; i++)
			for(j = 0; j < BC; j++)
			{
				W[t][i][j] = tk[i][(j)%4]
				^MColumn(tk[i][(j+1)%4],cTable[0])
				^ tk[i][(j+2)%4]
				^ MColumn(tk[i][(j+3)%4],cTable[1]);
			}
	}		
    
	return 0;
}

BYTE MColumn(BYTE a, int n)
{
	int i;
	for(i=0;i<n;i++)
		a=M[a];
	return a;
}

int RDEncrypt (word8 a[4][MAXBC], word8 rk[MAXROUNDS+1][4][MAXBC])
{
	/* Encryption of one block. 
	 */
	int r, BC;
	BC=4;
	/* begin with a key addition
	 */
	KeyAddition(a,rk[0],BC); 

     /* ROUNDS-1 ordinary rounds
	 */
	for(r = 1; r < ROUNDS; r++) {
		Substitution(a,S,BC);
		ShiftRow(a,0,BC);
		MixColumn(a,BC);
		KeyAddition(a,rk[r],BC);
	}
	
	/* Last round is special: there is no MixColumn
	 */
	Substitution(a,S,BC);
	ShiftRow(a,0,BC);
	KeyAddition(a,rk[ROUNDS],BC);

	return 0;
}

void KeyAddition(word8 a[4][MAXBC], word8 rk[4][MAXBC], word8 BC) 
{
	/* Exor corresponding text input and round key input bytes
	 */
	int i, j;
	
	for(i = 0; i < 4; i++)
   		for(j = 0; j < BC; j++) a[i][j] ^= rk[i][j];
	return;
}

void ShiftRow(word8 a[4][MAXBC], word8 d, word8 BC)
{
	/* Row 0 remains unchanged
	 * The other three rows are shifted a variable amount
	 */
	word8 tmp[MAXBC];
	int i, j;
	
	for(i = 1; i < 4; i++) 
	{
		for(j = 0; j < BC; j++) tmp[j] = a[i][shifts[d][i-1][j]]^(shifts[d+2][i-1][j]>>4);
		for(j = 0; j < BC; j++) a[i][j] = tmp[j];
	}
}



void MixColumn(word8 a[4][MAXBC], word8 BC) 
{
    /* Mix the four bytes of every column in a linear way
	 */
	word8 b[4][MAXBC];
	int i, j;
		
	for(j = 0; j < BC; j++)
			for(i = 0; i < 4; i++)
			{
				b[i][j] = MColumn(a[i][j],cTable[0])
					^ a[(i + 1) % 4][j]
					^ a[(i + 2) % 4][j]
					^ MColumn(a[(i + 3) % 4][j],cTable[1]);
			}
				
	for(i = 0; i < 4; i++)
		for(j = 0; j < BC; j++) a[i][j] = b[i][j];
}

void InvMixColumn(word8 a[4][MAXBC], word8 BC)
{
        /* Mix the four bytes of every column in a linear way
	 * This is the opposite operation of Mixcolumn
	 */
	word8 b[4][MAXBC];
	int i, j;
	
	for(j = 0; j < BC; j++)
		for(i = 0; i < 4; i++)             
			b[i][j] = mul(dTable[0],a[i][j])
			^ mul(dTable[3],a[(i + 1) % 4][j])                 
			^ mul(dTable[2],a[(i + 2) % 4][j])
			^ mul(dTable[1],a[(i + 3) % 4][j]);                        
	for(i = 0; i < 4; i++)
		for(j = 0; j < BC; j++) a[i][j] = b[i][j];
}

BYTE mul(word8 a, word8 b) {
   /* multiply two elements of GF(2^m)
    * needed for MixColumn and InvMixColumn
    */
	if (a && b) return Alogtable[(Logtable[a] + Logtable[b])%255];
	else return 0;
}

void RD_DeMain(DATA * buffer,word8 K[4][MAXKC]) 
{
	word32 i;
	int	k,j;
	word32 count,leng;
	word8 a_Sub[4][MAXBC];
	word8 m;
	word8 rk[ROUNDS+1][4][MAXBC];
	RDKeySched(K,rk);	
	count=buffer->length/16;
	for(i=0;i<count;i++)
	{
		for(j=0;j<4;j++)
			for(k=0;k<4;k++)
				a_Sub[j][k]=buffer->x[i*16+j*4+k];
		RDDecrypt(a_Sub,rk);
		for(j=0;j<4;j++)
			for(k=0;k<4;k++)
				buffer->x[i*16+j*4+k]=a_Sub[j][k];
	}

	m=buffer->x[0];
	leng=buffer->length;
	for(i=0;i<leng-1;i++)
		buffer->x[i]=buffer->x[i+1];
	buffer->length=leng-(16-m);
	return;
//	return buffer;
}


int RDDecrypt (word8 a[4][MAXBC], word8 rk[MAXROUNDS+1][4][MAXBC])
{
	int r, BC;
	/* To decrypt: apply the inverse operations of the encrypt routine,
	 *             in opposite order
	 * 
	 * (KeyAddition is an involution: it 's equal to its inverse)
	 * (the inverse of Substitution with table S is Substitution with the inverse table of S)
	 * (the inverse of Shiftrow is Shiftrow over a suitable distance)
	 */

        /* First the special round:
	 *   without InvMixColumn
	 *   with extra KeyAddition
	 */
	BC=4;
	KeyAddition(a,rk[ROUNDS],BC);
	Substitution(a,Si,BC);
	ShiftRow(a,1,BC);              
	/* ROUNDS-1 ordinary rounds
	 */
	for(r = ROUNDS-1; r > 0; r--) {
		KeyAddition(a,rk[r],BC);
		InvMixColumn(a,BC); 
		Substitution(a,Si,BC);
		ShiftRow(a,1,BC);                
	}
	
	/* End with the extra key addition
	 */
	
	KeyAddition(a,rk[0],BC);    

	return 0;
}

