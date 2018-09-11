

#include <string.h>
#include "CCEMan.h"
#include "CompressFun.h"
#include "CrypFun.h"


//#pragma arm section rwdata = "SRAM", zidata = "SRAM"		//Unfinished
unsigned char SendBuf[MAXSBUFLEN];
unsigned char RecvBuf[MAXSBUFLEN];
unsigned char zip_EnBuf[MAXSBUFLEN];
//#pragma arm section

#if ZIP_ENCRYPT_ENABLE
BYTE m_key[4][MAXKC]; 

void CCEManInit()
{
	memset(m_key,0,sizeof(m_key));
}

int SetKey(BYTE MainKey[4][4],int KeyLen)
{
	if(KeyLen != sizeof(m_key))
		return 0;
	memcpy(m_key,MainKey,KeyLen);
	return 1;
}
#endif


int FormFrame(unsigned char Oper,unsigned char * buf,int buflen)
{
	memset(SendBuf,0,sizeof(SendBuf));
	SendBuf[0] = 0x88;
	SendBuf[1] = Oper;
	SendBuf[2] = buflen / 0x100;
	SendBuf[3] = buflen % 0x100;
	memcpy(&SendBuf[4],buf,buflen);
	SendBuf[buflen + 4] = 0x77;
	return buflen + 5;
}

int DeData(BYTE * DataBuf, int DataLen)
{
	DATA temp;
	
#if ZIP_SHA_ENABLE
	int result = 0;
#endif
	int length = 0;
	
	memset(RecvBuf,0,sizeof(RecvBuf));
	
	if(DataLen<4)
		return -3;
	length = DataBuf[2] * 0x100 + DataBuf[3];
	
	if(DataBuf[0] != 0x88 || DataLen - 5 != length || DataBuf[DataLen -1] != 0x77)
	{
		memcpy(RecvBuf,&DataBuf,DataLen);
		return -3;
	}
	
	memcpy(RecvBuf,&DataBuf[4],length);
	temp.x = RecvBuf;
	temp.length = length;
	
#if ZIP_ENCRYPT_ENABLE
	if((DataBuf[1] & EXE_ENCRYPT) == EXE_ENCRYPT)
	{
		RD_DeMain(&temp, m_key);
	}
#endif
#if ZIP_SHA_ENABLE
	if((DataBuf[1] & EXE_SHA) == EXE_SHA)
	{
		if((result = CheckHash (&temp))==-2)			//校验散列值
			return result;
	}
#endif
	if((DataBuf[1] & EXE_COMPRESS_NEW) == EXE_COMPRESS_NEW)
	{
		Expand(&temp);
	}
	
	return temp.length;
}

int EnData(BYTE * DataBuf, int DataLen, unsigned char Oper)
{
	DATA temp;

#if ZIP_SHA_ENABLE
	int result;
#endif
	int len;
	
	memset(SendBuf,0,sizeof(SendBuf));

	memcpy(zip_EnBuf,DataBuf,DataLen);
	
	temp.x = zip_EnBuf;
	temp.length = DataLen;
	
	if((Oper & EXE_COMPRESS_NEW) == EXE_COMPRESS_NEW)
	{
		Compress(&temp);
	}

#if ZIP_SHA_ENABLE
	if((Oper & EXE_SHA) == EXE_SHA)
	{
		if((result = SHA_64(&temp))==-2)			//计算散列值
			return result;
	}
#endif

#if ZIP_ENCRYPT_ENABLE
	if((Oper & EXE_ENCRYPT) == EXE_ENCRYPT)
	{
		RD_EnMain(&temp, m_key);
	}
#endif

	len = FormFrame(Oper,temp.x,temp.length);
	
	return len;
}

int CEExpand(DATA * temp)
{
	int i;
	
	if((i = ExpendRAY (temp))==-2)  //RAY解压
		return i;
	
	if((i = ExpendPeriod (temp))==-2) //周期解压
		return i;
	
	return 0;
	
}

int CECompress(DATA * temp)
{
	int i;
	
	if((i = PeriodCompression (temp))==-2) //周期压缩
		return i;
	
	if((i = RAYCompression (temp))==-2)  //RAY压缩
		return i;
	
	return 0;	
}

int CheckFrame(unsigned char * buf,int buflen)
{
	int datalen;

	if(buf[0] != 0x88 ) 
		return -1;
	if(buf[buflen + 4] != 0x77)
		return -1;
	datalen = buf[2] * 0xff + buf[3];
	if(buflen !=datalen + 4)
		return -2;
	
	return 0;
}

