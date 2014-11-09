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

        CBIT.H

Abstract:

        Control/Bulk/Interrupt Transport
        USB Revison 1.0, Dec. 14, 1998

--*/

#ifndef _CBIT_H_
#define _CBIT_H_

#include "usbmscp.h"

//*****************************************************************************
// D E F I N E S
//*****************************************************************************

#define MAX_CBIT_STALL_COUNT 3

#define CBIT_COMMAND_COMPLETION_INTERRUPT   0x00

#define CBIT_STATUS_SUCCESS             0x00
#define CBIT_STATUS_FAIL                0x01
#define CBIT_STATUS_PHASE_ERROR         0x02
#define CBIT_STATUS_PERSISTENT_ERROR    0x03

//*****************************************************************************
//
// F U N C T I O N    P R O T O T Y P E S
//
//*****************************************************************************

DWORD
CBIT_DataTransfer(
    PUSBMSC_DEVICE     pUsbDevice,
    PTRANSPORT_COMMAND pCommand,
    PTRANSPORT_DATA    pData,
    BOOL               Direction // TRUE = Data-In, else Data-Out
    );

#endif // _CBIT_H_

// EOF
