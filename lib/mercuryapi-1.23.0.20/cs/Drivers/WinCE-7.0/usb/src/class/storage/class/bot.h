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

        BOT.H

Abstract:

        Bulk-Only Transport
        USB Revison 1.0, Sept. 31, 1999

--*/

#ifndef _BOT_H_
#define _BOT_H_

#include "usbmscp.h"

//*****************************************************************************
// D E F I N E S
//*****************************************************************************

#define MAX_BOT_STALL_COUNT 2

#define MAX_CBWCB_SIZE 16

//
// Command Block Wrapper Signature 'USBC'
//
#define CBW_SIGNATURE               0x43425355
#define CBW_FLAGS_DATA_IN           0x80
#define CBW_FLAGS_DATA_OUT          0x00

//
// Command Status Wrapper Signature 'USBS'
//
#define CSW_SIGNATURE               0x53425355
#define CSW_STATUS_GOOD             0x00
#define CSW_STATUS_FAILED           0x01
#define CSW_STATUS_PHASE_ERROR      0x02


//*****************************************************************************
// T Y P E D E F S
//*****************************************************************************

#pragma pack (push, 1)
//
// Command Block Wrapper
//
typedef struct _CBW
{
    ULONG   dCBWSignature; // 0-3

    ULONG   dCBWTag;    // 4-7

    ULONG   dCBWDataTransferLength; // 8-11

    UCHAR   bmCBWFlags; // 12

    UCHAR   bCBWLUN; // 13

    UCHAR   bCBWCBLength; // 14

    UCHAR   CBWCB[MAX_CBWCB_SIZE]; // 15-30

} CBW, *PCBW;

//
// Command Status Wrapper
//
typedef struct _CSW
{
    ULONG   dCSWSignature; // 0-3

    ULONG   dCSWTag; // 4-7

    ULONG   dCSWDataResidue; // 8-11

    UCHAR   bCSWStatus; // 12

} CSW, *PCSW;

#pragma pack (pop)


//*****************************************************************************
//
// F U N C T I O N    P R O T O T Y P E S
//
//*****************************************************************************

DWORD
BOT_DataTransfer(
    PUSBMSC_DEVICE     pUsbDevice,
    PTRANSPORT_COMMAND pCommand,
    PTRANSPORT_DATA    pData,
    BOOL               Direction // TRUE = Data-In, else Data-Out
    );

DWORD
BOT_GetMaxLUN(
    PUSBMSC_DEVICE pUsbDevice,PUCHAR pLun
    );

#endif // _BOT_H_

// EOF
