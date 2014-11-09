//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this sample source code is subject to the terms of the Microsoft
// license agreement under which you licensed this sample source code. If
// you did not accept the terms of the license agreement, you are not
// authorized to use this sample source code. For the terms of the license,
// please see the license agreement between you and Microsoft or, if applicable,
// see the LICENSE.RTF on your install media or the root of your tools installation.
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Module Name:  

serdbg.h

Abstract:

Serial debugging support

Notes: 


--*/

#ifndef __SERDBG_H__
#define __SERDBG_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef DEBUG
#define ZONE_INIT		DEBUGZONE(0)
#define ZONE_OPEN		DEBUGZONE(1)
#define ZONE_READ		DEBUGZONE(2)
#define ZONE_WRITE		DEBUGZONE(3)
#define ZONE_CLOSE		DEBUGZONE(4)
#define ZONE_IOCTL		DEBUGZONE(5)
#define ZONE_THREAD		DEBUGZONE(6)
#define ZONE_EVENTS		DEBUGZONE(7)
#define ZONE_CRITSEC	DEBUGZONE(8)
#define ZONE_FLOW		DEBUGZONE(9)
#define ZONE_IR			DEBUGZONE(10)
#define ZONE_USR_READ	DEBUGZONE(11)
#define ZONE_ALLOC		DEBUGZONE(12)
#define ZONE_FUNCTION	DEBUGZONE(13)
#define ZONE_WARN		DEBUGZONE(14)
#define ZONE_ERROR		DEBUGZONE(15)

// unofficial zones - The upper 16 zones don't show up with nice
// names in cesh, etc. because we only store mnemonics for the first
// 16 zones in DBGPARAM.  But for convenience, all of my serial drivers
// use the upper 16 bits consistently as defined below.
#define ZONE_RXDATA		DEBUGZONE(16)
#define ZONE_TXDATA		DEBUGZONE(17)
#else
#define ZONE_INIT       0
#define ZONE_OPEN       0
#define ZONE_READ       0
#define ZONE_WRITE      0
#define ZONE_CLOSE      0
#define ZONE_IOCTL      0
#define ZONE_THREAD     0
#define ZONE_EVENTS     0
#define ZONE_CRITSEC    0
#define ZONE_FLOW       0
#define ZONE_IR         0
#define ZONE_USR_READ   0
#define ZONE_ALLOC      0
#define ZONE_FUNCTION   0
#define ZONE_WARN       0
#define ZONE_ERROR      0
#define ZONE_RXDATA     0
#define ZONE_TXDATA     0
#endif // DEBUG

#ifdef __cplusplus
}
#endif

#endif /* __SERDBG_H__ */
