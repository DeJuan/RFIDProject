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

    debug.cpp

Abstract:  
    Debug for Human Interface Device (HID) Class.

Functions:

Notes: 

--*/

#include "hidmdd.h"


#ifdef DEBUG


//
// ***** Debug utility functions *****
//


// Debug Alloc
HLOCAL
HidAlloc(
    SIZE_T cb
    )
{
    LPVOID ptr;
    
    DEBUGCHK(cb != 0);

    ptr = LocalAlloc(LMEM_FIXED, cb);

    return ptr;
}


// Debug Free
HLOCAL
HidFree(
    LPVOID ptr
    )
{
    DEBUGCHK(ptr != NULL);
    
    return LocalFree(ptr);
}


// IsLocked helper function
BOOL
DebugIsLocked(
    INT cInCS
    )
{
    return cInCS > 0;
}


// Lock helper function
void
DebugLock(
    LPCRITICAL_SECTION pCS,
    INT *pcInCS
    )
{
    DEBUGCHK(*pcInCS < 10); // Check for cycle
    EnterCriticalSection(pCS);
    ++(*pcInCS);
}


// Unlock helper function
void
DebugUnlock(
    LPCRITICAL_SECTION pCS,
    INT *pcInCS
    )
{
    DEBUGCHK(DebugIsLocked(*pcInCS) == TRUE);
    --(*pcInCS);
    LeaveCriticalSection(pCS);
}



// 
// The following are used only by the MDD
//

// Verify the integrity of a Hid context.
void
ValidateHidContext(
    PCHID_CONTEXT pHidContext
    )
{
    DWORD cCollections;
    DWORD dwIdx;
    
    PREFAST_DEBUGCHK(pHidContext != NULL);
    DEBUGCHK(pHidContext->Sig == HID_SIG);
    DEBUGCHK(pHidContext->hPddDevice != NULL);
    DEBUGCHK(pHidContext->hidpDeviceDesc.CollectionDesc != NULL);
    
    cCollections = pHidContext->hidpDeviceDesc.CollectionDescLength;
    
    DEBUGCHK(pHidContext->pQueues != NULL);
    DEBUGCHK(pHidContext->pClientHandles != NULL);
    DEBUGCHK(LocalSize(pHidContext->pClientHandles) >= 
        (sizeof(HID_CLIENT_HANDLE) * cCollections));

    for (dwIdx = 0; dwIdx < cCollections; ++dwIdx) {
        const HidTLCQueue *pQueue = &pHidContext->pQueues[dwIdx];
        
        DEBUGCHK(pQueue == pHidContext->pClientHandles[dwIdx].pQueue);
        pQueue->Validate();

        // Do not call ValidateClientHandle here or infinite loop will result.
    }
}


// Verify the integrity of the HID client handle
void
ValidateClientHandle(
    PHID_CLIENT_HANDLE pHidClient 
    )
{
    DEBUGCHK(pHidClient != NULL);
    DEBUGCHK(pHidClient->Sig == HID_CLIENT_SIG);
    PREFAST_DEBUGCHK(pHidClient->pQueue != NULL);
    pHidClient->pQueue->Validate();
    DEBUGCHK(pHidClient->phidpPreparsedData != NULL);
    DEBUGCHK(pHidClient->pHidContext != NULL);
    ValidateHidContext(pHidClient->pHidContext);
}


// Output a Hid device description
void 
DumpHIDDeviceDescription(
    const HIDP_DEVICE_DESC *phidpDeviceDesc
    )
{
    UINT ui;
    
    PREFAST_DEBUGCHK(phidpDeviceDesc != NULL);
    
    if (ZONE_PARSE)
    {
        DEBUGMSG(ZONE_PARSE, (_T("\r\n")));
        DEBUGMSG(ZONE_PARSE, (_T("Hid Device Description:\r\n")));
        
        for (ui = 0; ui < phidpDeviceDesc->CollectionDescLength; ++ui)
        {
            const HIDP_COLLECTION_DESC *phidpCollDesc = phidpDeviceDesc->CollectionDesc + ui;
            PREFAST_DEBUGCHK(phidpCollDesc != NULL);

            DEBUGMSG(ZONE_PARSE, (_T("Collection #%u: Usage Page 0x%x Usage 0x%x\r\n"), 
                phidpCollDesc->CollectionNumber, phidpCollDesc->UsagePage, 
                phidpCollDesc->Usage));
            DEBUGMSG(ZONE_PARSE, (_T("Input Length 0x%x Output Length 0x%x Feature Length 0x%x\r\n"),
                phidpCollDesc->InputLength, phidpCollDesc->OutputLength, 
                phidpCollDesc->FeatureLength));
        }
        
        for (ui = 0; ui < phidpDeviceDesc->ReportIDsLength; ++ui)
        {
            const HIDP_REPORT_IDS *phidpReports = phidpDeviceDesc->ReportIDs + ui;
            PREFAST_DEBUGCHK(phidpReports != NULL);

            DEBUGMSG(ZONE_PARSE, (_T("Collection #%u, Report #%u\r\n"),
                phidpReports->CollectionNumber, phidpReports->ReportID));
            DEBUGMSG(ZONE_PARSE, (_T("Input Length 0x%x Output Length 0x%x Feature Length 0x%x\r\n"),
                phidpReports->InputLength, phidpReports->OutputLength, 
                phidpReports->FeatureLength));
        }

        DEBUGMSG(ZONE_PARSE, (_T("\r\n")));
    }
}


static const LPCTSTR g_rgpszItems[] =
{
    _T("!RSVD!"),
    _T("Usage Page"), //x01
    _T("Usage"), //x02
    _T("!RSVD!"),
    _T("!RSVD!"),
    _T("Logical Min"), //x05
    _T("Usage Min"), //x06
    _T("!RSVD!"),
    _T("!RSVD!"),
    _T("Logical Max"), //x09
    _T("Usage Max"), //x0a
    _T("!RSVD!"),
    _T("!RSVD!"),
    _T("Physical Min"), //x0d
    _T("Designator Index"), //x0e
    _T("!RSVD!"),
    _T("!RSVD!"),
    _T("Physical Max"), //x11
    _T("Designator Min"), //x12
    _T("!RSVD!"),
    _T("!RSVD!"),
    _T("Unit Exponent"), //x15
    _T("Designator Max"), //x16
    _T("!RSVD!"),
    _T("!RSVD!"),
    _T("Unit"), //x19
    _T("!RSVD!"),
    _T("!RSVD!"),
    _T("!RSVD!"),
    _T("Report Size"), //x1d
    _T("String Index"), //x1e
    _T("!RSVD!"),
    _T("Input"), //x20
    _T("Report ID"), //x21
    _T("String Min"), //x22
    _T("!RSVD!"),
    _T("Output"), //x24
    _T("Report Count"), //x25
    _T("String Max"), //x26
    _T("!RSVD!"),
    _T("Collection"), //x28
    _T("Push"), //x29
    _T("Delimiter"), //x2a
    _T("!RSVD!"),
    _T("Feature"), //x2c
    _T("Pop"), //x2d
    _T("!RSVD!"),
    _T("!RSVD!"),
    _T("End Collection"), //x30
    _T("!RSVD!"),
    _T("!RSVD!"),
    _T("!RSVD!"),
    _T("!RSVD!"),
    _T("!RSVD!"),
    _T("!RSVD!"),
    _T("!RSVD!"),
    _T("!RSVD!"),
    _T("!RSVD!"),
    _T("!RSVD!"),
    _T("!RSVD!"),
    _T("!RSVD!"),
    _T("!RSVD!"),
    _T("!RSVD!"),
    _T("!RSVD!"), //x3F
};


#define HID_LONG_ITEM 0xFE
#define HID_ITEM_LENGTH_MASK 0x03
#define HID_ITEM_TYPE_MASK 0x0C
#define HID_ITEM_TYPE_SHIFT 2
#define HID_ITEM_TAG_MASK 0xF0
#define HID_ITEM_TAG_SHIFT 4

// Retrieves the size of the data from the item byte.
// Returns HID_LONG_ITEM if long data item.
static
BYTE
GetDataSize(
    BYTE bItem
    )
{    
    BYTE bLength;

    if (bItem == HID_LONG_ITEM) {
        bLength = HID_LONG_ITEM;
    }
    else {
        bLength = bItem & HID_ITEM_LENGTH_MASK;

        // 0-2 means 0-2, 3 means 4
        if (bLength == 0x03) {
            bLength = 4;
        }
    }

    return bLength;
}


// Returns the data for this item.
static
DWORD
GetData(
    const BYTE *pbBuffer,
    DWORD dwLength
    )
{
    DWORD dwValue = 0;
    DWORD dwTemp;
    DWORD dwIdx;

    DEBUGCHK(pbBuffer != NULL);
    DEBUGCHK(dwLength <= 4);

    for (dwIdx = 0; dwIdx < dwLength; ++dwIdx) {
        dwTemp = pbBuffer[dwIdx];
        dwValue = (dwTemp << 24) | (dwValue >> 8);
    }

    while (dwIdx < 4) {
        dwValue >>= 8;
        ++dwIdx;
    }

    return dwValue;
}


// Outputs the HID report descriptor in a readable format.
void
DumpHIDReportDescriptor(
    const BYTE *pbReportDescriptor,
    DWORD cbReportDescriptor
    )
{
    static const LPCTSTR rgpszFmtStrings[] = {
        _T(""),
        _T("%02X"),
        _T("%04X"),
        _T(""),
        _T("%08X"),
    };
    
    DWORD dwIdx;
    DWORD cbItemData = 0;

    DEBUGCHK( _countof(g_rgpszItems) == 1 + 
        ((HID_ITEM_TAG_MASK | HID_ITEM_TYPE_MASK) >> HID_ITEM_TYPE_SHIFT) );

    for (dwIdx = 0; dwIdx < cbReportDescriptor; dwIdx += (cbItemData + 1)) {
        BYTE bItem = pbReportDescriptor[dwIdx];
        TCHAR szUnsignedData[9];
        DWORD dwData;

        cbItemData = GetDataSize(bItem);
        if (cbItemData == HID_LONG_ITEM) {
            cbItemData = pbReportDescriptor[dwIdx + 1];
            DEBUGMSG(ZONE_PARSE,
                (_T("RDesc idx %3d (%02X) len=%d -- Long item\r\n"),
                     dwIdx, bItem, cbItemData));
            ++cbItemData; // Skip length byte
            continue;
        }

        dwData = GetData(&pbReportDescriptor[dwIdx + 1], cbItemData);

        BYTE bTagAndType = bItem >> HID_ITEM_TYPE_SHIFT;

        DEBUGCHK(cbItemData < _countof(rgpszFmtStrings));
        VERIFY(SUCCEEDED(StringCchPrintf(szUnsignedData, _countof(szUnsignedData), rgpszFmtStrings[cbItemData], dwData)));

        DEBUGCHK(bTagAndType < _countof(g_rgpszItems));
        DEBUGMSG(ZONE_PARSE,
            (_T("RDesc idx %03X (%02X) len=%d tag=%02X (%-16s) data=%8s\r\n"),
            dwIdx, bItem, cbItemData, bTagAndType,
            g_rgpszItems[bTagAndType], szUnsignedData));
    }
}


#endif // DEBUG


