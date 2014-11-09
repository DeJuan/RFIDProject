/** \file include/host_types.h
 *
 * Typedefs for types of definite bit length.
 * Conditionalized for GNU and TMS320C55xx compilers
 *
 * \author Harry Tsai <harry@thingmagic.com>
 * \date 20031024
 * Copyright (C) 2003 ThingMagic, LLC
 */

#ifndef __HOSTTYPES_H__
#define __HOSTTYPES_H__

/****************************************/
/* 8, 16, 32-bit types */

#if __TMS320C55X__
/* TMS320C5502 */
typedef unsigned char u8;
typedef char s8;
typedef unsigned short u16;
typedef short s16;
typedef unsigned long u32;
typedef long s32;

/* Make sure NULL is defined, if nothing else has caught it yet. */
#ifndef NULL
# define NULL (0)
#endif

#endif /* __TMS320C55X__ */

#if _TMS320C5XX
/* TMS320C5410 */
typedef unsigned char u8;
typedef char s8;
typedef unsigned int u16;
typedef int s16;
typedef unsigned long u32;
typedef long s32;
#endif /* _TMS320C5XX */

#ifdef __GNUC__
#ifdef __linux__
/* GNU/Linux */
#include <asm/types.h>
typedef __u8 u8;
typedef __s8 s8;
typedef __u16 u16;
typedef __s16 s16;
typedef __u32 u32;
typedef __s32 s32;
typedef __u64 u64;
typedef __s64 s64;
#elif defined(__CYGWIN__)
#include <sys/types.h>
typedef u_int8_t u8;
typedef int8_t s8;
typedef u_int16_t u16;
typedef int16_t s16;
typedef u_int32_t u32;
typedef int32_t s32;
typedef u_int64_t u64;
typedef int64_t s64;
#else
typedef unsigned char u8;
typedef char s8;
typedef unsigned short u16;
typedef short s16;
typedef unsigned long u32;
typedef long s32;
typedef unsigned long long u64;
typedef long long s64;
#endif
typedef unsigned int uInt;
typedef int sInt;
typedef unsigned int            DATA;
typedef unsigned long           LDATA;
typedef void*                   tAddr;
#endif /* __GNUC__ */

#if (!RPC_HDR) && (!RPC_XDR) && (!RPC_SVC) && (!RPC_CLNT)
/****************************************/
/* HPI addresses are specified in a 16-bit space. */
typedef u16 HPI_ADDR;
#endif

#if 0
/****************************************/
/* __attribute__((packed)) makes GCC pack structures just as written.
 * DSP compiler doesn't support __attribute__ but packs by default. */
#ifndef __attribute__
#define __attribute__(x)  /* NOP if not supported by compiler */
#endif
#endif

/****************************************/
/* Number of bytes/words necessary to hold a number of bits. */
#define NUM_BYTES_FOR_BITS(numbits) \
  (((numbits)==0)?0:(1+(((numbits)-1)>>3)))
#define NUM_U16_FOR_BITS(numbits) \
  (((numbits)==0)?0:(1+(((numbits)-1)>>4)))
#define NUM_U32_FOR_BITS(numbits) \
  (((numbits)==0)?0:(1+(((numbits)-1)>>5)))


#define NUM_U16_FOR_HEX_STRLEN(len) (((len)==0)?0:(1+(((len)-1)>>2)))

/****************************************/
/* Boolean values */

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#if (!RPC_HDR) && (!RPC_XDR) && (!RPC_SVC) && (!RPC_CLNT)
#ifndef __cplusplus
#ifndef __bool_true_false_are_defined 
/* boolean */
typedef enum 
{
  False = 0,                    /* false */
  True  = 1                     /* true */
} bool;
#endif
#endif
#endif /* !RPC */

#ifndef PROFILE_BEGIN
#define PROFILE_BEGIN(metric)
#endif
#ifndef PROFILE_END
#define PROFILE_END(metric)
#endif

#endif /* __HOSTTYPES_H__ */
