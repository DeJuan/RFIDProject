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
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Module Name:  

    client.cpp

Abstract:  
    Client interface for Human Interface Device (HID) Class.

Functions:

Notes: 

--*/


#include "hidmdd.h"


// Get a report from the device. Note that this is not to be done for
// polling the device on a regular basis. The first byte of pbBuffer must be 
// the ID of the report to receive (even if it is 0).
static
DWORD
WINAPI
HidGetReport(
    HID_HANDLE       hDevice,
    HIDP_REPORT_TYPE type,
    __out_bcount(cbBuffer) PCHAR pbBuffer,
    DWORD            cbBuffer,
    PDWORD           pcbTransferred,
    DWORD            dwTimeout
    )
{
    SETFNAME(_T("HidGetReport"));
    
    PHID_CLIENT_HANDLE pHidClient = (PHID_CLIENT_HANDLE) hDevice;
    PHID_CONTEXT pHidContext;
    BYTE bReportID = 0;
    DWORD dwErr = ERROR_SUCCESS;

    if (VALID_CLIENT_HANDLE(pHidClient) == FALSE) {
        DEBUGMSG(ZONE_ERROR, (_T("%s: Invalid device handle\r\n"), pszFname));
        dwErr = ERROR_INVALID_HANDLE;
        goto EXIT;
    }

    if ( (pbBuffer == NULL) || (pcbTransferred == NULL) || (cbBuffer == 0) ||
         (IS_HID_REPORT_TYPE_VALID(type) == FALSE) ) {
        DEBUGMSG(ZONE_ERROR, (_T("%s: Invalid parameter\r\n"), pszFname));
        dwErr = ERROR_INVALID_PARAMETER;
        goto EXIT;
    }
    

    ValidateClientHandle(pHidClient);
    pHidContext = pHidClient->pHidContext;
    ValidateHidContext(pHidContext);

    __try {
        bReportID = *pbBuffer;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    if (dwErr != ERROR_SUCCESS) { 
        // Exception occurred.
        DEBUGMSG(ZONE_ERROR, (_T("%s: Exception writing to user buffer\r\n"),
            pszFname));
        goto EXIT;
    }

    // Adjust pbBuffer and cbBuffer so report ID is not transferred.
    dwErr = HidPdd_GetReport(pHidContext->hPddDevice, type, pbBuffer + 1, 
        cbBuffer - 1, pcbTransferred, dwTimeout, bReportID);

EXIT:
    return dwErr;
}


// Set a report on the device.
// Returns the error values from IssueVendorTransfer.
static
DWORD
WINAPI
HidSetReport(
    HID_HANDLE       hDevice,
    HIDP_REPORT_TYPE type,
    __in_bcount(cbBuffer) PCHAR pbBuffer,
    DWORD            cbBuffer,
    DWORD            dwTimeout
    )
{
    SETFNAME(_T("HidSetReport"));
    
    PHID_CLIENT_HANDLE pHidClient = (PHID_CLIENT_HANDLE) hDevice;
    PHID_CONTEXT pHidContext;
    BYTE bReportID = 0;
    DWORD dwErr = ERROR_SUCCESS;

    if (VALID_CLIENT_HANDLE(pHidClient) == FALSE) {
        DEBUGMSG(ZONE_ERROR, (_T("%s: Invalid device handle\r\n"), pszFname));
        dwErr = ERROR_INVALID_HANDLE;
        goto EXIT;
    }

    if ( (pbBuffer == NULL) || (cbBuffer == 0) || 
         (IS_HID_REPORT_TYPE_VALID(type) == FALSE) ) {
        DEBUGMSG(ZONE_ERROR, (_T("%s: Invalid parameter\r\n"), pszFname));
        dwErr = ERROR_INVALID_PARAMETER;
        goto EXIT;
    }


    ValidateClientHandle(pHidClient);
    pHidContext = pHidClient->pHidContext;
    ValidateHidContext(pHidContext);

    __try {
        bReportID = *pbBuffer;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    if (dwErr != ERROR_SUCCESS) { 
        // Exception occurred.
        DEBUGMSG(ZONE_ERROR, (_T("%s: Exception writing to user buffer\r\n"),
            pszFname));
        goto EXIT;
    }

    if (bReportID == 0x0)
    {
        dwErr = HidPdd_SetReport(pHidContext->hPddDevice, type, pbBuffer + 1, cbBuffer - 1, dwTimeout, bReportID);
    }
    else
    {
        dwErr = HidPdd_SetReport(pHidContext->hPddDevice, type, pbBuffer, cbBuffer, dwTimeout, bReportID);
    }

EXIT:
    return dwErr;
}


// Get an interrupt report from the device.
static
DWORD
WINAPI 
HidGetInterruptReport(
    HID_HANDLE hDevice,
    PCHAR      pbBuffer,
    DWORD      cbBuffer, 
    PDWORD     pcbTransferred,
    HANDLE     hCancel,
    DWORD      dwTimeout
    )
{
    SETFNAME(_T("HidGetInterruptReport"));
    
    PHID_CLIENT_HANDLE pHidClient = (PHID_CLIENT_HANDLE) hDevice;
    DWORD dwErr;
    
    if (VALID_CLIENT_HANDLE(pHidClient) == FALSE) {
        DEBUGMSG(ZONE_ERROR, (_T("%s: Invalid device handle\r\n"), pszFname));
        dwErr = ERROR_INVALID_HANDLE;
        goto EXIT;
    }

    if ( (pbBuffer == NULL) || (pcbTransferred == NULL) ) {
        dwErr = ERROR_INVALID_PARAMETER;
        goto EXIT;
    }

    ValidateClientHandle(pHidClient);
    ValidateHidContext(pHidClient->pHidContext);

    dwErr = pHidClient->pQueue->Dequeue(pbBuffer, cbBuffer, pcbTransferred, 
        hCancel, dwTimeout);

EXIT:    
    return dwErr;
}


// Get an string from the device. Call with pszBuffer == NULL to get
// the character count required (then add 1 for the NULL terminator).
static
DWORD
WINAPI 
HidGetString(
    HID_HANDLE      hDevice,
    HID_STRING_TYPE stringType,
    DWORD           dwIdx,     // Only used with stringType == HID_STRING_INDEXED 
    __out_ecount(cchBuffer) LPWSTR          pszBuffer, // Set to NULL to get character count
    DWORD           cchBuffer, // Count of chars that will fit into pszBuffer
                               // including the NULL terminator.
    PDWORD          pcchActual // Count of chars in the string NOT including 
                               // the NULL terminator
    )
{
    SETFNAME(_T("HidGetString"));
    
    PHID_CLIENT_HANDLE pHidClient = (PHID_CLIENT_HANDLE) hDevice;
    PHID_CONTEXT pHidContext;
    DWORD dwErr;
    
    if (VALID_CLIENT_HANDLE(pHidClient) == FALSE) {
        DEBUGMSG(ZONE_ERROR, (_T("%s: Invalid device handle\r\n"), pszFname));
        dwErr = ERROR_INVALID_HANDLE;
        goto EXIT;
    }

    if ( (pcchActual == NULL) ||
         ((stringType == HID_STRING_INDEXED) && (dwIdx == 0)) ) {
        DEBUGMSG(ZONE_ERROR, (_T("%s: Invalid parameter\r\n"), pszFname));
        dwErr = ERROR_INVALID_PARAMETER;
        goto EXIT;
    }

    ValidateClientHandle(pHidClient);
    pHidContext = pHidClient->pHidContext;
    ValidateHidContext(pHidContext);

    dwErr = HidPdd_GetString(pHidContext->hPddDevice, stringType, dwIdx, 
        pszBuffer, cchBuffer, pcchActual);

EXIT:
    return dwErr;
}
    

// Get the capacity of the queue for the client's TLC.
static
DWORD
WINAPI
HidGetQueueSize(
    HID_HANDLE hDevice,
    PDWORD     pdwSize
    )
{
    SETFNAME(_T("HidGetQueueSize"));
    
    PHID_CLIENT_HANDLE pHidClient = (PHID_CLIENT_HANDLE) hDevice;
    DWORD dwErr = ERROR_SUCCESS;
    DWORD dwCapacity;
    
    if (VALID_CLIENT_HANDLE(pHidClient) == FALSE) {
        DEBUGMSG(ZONE_ERROR, (_T("%s: Invalid device handle\r\n"), pszFname));
        dwErr = ERROR_INVALID_HANDLE;
        goto EXIT;
    }

    if (pdwSize == NULL) {
        DEBUGMSG(ZONE_ERROR, (_T("%s: Invalid parameter\r\n"), pszFname));
        dwErr = ERROR_INVALID_PARAMETER;
        goto EXIT;
    }

    ValidateClientHandle(pHidClient);
    ValidateHidContext(pHidClient->pHidContext);

    dwCapacity = pHidClient->pQueue->GetCapacity();

    __try {
        *pdwSize = dwCapacity;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        DEBUGMSG(ZONE_ERROR, (_T("%s: Exception writing to user buffer\r\n"), 
            pszFname));
        dwErr = ERROR_INVALID_PARAMETER;
    }
        
EXIT:
    return dwErr;
}
    

// Set the capacity of the queue for the client's TLC.
static
DWORD
WINAPI
HidSetQueueSize(
    HID_HANDLE hDevice,
    DWORD      dwSize
    )
{
    SETFNAME(_T("HidSetQueueSize"));
    
    PHID_CLIENT_HANDLE pHidClient = (PHID_CLIENT_HANDLE) hDevice;
    DWORD dwErr;
    
    if (VALID_CLIENT_HANDLE(pHidClient) == FALSE) {
        DEBUGMSG(ZONE_ERROR, (_T("%s: Invalid device handle\r\n"), pszFname));
        dwErr = ERROR_INVALID_HANDLE;
        goto EXIT;
    }

    if (dwSize == 0) {
        DEBUGMSG(ZONE_ERROR, (_T("%s: Called with invalid size of 0\r\n"), 
            pszFname));
        dwErr = ERROR_INVALID_PARAMETER;
        goto EXIT;
    }


    ValidateClientHandle(pHidClient);
    ValidateHidContext(pHidClient->pHidContext);

    dwErr = pHidClient->pQueue->SetCapacity(dwSize);

EXIT:
    return dwErr;
}


// List of HID functions. Passed to HID clients.
const HID_FUNCS g_HidFuncs =
{
    sizeof(HID_FUNCS), // DWORD dwCount;
    
    &HidGetReport,
    &HidSetReport,
    &HidGetInterruptReport,
    &HidGetString,
    &HidGetQueueSize,
    &HidSetQueueSize,
};


