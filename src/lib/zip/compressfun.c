

#include <stdlib.h>
#include <string.h>
#include "CompressFun.h"

void set_one(unsigned char *x, int a)//将x的从前往后数第a个比特位置为1
{
	unsigned char table[8]={128,64,32,16,8,4,2,1};
	x[a/8] = x[a/8]^table[a%8];
}

int get_bit(unsigned char *x, int a)//获得x的第a个比特位置的数值
{
	unsigned char table[8]={128,64,32,16,8,4,2,1};
	int bit = 0;
	bit = x[a/8] & table[a%8];
	bit = bit >> (7-a%8);
	return bit;
}


//周期压缩函数
//返回值：
//0：成功
//-1:跳过
//-2:失败
int PeriodCompression(DATA *buffer)
{
	int i, j, k;
	int guess_length;
	int period, current_period;
	int period_number = 0, current_period_number;
	word32 head_length, current_head_length;
	word32 tail_length;
	
	int getit;
	
	int same_column_number;
	int no_zero;
	
	word8 *MaxMinColumn;
	
	DATA zip;
	DATA record;
	DATA zero;//记录相同列代表元中的非零原
	
	//begin
	period = buffer->length;
	current_period = buffer->length;
	head_length = buffer->length;
	current_head_length = buffer->length;
	
	//首先寻找周期
	/*确定猜测长度*/
	if (GUESS_SIZE < (buffer->length/2))        //GUESS_SIZE = 30
		guess_length = GUESS_SIZE;
	else
		guess_length = buffer->length/2;
	
	for (i=0; i<guess_length; i++)
	{
		if (buffer->x[i] > COMPARE_CHAR)
		{
			current_head_length = i;
			
			for (j=i+1; j<buffer->length/2; j++)
			{
				getit = 1;
				if (buffer->x[j] > COMPARE_CHAR)
				{
					current_period = j - current_head_length;		
					current_period_number = (buffer->length - current_head_length) / current_period;
					for (k=2; k<current_period_number; k++)
					{
						if (buffer->x[current_head_length+k*current_period] < COMPARE_CHAR)
						{
							getit = 0;
							break;
						}
					}
					if ((getit == 1) && (period>current_period))//当前周期寻找成功
					{
						//记录最短周期及其信息
						period = current_period;
						head_length = current_head_length;
						period_number = current_period_number;
						break;
					}			
				}		
			}
			current_period = buffer->length;
			current_head_length= buffer->length;
		}
	}//寻找周期结束
	
	
	//压缩判断数据提出相同列是否有效率
	if (period >= buffer->length/2)
		return -1;
	else//
	{
		tail_length = buffer->length - period * period_number - head_length;
		
		if ((MaxMinColumn = (word8 *)rt_malloc(period * 2)) == NULL)
			exit(0);
		
		record.length = (period+7) / 8;
		
		if ((record.x = (word8 *)rt_malloc(record.length)) == NULL)
			exit(0);
		
		memset(record.x, 0, record.length);
		
		if ((zip.x = (word8 *)rt_malloc(buffer->length * 2)) == NULL)
			exit(0);
		//分配结束
		
		//搜寻周期部分每一列的最大和最小值
		for (k=0; k<period; k++)
		{
			MaxMinColumn[2*k] = buffer->x[head_length+k];
			MaxMinColumn[2*k+1] = buffer->x[head_length+k];
			for (j=0; j<period_number; j++)
			{
				if (MaxMinColumn[2*k] < buffer->x[head_length+j*period+k])
					MaxMinColumn[2*k] = buffer->x[head_length+j*period+k];
				if (MaxMinColumn[2*k+1] > buffer->x[head_length+j*period+k])
					MaxMinColumn[2*k+1] = buffer->x[head_length+j*period+k];
			}
		}
		
		same_column_number = 0;
		for (i=0; i<period; i++)
		{
			if (MaxMinColumn[2*i] == MaxMinColumn[2*i+1])
			{
				same_column_number++;
			}
		}
		
		zero.length = (same_column_number+7) / 8;
		
		if ((zero.x = (word8 *)rt_malloc(zero.length)) == NULL)
			exit(0);
		
		memset(zero.x, 0, zero.length);
		
		//构造zip数据
		/*输出头部数据*/
		memcpy(zip.x+5, buffer->x, head_length);
		
		zip.length = 5 + zero.length + record.length + head_length;
		
		same_column_number = 0;
		no_zero = 0;
		for (i=0; i<period; i++)
		{
			if (MaxMinColumn[2*i] == MaxMinColumn[2*i+1])
			{
				set_one(record.x, i);
				same_column_number++;
				if (MaxMinColumn[2*i] != 0)
				{
					set_one(zero.x, same_column_number-1);
					zip.x[zip.length++] = MaxMinColumn[2*i];
					no_zero++;
				}
			}
		}
		
		//构造zip数据
		zip.x[0] = BIAOSHI1;//压缩标识
		zip.x[1] = period;
		zip.x[2] = head_length;
		zip.x[3] = buffer->length/256;
		zip.x[4] = buffer->length%256;
		
		//输出record
		memcpy(zip.x+head_length+5, record.x, record.length);
		
		//输出zero
		memcpy(zip.x+5+head_length+record.length, zero.x, zero.length);
		
		//将列不全相同的矩阵输出
		for (i=0; i<period_number; i++)
		{
			for (j=0; j<period; j++)
			{
				if (get_bit(record.x, j) == 0)
					zip.x[zip.length++] = buffer->x[head_length+i*period+j];
			}
		}
		//输出尾部数据
		for (i=0; i<tail_length; i++)
			zip.x[zip.length++] = buffer->x[head_length+period*period_number+i];
		
		//判断提出相同列是否有效率
		if (same_column_number*period_number > 5+record.length+zero.length+no_zero)
		{
			memcpy(buffer->x, zip.x, zip.length);
			buffer->length = zip.length;
		}
		
		//释放指针
		rt_free(zip.x);
		zip.x = NULL;
		
		rt_free(record.x);
		record.x = NULL;
		
		rt_free(zero.x);
		zero.x = NULL;
		
		rt_free(MaxMinColumn);
		MaxMinColumn = NULL;
		
		return 0;
	}
}

//DEL BYTE get_bit(BYTE bit, int a)
//DEL {
//DEL 	bit = bit & 0x01<<(7-(a%8));
//DEL 	bit = bit >> (7-a%8);
//DEL 	return bit;
//DEL }

//位图压缩函数
//返回值：
//0：成功
//-1:跳过
//-2:失败
int RAYCompression(DATA *buffer)
{
	long int i, j;
	long int stat_number;			//统频个数记数器
	unsigned long int data_length;
	unsigned long int Byte[256]; 
	int nolyone;
	int flag_number = 0;
	int rule_number = 0;
	unsigned int max = 0;
	unsigned int doublebytes;
	unsigned int maxdoublebytes = 0;
	unsigned int *stat;
	
	DATA zip;
	DATA buffer_copy;
	unsigned char rule[3*256];
	unsigned char flag[256];
	
	for (i=0; i<256; i++)			//初始化
	{
		Byte[i] = 0;
		flag[i] = 0;
	}
	for (i=0; i<(3*256); i++)
		rule[i] = 0x00;
	
	if ((stat = (unsigned int *)rt_malloc((buffer->length)*8)) == NULL)
	     	return -2;
	if ((zip.x = (BYTE *)rt_malloc((buffer->length )*2)) == NULL)
	{
		rt_free(stat);// add by tanxin 2006.9.7
		stat = NULL;
		return -2;
	}
	if ((buffer_copy.x = (BYTE *)rt_malloc(buffer->length)) == NULL)
	{
		rt_free(stat);
	        rt_free(zip.x); // add by tanxin 2006.9.7
		stat = NULL;
		zip.x = NULL;
		return -2;
	}
	buffer_copy.length = buffer->length;
	memcpy(buffer_copy.x, buffer->x, buffer->length);
	
	data_length = buffer->length;		//记录原数据长度
	
	for (j=0; j<buffer->length; j++)
	{
		Byte[buffer->x [j]]++;
	}
	
	flag_number = 0;
	
	for (j=0; j<256; j++)
	{
		if (Byte[j&0xff] == 0)
		{
			flag[flag_number++] = (j&0xff);
		}
	}				//字节统计，并对buffer中没有的字节进行记数，并存入数组
	
	memset(stat,0x00,(buffer->length)*4);
	
	max = 3;
	stat_number = 0;
	
	for (j=0; j<buffer->length-1; j++)
	{
		doublebytes = buffer->x [j]*256 + buffer->x [j+1];
		nolyone = 0;
		for (i=0; i<stat_number; i++)
		{
			if (stat[2*i] == doublebytes)
			{
				stat[2*i+1]++;
				nolyone = 1;
				
				if (max < stat[2*i+1])
				{
					max = stat[2*i+1];
					maxdoublebytes = stat[2*i];
				}
				break;
			}
		}
		if (nolyone == 0)
		{
			stat[2*stat_number+1] = 1;
			stat[2*stat_number] = doublebytes;
			stat_number++;
		}
	}
	
	while (	(max > 3) && (flag_number > 0) )
	{
		rule[rule_number*3] = flag[flag_number-1];
		rule[rule_number*3+1] = (maxdoublebytes>>8) & 0xFF;
		rule[rule_number*3+2] = maxdoublebytes & 0xFF;
		rule_number++;
		
		zip.length = 0;
		for (j=1; j<buffer->length; j++)
		{
			if ((buffer->x [j-1]*256 + buffer->x [j]) == maxdoublebytes)
			{
				zip.x [zip.length++] = flag[flag_number-1];
				j++;
			}
			else
				zip.x [zip.length++] = buffer->x [j-1];
		}
		if (j != buffer->length+1 )
			zip.x [zip.length++] = buffer->x [j-1];
		flag_number--;
		
		for (i=0; i<zip.length; i++)
			buffer->x [i] = zip.x [i];
		buffer->length = zip.length;
		
		memset(stat,0x00,(buffer->length )*4);
		
		//重新统频
		max = 3;
		stat_number = 0;
		for (j=0; j<buffer->length-1; j++)
		{
			doublebytes = buffer->x [j]*256 + buffer->x [j+1];
			nolyone = 0;
			if(rule_number==100)
				i=0;
			for (i=0; i<stat_number; i++)
			{
				if (stat[i*2] == doublebytes)
				{
					stat[i*2+1]++;
					nolyone = 1;
					if (max < stat[i*2+1])
					{
						max = stat[i*2+1];
						maxdoublebytes = stat[i*2];
					}
					break;			
				}
			}
			if (nolyone == 0)
			{
				stat[2*stat_number+1] = 1;
				stat[2*stat_number] = doublebytes;
				stat_number++;
			}
		}
		
	}
	
	for (i=0; i<(long int)rule_number; i++)
	{
		buffer->x [buffer->length++] = rule[i*3];
		buffer->x [buffer->length++] = rule[i*3+1];
		buffer->x [buffer->length++] = rule[i*3+2];
	}
	
	//附加压缩标识	==RAY处理结束===
	for (i=0; i<buffer->length; i++)
		zip.x [i] = buffer->x [i];
	zip.length = buffer->length;
	
	if (rule_number != 0)
	{
		buffer->length += 4;
		buffer->x [0] = BIAOSHI2;
		buffer->x [1] = rule_number;
		buffer->x [2] = data_length/256;
		buffer->x [3] = data_length%256;
		
		for (i=0; i<zip.length; i++)
			buffer->x [i+4] = zip.x [i];
	}
	//如果压缩没有效率
	if (buffer_copy.length <= buffer->length)
	{
		memcpy(buffer->x, buffer_copy.x, buffer_copy.length);
		buffer->length = buffer_copy.length;
	}
	
	rt_free(buffer_copy.x);
	buffer_copy.x = NULL;
	rt_free(stat);
	stat = NULL;
	rt_free(zip.x );
	zip.x = NULL;
	
	return 0;
}

//完整性计算函数
//返回值：
//0：成功
//-1:跳过
//-2:失败
int SHA_64(DATA * buffer)
{
	
	DATA           SJ;  
	word8  *y,H0[4],H1[4],A0[4],A1[4],W[320],temp[4]={0},s[4],tt;
	word32  l,n,t,fenzu,i;
	word8 *c;
	
	
	H0[0]=0x12;H0[1]=0x34;H0[2]=0x56;H0[3]=0x78;
	H1[0]=0x90;H1[1]=0xab;H1[2]=0xcd;H1[3]=0xef;
	
	l=buffer->length;
	
	if ((c = (word8 *)rt_malloc(l+128)) == NULL)
		exit(0);
	memset(c,0,(l+128));
	
	SJ.x = c;
    SHA_PAD(buffer,&SJ); //数据填充
	
	n=SJ.length/64;       //分组数，每组64字节，512比特
	y=SJ.x;
	
	for(fenzu=0;fenzu<n;fenzu++)
	{
		for(i=0;i<320;i++)  W[i]=0; 
		t=fenzu*64;
		for(i=0;i<64;i++)   W[i]=y[t+i];  
		
		for(t=16;t<80;t++) 
		{
			for(i=0;i<4;i++)  W[t*4+i]=W[(t-3)*4+i]^W[(t-8)*4+i]^W[(t-14)*4+i]^W[(t-16)*4+i]; 		
			tt=W[t*4]; 
			for(i=0;i<3;i++) W[t*4+i]=(W[t*4+i]<<1)|(W[t*4+i+1]>>7&1);
			W[t*4+3]=(W[t*4+3]<<1)|(tt>>7&1);
		}
		
		for(i=0;i<4;i++) {A0[i]=H0[i]; A1[i]=H1[i];}
		
		for(t=0;t<80;t++)
		{
			for(i=0;i<4;i++) temp[i]=0;
			for(i=0;i<4;i++) s[i]=W[t*4+i];
			SHA_F(A0,A1,s,t,temp);
			
			for(i=0;i<4;i++) s[i]=SHA_K(t,i);
			SHA_CZJ(temp,s);
			
			A1[0]=((A0[3]&0x3)<<6)|(A0[0]>>2);
			A1[1]=((A0[0]&0x3)<<6)|(A0[1]>>2);
			A1[2]=((A0[1]&0x3)<<6)|(A0[2]>>2);
			A1[3]=((A0[2]&0x3)<<6)|(A0[3]>>2);
			
			for(i=0;i<4;i++) A0[i]=temp[i];	
		}
		
		SHA_CZJ(H0,A0);
		SHA_CZJ(H1,A1);	
	}
	rt_free(c);
	memcpy(&buffer->x[l],H0,sizeof(H0));
	memcpy(&buffer->x[l+4],H1,sizeof(H1));
	//	for(i=0;i<4;i++) MD-[i]=H0[i];
	//	for(i=0;i<4;i++) MD[4+i]=H1[i];
	//	OUT.x=MD;
	//	OUT.length=8;
	buffer->length += 8;
	return 0;
	
}

//周期解压函数
//返回值：
//0：成功
//-1:跳过
//-2:失败
int ExpendPeriod(DATA *buffer)
{
	int period;
	int head_length;
	int period_number;
	int tail_length;
	word32 data_length;//原数据长度
	int same_column_number;
	int no_zero;
	int count1, count2, count3;//累加记数器
	
	int i, j, m;
//	word32 k;
	
	DATA recover;
	DATA record;
	DATA zero;
	//begin
	
	if (buffer->x[0] == BIAOSHI1)
	{
		//解析压缩信息
		period = buffer->x[1];
		head_length = buffer->x[2];
		data_length = buffer->x[3] * 256 + buffer->x[4];
		
		//初始化分配内存
		period_number = (data_length - head_length) / period;//获得周期数
		
		tail_length = data_length - period * period_number - head_length;
		
		record.length = (period+7) / 8;
		
		if ((record.x = (word8 *)rt_malloc(record.length)) == NULL)
			exit(0);
		
		if ((recover.x = (word8 *)rt_malloc(data_length)) == NULL)
			exit(0);
		//分配结束
		
		//===============恢复数据==============//
		
		recover.length =  head_length + period * period_number;
		//恢复头部数据
		memcpy(recover.x, buffer->x+5, head_length);
		
		//提取record
		memcpy(record.x, buffer->x+5+head_length, record.length);
		
		same_column_number = 0;
		for (i=0; i<period; i++)
		{	
			if (get_bit(record.x, i) == 1)
				same_column_number++;
		}
		
		zero.length = (same_column_number+7) / 8;
		
		if ((zero.x = (word8 *)rt_malloc(zero.length)) == NULL)
			exit(0);
		
		//提取zero
		memcpy(zero.x, buffer->x+5+head_length+record.length, zero.length);
		
		no_zero = 0;
		for (i=0; i<same_column_number; i++)
		{
			if (get_bit(zero.x, i) == 1)
				no_zero++;
		}
		
		//恢复周期段数据
		count2 = 0;
		count3 = 0;
		for (i=0; i<period_number; i++)
		{
			count1 = 0;
			for (j=0; j<period; j++)
			{
				if (get_bit(record.x, j) == 0)
				{
					recover.x[head_length+i*period+j] = 
						buffer->x[5+head_length+record.length+zero.length+no_zero+i*(period-same_column_number)+count1];
					count1++;			
				}
				else//列相同
				{
					if (i == 0)
					{
						if (get_bit(zero.x, count3) == 0)
						{
							for (m=0; m<period_number; m++)
							{
								recover.x[head_length+m*period+j] = 0x00;
							}
						}
						else
						{
							for (m=0; m<period_number; m++)
							{
								recover.x[head_length+m*period+j] = buffer->x[5+head_length+record.length+zero.length+count2];		
							}
							count2++;
						}
						count3++;
					}
				}
			}
		}
		
		//恢复尾部数据
		for (i=0; i<tail_length; i++)
			recover.x[recover.length++] = buffer->x[buffer->length-tail_length+i];
		//结束
		
		//重新分配buffer内存
		//		if ((buffer->x = realloc(buffer->x, data_length)) == NULL)
		//			exit(0);
		
		//返回buffer
		memcpy(buffer->x, recover.x, recover.length);
		buffer->length = recover.length;
		
		//
		rt_free(recover.x);
		recover.x = NULL;
		
		rt_free(record.x);
		record.x = NULL;
		
		rt_free(zero.x);
		zero.x = NULL;
	}
	
	//	return buffer;
	return 0;
}

//位图解压函数
//返回值：
//0：成功
//-1:跳过
//-2:失败
int ExpendRAY(DATA *buffer)
{
	DATA recover;
	long int i, j, k;
	long int rule_number;
	long int data_length;
	
	unsigned char flag;
	unsigned char digram[2];
	
	rule_number = (long int)buffer->x [1];
	data_length = (long int)buffer->x [2]*256 + buffer->x [3];
	
	//重新分配buffer内存
	//	if ((buffer->data = (BYTE *)realloc(buffer->data , data_length*2)) == NULL)
	//		return -2;
	
	if (buffer->x [0] != BIAOSHI2)
	{
		return 0;
	}
	else
	{
		if ((recover.x = (BYTE *)rt_malloc(data_length*2)) == NULL)
			return -2;
		recover.length = 0;
		
		for (j=0; j<rule_number; j++)
		{	
			//提取规则
			flag = buffer->x [buffer->length - 3];
			digram[0] = buffer->x [buffer->length - 2];
			digram[1] = buffer->x [buffer->length - 1];
			recover.length = 0;
			
			if (j == 0)
				k = 4;
			else
				k = 0;
			for (i=k; i<(buffer->length-3); i++)
			{
				if (buffer->x [i] == flag)
				{
					recover.x[recover.length++] = digram[0];
					recover.x[recover.length++] = digram[1];
				}
				else
					recover.x[recover.length++] = buffer->x [i];
				
				if(recover.length >= (data_length*2))
					return -2;
			}
			
			for (i=0; i<recover.length; i++)
				buffer->x [i] = recover.x[i];
			buffer->length = recover.length;
		}
		
		//释放内存
		rt_free(recover.x);
		recover.x = NULL;
		
		return 0;
	}
}

//DEL unsigned long int Rotl(unsigned long x, int s)
//DEL {
//DEL 	return (x<<s)|(x>>(32-s));
//DEL }

word8 SHA_F(word8 * B, word8 * C, word8 * D, int t, word8 * a)
{
   	word8 ss[4];
	int  i;
	
	
	if(0<=t && t<20) 
	{
		for(i=0;i<4;i++) a[i]=B[i]&C[i];
		for(i=0;i<4;i++) ss[i]=(~B[i])&D[i];
		for(i=0;i<4;i++) a[i]=a[i]|ss[i];
	} 
	
	else if(t>19 && t<40) 
	{
		for(i=0;i<4;i++) a[i]=B[i]^C[i]^D[i];
	}  
	
	else if(t>39 && t<60) 
	{
		for(i=0;i<4;i++) a[i]=B[i]&C[i];
		for(i=0;i<4;i++) ss[i]=B[i]&D[i];
		for(i=0;i<4;i++) a[i]=a[i]|ss[i];
		for(i=0;i<4;i++) ss[i]=C[i]&D[i];
		for(i=0;i<4;i++) a[i]=a[i]|ss[i];
	} 
	
	else if(t>59 && t<80) 
	{
		for(i=0;i<4;i++) a[i]=B[i]^C[i]^D[i];
	}
	else {exit(0);}
	return 0;
	
}

word8 SHA_K(int t,int i)
{
	if(0<=t && t<20) 
	{
		if(i==0) return 0x5a;
		else if(i==1) return 0x82;
		else if(i==2) return 0x79;
		else if(i==3) return 0x99;
		else {exit(0);}
	}
			 
    else if(t>19 && t<40) //return 0x6ed9eba1;    3^(1/2)*2^32/4
	{
		if(i==0) return 0x6e;
		else if(i==1) return 0xd9;
		else if(i==2) return 0xeb;
		else if(i==3) return 0xa1;
		else {exit(0);}
	}
	
	else if(t>39 && t<60) //return 0x8f1bbcdc;	//5^(1/2)*2^32/4
	{
		if(i==0) return 0x8f;
		else if(i==1) return 0x1b;
		else if(i==2) return 0xbc;
		else if(i==3) return 0xdc;
		else {exit(0);}
	}
	
    else if(t>59 && t<80) //return 0xca62c1d6;	10^(1/2)*2^32/4
	{
		if(i==0) return 0xca;
		else if(i==1) return 0x62;
		else if(i==2) return 0xc1;
		else if(i==3) return 0xd6;
		else {exit(0);}
	}
	
	else {exit(0);}
}

int CheckHash(DATA *buffer)
{
	BYTE MD[8];
	long int i;
	int datalen;
	
	for (i=buffer->length-8; i<buffer->length; i++)		//提取散列值
		MD[i-(buffer->length-8)] = buffer->x [i];
	
	buffer->length -= 8;     //去掉散列值
	 datalen = buffer->length;
	
	if(SHA_64(buffer)<0)								//重新计算											
		return -1;
	
	for(i=0; i<8; i++)
	{
		if(MD[i] != buffer->x [datalen+i])
			return -2;
	}
	buffer->length -= 8;
	return 0;
}

/*
int SHA_1(DATA *buffer)
{
	unsigned long int H[5],W[80],t,fenzu,l,n,temp,A,B,C,D,E,i,d,byte_num;;
	BYTE  *y,*c;
	BYTE MD[20];
	
	H[0]=0x67452301;     
	H[1]=0xefcdab89;
	H[2]=0x98badcfe;
	H[3]=0x10325476;
	H[4]=0xc3d2e1f0;
	
	l=buffer->length;
	
	if ((c = (BYTE *)rt_malloc(l+1024)) == NULL)
		return -2;
	memset(c,0,(l+1024));
	
	  l=buffer->length<<3;
	  d=(447-l)%512;
	  byte_num=(l+1+d+64)>>3; 
	  
	  for(i=0;i<buffer->length;i++) 
		  c[i]=buffer->data [i];
	  
	  c[i]=1<<7; 
	  i=0;
	  while(i<4)
	  {
		  c[byte_num-1-i]|=((BYTE)((l>>(8*i))&0xff));
		  i++;
	  }
	  y = c;
	  n = byte_num>>6;
	  
	  for(fenzu=0;fenzu<n;fenzu++)
	  {
		  for(i=0;i<80;i++) W[i]=0;
		  for(i=0;i<64;i++) 
		  {
			  t=i/4;
			  W[t]=(W[t]<<8)|y[(fenzu<<6)+i];
		  }
		  for(t=16;t<80;t++) W[t]=Rotl(W[t-3]^W[t-8]^W[t-14]^W[t-16],1);
		  A=H[0];  
		  B=H[1];  
		  C=H[2]; 
		  D=H[3];  
		  E=H[4];
		  for(t=0;t<80;t++)
		  {
			  temp=(Rotl(A,5)+SHA_f(B,C,D,t)+E+W[t]+SHA_k(t));  
			  E=D;
			  D=C;
			  C=Rotl(B,30);
			  B=A;
			  A=temp;
		  }	
		  H[0]+=A;
		  H[1]+=B;
		  H[2]+=C;
		  H[3]+=D;
		  H[4]+=E; 
	  }
	  rt_free(c);
	  
	  for(i=0;i<5;i++) 
	  {
		  MD[4*i]=(unsigned char)((H[i]>>24)&0xff);
		  MD[4*i+1]=(unsigned char)((H[i]>>16)&0xff);
		  MD[4*i+2]=(unsigned char)((H[i]>>8)&0xff);
		  MD[4*i+3]=(unsigned char)(H[i]&0xff);
	  }
	  
	  for (i=0; i<20; i++)
		  buffer->data [buffer->length++] = MD[i];
	  
	  return 0;
}
*/

void SHA_CZJ(word8 *a,word8 *b)
{
	word8 jinwei;
	int           s,i;
	
	jinwei=0;
	for(i=3;i>0;i--)
	{
		s=a[i]+b[i]+jinwei;
		a[i]=s%256;
		jinwei=s/256;
	}
	s=a[0]+b[0]+jinwei;
	a[0]=s%256; 
}

void SHA_PAD(DATA * buffer,DATA * y) 
{
	word32 l,d,i,byte_num;
	word8 *p;
	//	DATA          OUT;
	
	l=buffer->length<<3;         //源数据的比特长度，即文件中的|x|
	d=(447-l)%512;
	byte_num=(l+1+d+64)>>3;     //扩充后数据的字节数，且为是64的倍数
	
	p=buffer->x;
	for(i=0;i<buffer->length;i++) y->x[i]=p[i];
	y->x[i]=1<<7; 
	y->x[byte_num-1]=(word8)(l%256);
	y->x[byte_num-2]=(word8)(l/256);
	
	//OUT.x=y;
	//OUT.length=byte_num;
	y->length = byte_num;
	//	return 0;
}

