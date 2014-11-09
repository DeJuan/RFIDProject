/*
 * File: HostTypes.h
 * Author: SPAIK
 *
 * This file contains standard type definitions.
 *
 * 20 Oct 2003: Created.
 *
 * Copyright ThingMagic LLC 2003
 */

#ifndef __HOSTTYPES_H__
#define __HOSTTYPES_H__

// Native machine size definition
typedef int               sInt;
typedef unsigned int      uInt;

// Standard hosttypes definitions

/* Microsoft doesn't have stdint.h */
#ifdef _MSC_VER
typedef   signed __int8		s8;
typedef unsigned __int8		u8;
typedef   signed __int16	s16;
typedef unsigned __int16	u16;
typedef   signed __int32	s32;
typedef unsigned __int32	u32;
typedef   signed __int64	s64;
typedef unsigned __int64	u64;
/* C99-compliant compilers have stdint.h (e.g., MinGW, Cygwin) */
#else // ndef _MSC_VER
#include <stdint.h>
typedef   int8_t  s8;
typedef  uint8_t  u8;
typedef  int16_t s16;
typedef uint16_t u16;
typedef  int32_t s32;
typedef uint32_t u32;
typedef  int64_t s64;
typedef uint64_t u64;
#endif // ndef _MSC_VER

#endif // #ifndef __HOSTTYPES_H__
