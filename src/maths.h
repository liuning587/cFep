/**
 ******************************************************************************
 * @file       maths.h
 * @brief      ϵͳ�������������ͺ�Ľӿ�
 * @details    ���ļ�����ϵͳ���õ���������ʽ����궨��
 * @copyright  
 ******************************************************************************
 */
#ifndef _MATHS_H_
#define _MATHS_H_

/*-----------------------------------------------------------------------------
 Section: Macro Definitions
 ----------------------------------------------------------------------------*/
#define MAX(x, y)   (((x) < (y)) ? (y) : (x))
#define MIN(x, y)   (((x) < (y)) ? (x) : (y))
#define BCD2HEX(x)  (((x) >> 4) * 10 + ((x) & 0x0F))            /* 20H -> 20 */
#define HEX2BCD(x)  (((x) % 10) + ((((x) / 10) % 10) << 4))     /* 20 -> 20H */

#define MSB(x) (((x) >> 8) & 0xff)    /* most signif byte of 2-byte integer */
#define LSB(x) ((x) & 0xff)           /* least signif byte of 2-byte integer*/
#define MSW(x) (((x) >> 16) & 0xffff) /* most signif word of 2-word integer */
#define LSW(x) ((x) & 0xffff)         /* least signif byte of 2-word integer*/

/* swap the MSW with the LSW of a 32 bit integer */
#define WORDSWAP(x) (MSW(x) | (LSW(x) << 16))

#define LLSB(x)     ((x) & 0xff)        /* 32bit word byte/word swap macros */
#define LNLSB(x)    (((x) >> 8) & 0xff)
#define LNMSB(x)    (((x) >> 16) & 0xff)
#define LMSB(x)     (((x) >> 24) & 0xff)
#define LONGSWAP(x) ((LLSB(x) << 24) |  (LNLSB(x) << 16)| \
                     (LNMSB(x) << 8) |  (LMSB(x)))

#define LMMSB(x)    (((x)>>32)&0xff)
#define LLLSB(x)    ((x) & 0x0F)
#define LLNLSB(x)   (((x) >> 4) & 0x0F)
#define LLNLHB(x)   (((x) >> 8) & 0x0f)
#define BYTESWAP(x) ((MSB((x)) | (LSB((x)) << 8)))
#define BITS(x,y)   (((x)>>(y))&0x01)   /* �ж�ĳλ�Ƿ�Ϊ1 */
#define SETBITS(x,y,n) (x) = (n) ? ((x)|(1 << (y))) : ((x) &(~(1 << (y))));
#define INVERSE(x,y)    ((x)=(x)^(1<<(y)))  /* ��ĳλ�÷� */
#define ARRAY_SIZE(a) (sizeof (a) / sizeof ((a)[0]))

#define MOFFSET(structure, member)  /* byte offset of member in structure*/\
        ((int) &(((structure *) 0) -> member))

#define MEMBER_SIZE(structure, member)  /* size of a member of a structure */\
        (sizeof (((structure *) 0) -> member))

#define ROUND_UP(x, align)  (((int) (x) + (align - 1)) & ~(align - 1))
#define ROUND_DOWN(x, align)    ((int)(x) & ~(align - 1))
#define ALIGNED(x, align)   (((int)(x) & (align - 1)) == 0)

#define in_range(c, lo, up)  ((uint8_t)c >= lo && (uint8_t)c <= up)
#define isprint(c)           in_range(c, 0x20, 0x7f)
#define isdigit(c)           in_range(c, '0', '9')
#define isxdigit(c)          (isdigit(c) || in_range(c, 'a', 'f') || in_range(c, 'A', 'F'))
#define islower(c)           in_range(c, 'a', 'z')
#define isspace(c)           (c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v')
#define isascii(c)          ((unsigned) (c) <= 0177)
#define toascii(c)          ((c) & 0177)
#define STR_LEN(str)        (sizeof(str) - 1)

/*�����ж�*/
#define IS_LEAP_YEAR(y) (((((y) % 4) == 0) && (((y) % 100) != 0)) \
          || (((y) % 400) == 0))

#endif /*_MATH_H_ */

/*-------------------------------End of math.h-------------------------------*/
