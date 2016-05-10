/******************************************************************************
 *  MD5.H - header file for MD5.C                                             *
 *  Copyright (C) 2001-2002 by ShadowStar.                                    *
 *  Use and modify freely.                                                    *
 *  http://shadowstar.126.com/                                                *
 ******************************************************************************
 */
#ifndef _MD5_H
#define _MD5_H

//---------------------------------------------------------------------------

/* POINTER defines a generic pointer type */
typedef unsigned char *POINTER;

/* UINT2 defines a two byte word */
typedef unsigned short int UINT2;

/* UINT4 defines a four byte word */
typedef unsigned int UINT4;


/* Constants for MD5Transform routine.
 */
#define S11 7
#define S12 12
#define S13 17
#define S14 22
#define S21 5
#define S22 9
#define S23 14
#define S24 20
#define S31 4
#define S32 11
#define S33 16
#define S34 23
#define S41 6
#define S42 10
#define S43 15
#define S44 21

//---------------------------------------------------------------------------
/* MD5 context. */
typedef struct
{
	UINT4 state[4];           /* state (ABCD) */
	UINT4 count[2];           /* number of bits, modulo 2^64 (lsb first) */
	unsigned char buffer[64]; /* input buffer */
} MD5_CTX;

//---------------------------------------------------------------------------
class CMD5
{
public :
	void MessageDigest(const unsigned char *szInput, unsigned int inputLen,
			unsigned char szOutput[16], int iIteration = 1);
private :
	MD5_CTX context;
	void Init(MD5_CTX *context);
	void Update(MD5_CTX *context, const unsigned char *input, unsigned int inputLen);
	void Final(unsigned char digest[16], MD5_CTX *context);
	void Transform(UINT4 state[4], const unsigned char block[64]);
};

//---------------------------------------------------------------------------
/* F, G, H and I are basic MD5 functions.
 */
#define F(x, y, m_alloc_size) (((x) & (y)) | ((~x) & (m_alloc_size)))
#define G(x, y, m_alloc_size) (((x) & (m_alloc_size)) | ((y) & (~m_alloc_size)))
#define H(x, y, m_alloc_size) ((x) ^ (y) ^ (m_alloc_size))
#define I(x, y, m_alloc_size) ((y) ^ ((x) | (~m_alloc_size)))

//---------------------------------------------------------------------------
/* ROTATE_LEFT rotates x left n bits.
 */
#define ROTATE_LEFT(x, m_datalen) (((x) << (m_datalen)) | ((x) >> (32-(m_datalen))))

/* FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4.
Rotation is separate from addition to prevent recomputation.
 */
#define FF(m_pdata, b, c, d, x, s, ac)\
{ \
	(m_pdata) += F ((b), (c), (d)) + (x) + (UINT4)(ac); \
	(m_pdata) = ROTATE_LEFT ((m_pdata), (s)); \
	(m_pdata) += (b); \
}
#define GG(m_pdata, b, c, d, x, s, ac)\
{ \
	(m_pdata) += G ((b), (c), (d)) + (x) + (UINT4)(ac); \
	(m_pdata) = ROTATE_LEFT ((m_pdata), (s)); \
	(m_pdata) += (b); \
}
#define HH(m_pdata, b, c, d, x, s, ac)\
{ \
	(m_pdata) += H ((b), (c), (d)) + (x) + (UINT4)(ac); \
	(m_pdata) = ROTATE_LEFT ((m_pdata), (s)); \
	(m_pdata) += (b); \
}
#define II(m_pdata, b, c, d, x, s, ac)\
{ \
	(m_pdata) += I ((b), (c), (d)) + (x) + (UINT4)(ac); \
	(m_pdata) = ROTATE_LEFT ((m_pdata), (s)); \
	(m_pdata) += (b); \
}
//---------------------------------------------------------------------------

#endif