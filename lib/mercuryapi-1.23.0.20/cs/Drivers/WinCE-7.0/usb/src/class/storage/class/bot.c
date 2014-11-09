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

        BOT.C

Abstract:

        Bulk-Only Transport
        USB Revison 1.0, Sept. 31, 1999

--*/

#include "bot.h"
BOOL BulkTransferMgrTransfer(LPVOID lpContent, LPTRANSFER_NOTIFY_ROUTINE NotifyRoutine,PVOID NotifyContext,DWORD Flags,
        LPVOID pBuffer,DWORD BufferLength);
BOOL BulkTransferClose(LPVOID lpContent);
BOOL BulkTransferMgrGetStatus(LPVOID lpContent, LPDWORD lpByteTransferred , LPDWORD lpdErrors);
BOOL BulkTransferWait(LPVOID lpContent,DWORD dwTicks);

BOOL
BOT_ResetAllPipes(
    PUSBMSC_DEVICE pUsbDevice
    );

DWORD
BOT_ResetEvent(
    HANDLE hEvent
    )
{
    DWORD dwErr = ERROR_SUCCESS;

    if ( hEvent && !ResetEvent(hEvent) ) {
        ASSERT(0);
        dwErr = GetLastError();
        DEBUGMSG( ZONE_ERR,(TEXT("*** ResetEvent ERROR:%d ***\n"), dwErr));
        return dwErr;
    }

    return TRUE;
}


//
// 3.1 Bulk-Only Mass Storage Reset.
// This class-specific command shall ready the device for the next CBW.
//
BOOL
BOT_MassStorageReset(
    PUSBMSC_DEVICE pUsbDevice
    )
{
    USB_DEVICE_REQUEST ControlHeader;
    DWORD dwBytesTransferred = 0;
    DWORD dwErr = 0, dwUsbErr = 0;
    BOOL bRc = FALSE;
    UCHAR count = 0;
    
    INIT_DT;

    DEBUGMSG( ZONE_TRACE, (TEXT("USBMSC>BOT_MassStorageReset\n")));

    START_DT;

    if ( ACCEPT_IO(pUsbDevice) ) {

        ControlHeader.bmRequestType = USB_REQUEST_HOST_TO_DEVICE | USB_REQUEST_CLASS | USB_REQUEST_FOR_INTERFACE;
        ControlHeader.bRequest = 255;
        ControlHeader.wValue = 0;
        ControlHeader.wIndex = pUsbDevice->pUsbInterface->Descriptor.bInterfaceNumber;
        ControlHeader.wLength = 0;
_retry:
        dwErr = IssueVendorTransfer( pUsbDevice->UsbFuncs,
                                     pUsbDevice->hUsbDevice,
                                     DefaultTransferComplete,
                                     pUsbDevice->hEP0Event,
                                     (USB_OUT_TRANSFER|USB_NO_WAIT|USB_SHORT_TRANSFER_OK),
                                     &ControlHeader,
                                     NULL, 0,
                                     &dwBytesTransferred,
                                     pUsbDevice->Timeouts.Reset,
                                     &dwUsbErr );

        if ( ERROR_SUCCESS == dwErr && USB_NO_ERROR == dwUsbErr ) {
            bRc = TRUE;
        } else if (ERROR_SUCCESS == ResetDefaultEndpoint(pUsbDevice->UsbFuncs, pUsbDevice->hUsbDevice) ) {
            if (++count < MAX_BOT_STALL_COUNT) {
                DEBUGMSG( ZONE_BOT, (TEXT("Retry BOT_MassStorageReset\n")));
                Sleep(ONE_FRAME_PERIOD);
                //TEST_TRAP();
                goto _retry;
            }
        }
    }

    STOP_DT( TEXT("BOT_MassStorageReset"), 10, pUsbDevice->Timeouts.Reset );
    
    DEBUGMSG( ZONE_TRACE, (TEXT("USBMSC<BOT_MassStorageReset:%d\n"), bRc) );

    return bRc;
}


//
// 3.2 Get Max LUN
//
DWORD
BOT_GetMaxLUN(
    PUSBMSC_DEVICE pUsbDevice,PUCHAR pLun
    )
{
    USB_DEVICE_REQUEST ControlHeader;
    DWORD dwBytesTransferred = 0;
    UCHAR bMaxLun = 0xFF;
    DWORD dwErr = 0, dwUsbErr = 0;

    INIT_DT;

    DEBUGMSG( ZONE_TRACE, (TEXT("USBMSC>BOT_GetMaxLUN(pUsbDevice%x,pLun=%x\n"),pUsbDevice, pLun));

    START_DT;

    if ( ACCEPT_IO(pUsbDevice) &&  pLun ) {

        ControlHeader.bmRequestType = USB_REQUEST_DEVICE_TO_HOST | USB_REQUEST_CLASS | USB_REQUEST_FOR_INTERFACE;
        ControlHeader.bRequest = 254;
        ControlHeader.wValue = 0;
        ControlHeader.wIndex = pUsbDevice->pUsbInterface->Descriptor.bInterfaceNumber;
        ControlHeader.wLength = 1;

        dwErr = IssueVendorTransfer( pUsbDevice->UsbFuncs,
                                     pUsbDevice->hUsbDevice,
                                     DefaultTransferComplete,
                                     pUsbDevice->hEP0Event,
                                     (USB_IN_TRANSFER|USB_NO_WAIT|USB_SHORT_TRANSFER_OK),
                                     &ControlHeader,
                                     &bMaxLun, 0,
                                     &dwBytesTransferred,
                                     pUsbDevice->Timeouts.CommandBlock,
                                     &dwUsbErr );

        if ( ERROR_SUCCESS != dwErr || USB_NO_ERROR != dwUsbErr ) {
            
            ResetDefaultEndpoint(pUsbDevice->UsbFuncs, pUsbDevice->hUsbDevice);
                    
        }
        else {
            if (0 <= bMaxLun && bMaxLun < MAX_LUN) {
                *pLun = bMaxLun + 1;
            }
            else {
                *pLun = 1;
            }
        }

    } 

    STOP_DT( TEXT("BOT_GetMaxLUN"), 10, pUsbDevice->Timeouts.CommandBlock );

    DEBUGMSG( ZONE_TRACE, (TEXT("USBMSC<BOT_GetMaxLUN:%d\n"), bMaxLun ));

    return dwErr;
}


//
// 5.3.4 Reset Recovery
//
BOOL
BOT_ResetRecovery(
    PUSBMSC_DEVICE pUsbDevice
    )
{
    BOOL bRc = FALSE;
    DWORD dwErr;

    INIT_DT;

    DEBUGMSG(ZONE_TRACE,(TEXT("USBMSC>BOT_ResetRecovery\n")));

    START_DT;
    
    // 0. reset endpoint 0
    dwErr = ResetDefaultEndpoint(pUsbDevice->UsbFuncs, 
                                 pUsbDevice->hUsbDevice);
    
    if ( ERROR_SUCCESS == dwErr ) 
    {
        // 1. send class command
        BOT_MassStorageReset(pUsbDevice);

        // 2. reset BulkIn endpoint
        dwErr = ResetBulkEndpoint(pUsbDevice->UsbFuncs, 
                                  pUsbDevice->hUsbDevice,
                                  pUsbDevice->BulkIn.hPipe,
                                  DefaultTransferComplete,
                                  pUsbDevice->BulkIn.hEvent,
                                  pUsbDevice->BulkIn.bIndex,
                                  pUsbDevice->Timeouts.Reset);
        
        if ( ERROR_SUCCESS == dwErr) 
        {
            // 3. reset BulkOut endpoint
            dwErr = ResetBulkEndpoint(pUsbDevice->UsbFuncs, 
                                      pUsbDevice->hUsbDevice,
                                      pUsbDevice->BulkOut.hPipe,
                                      DefaultTransferComplete,
                                      pUsbDevice->BulkOut.hEvent,
                                      pUsbDevice->BulkOut.bIndex,
                                      pUsbDevice->Timeouts.Reset );

            if ( ERROR_SUCCESS == dwErr ) {
                bRc = TRUE;
            }  //else TEST_TRAP();

        }  //else TEST_TRAP();
        //}
    } //else TEST_TRAP();

    BOT_ResetEvent(pUsbDevice->hEP0Event);
    BOT_ResetEvent(pUsbDevice->BulkIn.hEvent);
    BOT_ResetEvent(pUsbDevice->BulkOut.hEvent);

    STOP_DT( TEXT("BOT_ResetRecovery"), 10, pUsbDevice->Timeouts.Reset );
    
    DEBUGMSG(ZONE_TRACE,(TEXT("USBMSC<BOT_ResetRecovery:%d\n"), bRc));
   
    return bRc;
}



//
// Reset All Pipes
//
BOOL
BOT_ResetAllPipes(
    PUSBMSC_DEVICE pUsbDevice
    )
{
   BOOL bRc = FALSE;
   DWORD dwErr;

   INIT_DT;

   DEBUGMSG(ZONE_TRACE,(TEXT("USBMSC>BOT_ResetAllPipes\n")));

   START_DT

   // 0. reset endpoint 0
   dwErr = ResetDefaultEndpoint(pUsbDevice->UsbFuncs, 
                                pUsbDevice->hUsbDevice);

   if ( ERROR_SUCCESS == dwErr ) 
   {
        // 1. reset BulkIn endpoint
        if ( ResetPipe(pUsbDevice->UsbFuncs,
                       pUsbDevice->BulkIn.hPipe,
                       USB_NO_WAIT) ) 
        {
            // 2. reset BulkOut endpoint
            if ( ResetPipe(pUsbDevice->UsbFuncs,
                           pUsbDevice->BulkOut.hPipe,
                           USB_NO_WAIT) ) 
            {               
                bRc = TRUE;
            }
        }
    }

    BOT_ResetEvent(pUsbDevice->hEP0Event);
    BOT_ResetEvent(pUsbDevice->BulkIn.hEvent);
    BOT_ResetEvent(pUsbDevice->BulkOut.hEvent);

    STOP_DT( TEXT("BOT_ResetAllPipes"), 10, pUsbDevice->Timeouts.Reset );  

    DEBUGMSG(ZONE_TRACE,(TEXT("USBMSC<BOT_ResetAllPipes:%d\n"), bRc));
   
    return bRc;
}


//
// See 5.3 Data Transfer Conditions
//
DWORD
BOT_DataTransfer(
    PUSBMSC_DEVICE           pUsbDevice,
    PTRANSPORT_COMMAND       pCommand,
    OPTIONAL PTRANSPORT_DATA pData, // OPTIONAL
    BOOL                     Direction // TRUE = Data-In, else Data-Out
    )
{
    // Command Block Wrapper
    CBW Cbw;
    DWORD dwCbwSize = sizeof(CBW);

    // Command Status Wrapper
    CSW Csw;
    DWORD dwCswSize = sizeof(CSW);

    DWORD dwBytesTransferred = 0;   
    DWORD dwErr = ERROR_SUCCESS;
    DWORD dwUsbErr = USB_NO_ERROR;
    DWORD dwResetErr, dwCswErr;

    BOOL  bRc = FALSE;
    UCHAR ucStallCount;

    INIT_DT;

    DEBUGMSG(ZONE_TRACE,(TEXT("USBMSC>BOT_DataTransfer\n")));

    // parameter check
    if ( !pCommand || !pCommand->CommandBlock || pCommand->Length > MAX_CBWCB_SIZE ) {
        dwErr = ERROR_INVALID_PARAMETER;
        DEBUGMSG(ZONE_ERR,(TEXT("BOT_DataTransfer error:%d\n"),dwErr));
        SetLastError(dwErr);
        return dwErr;
    }

    if ( !ACCEPT_IO(pUsbDevice) ) {
        dwErr = ERROR_ACCESS_DENIED;
        DEBUGMSG( ZONE_ERR,(TEXT("BOT_DataTransfer error:%d\n"), dwErr));
        SetLastError(dwErr);
        return dwErr;
    }

    //
    // We require exclusive entry into the transport.
    // we could implement command queuing.
    //
    EnterCriticalSection(&pUsbDevice->Lock);

    if ( !BOT_ResetAllPipes(pUsbDevice) ) {
        DEBUGMSG( ZONE_ERR, (TEXT("BOT_ResetAllPipes failed!\n")));
        dwErr = ERROR_GEN_FAILURE;
        goto BOT_SendCommandDone;
    }

    ucStallCount = 0;

    pUsbDevice->dwCurTag++;

    memset( &Cbw, 0, dwCbwSize );
    memset( &Csw, 0, dwCswSize );

    //
    // build the active CBW
    //        
    Cbw.dCBWSignature = CBW_SIGNATURE;
    Cbw.dCBWTag = pUsbDevice->dwCurTag;    
    Cbw.dCBWDataTransferLength = pData ? pData->RequestLength : 0;

    if (Direction) {
        Cbw.bmCBWFlags |= 0x80;
    }
    
    Cbw.bCBWLUN = (BYTE)(pCommand->dwLun & 0xf); // TBD
    Cbw.bCBWCBLength = (UCHAR)pCommand->Length;
    memcpy( &Cbw.CBWCB, pCommand->CommandBlock, pCommand->Length);

    //
    // 5.3.1 Command Block Transport
    //
    DEBUGMSG(ZONE_BOT,(TEXT("5.3.1 CBW\n")));
    
    START_DT;

    dwErr = IssueBulkTransfer( pUsbDevice->UsbFuncs,
                               pUsbDevice->BulkOut.hPipe,
                               DefaultTransferComplete,               // Callback
                               pUsbDevice->BulkOut.hEvent,    // Callback Context
                               USB_OUT_TRANSFER|USB_NO_WAIT|USB_SHORT_TRANSFER_OK,// Flags
                              &Cbw, 0,
                               dwCbwSize,
                              &dwBytesTransferred,
                               pUsbDevice->Timeouts.CommandBlock,
                              &dwUsbErr );
    
    STOP_DT( TEXT("CBW"), 10, pUsbDevice->Timeouts.CommandBlock);

    if ( ERROR_SUCCESS != dwErr || USB_NO_ERROR != dwUsbErr || dwBytesTransferred != dwCbwSize ) {

        DEBUGMSG( ZONE_ERR, (TEXT("BOT_DataTransfer error(5.3.1, %d, 0x%x, %d, %d, %d)\n"), 
            dwErr, dwUsbErr, dwBytesTransferred, dwCbwSize, pUsbDevice->Timeouts.CommandBlock));
        
        bRc = BOT_ResetRecovery(pUsbDevice);
        
        goto BOT_SendCommandDone;
    }


    //
    // 5.3.2 Data Transport
    //
    if (pData && pData->DataBlock && pData->RequestLength) 
    {
        PIPE pipeObj = Direction ? pUsbDevice->BulkIn : pUsbDevice->BulkOut;
        DWORD dwFlags = Direction ? (USB_IN_TRANSFER|USB_NO_WAIT|USB_SHORT_TRANSFER_OK) : (USB_OUT_TRANSFER|USB_NO_WAIT|USB_SHORT_TRANSFER_OK);
        BOOL fRet ;
        DWORD dwTimeout = pCommand->Timeout;
        DWORD dwUsbErr = USB_NOT_COMPLETE_ERROR;
        DWORD dwTransferLength = 0 ;
        ResetEvent(pipeObj.hEvent);
        DEBUGMSG(ZONE_BOT,(TEXT("5.3.2 Data%sTransport - dwDataLength:%d, TimeOut:%d \n"), Direction ? TEXT("In") : TEXT("Out"), pData->RequestLength, dwTimeout ));
        
        fRet = BulkTransferMgrTransfer(pipeObj.pAsyncContent, DefaultTransferComplete, pipeObj.hEvent, 
            dwFlags, pData->DataBlock, pData->RequestLength );
        if (fRet ) {
            if (!BulkTransferWait(pipeObj.pAsyncContent,dwTimeout)) {
                fRet = BulkTransferMgrGetStatus(pipeObj.pAsyncContent, &dwTransferLength , &dwUsbErr);
                dwUsbErr = USB_NOT_COMPLETE_ERROR;
            }
            else {
                fRet = BulkTransferMgrGetStatus(pipeObj.pAsyncContent, &dwTransferLength , &dwUsbErr);
            }
            BulkTransferClose(pipeObj.pAsyncContent);
        }
        pData->TransferLength  = dwTransferLength;
        
        if ( USB_NO_ERROR != dwUsbErr ) {

            UCHAR bIndex = Direction ? pUsbDevice->BulkIn.bIndex : pUsbDevice->BulkOut.bIndex;
            
            // 6.7.x.3: the host shall clear the Bulk pipe
            DEBUGMSG( ZONE_ERR, (TEXT("BOT_DataTransfer warning(6.7.x.3, RequestLength:%d TransferLength:%d Err:%d UsbErr:0x%x, dwTimeout:%d)\n"), 
                    pData->RequestLength, dwTransferLength, dwErr, dwUsbErr, dwTimeout ));

            // test/reset Bulk endpoint
            dwResetErr = ResetBulkEndpoint( pUsbDevice->UsbFuncs,
                                            pUsbDevice->hUsbDevice,
                                            pipeObj.hPipe,
                                            DefaultTransferComplete,
                                            pipeObj.hEvent,
                                            bIndex,
                                            pUsbDevice->Timeouts.Reset );

            if (ERROR_SUCCESS != dwResetErr) {
                DEBUGMSG( ZONE_ERR, (TEXT("ResetBulkEndpoint.1 ERROR:%d\n"), dwResetErr));
                
                bRc = BOT_ResetRecovery(pUsbDevice);

                if (ERROR_SUCCESS == dwErr)
                    dwErr = ERROR_GEN_FAILURE;

                goto BOT_SendCommandDone;
            }

        }

    }

    //
    // 5.3.3 Command Status Transport (CSW)
    //
    ucStallCount = 0; // reset Stall count

    DEBUGMSG(ZONE_BOT,(TEXT("5.3.3 CSW\n")));

_RetryCSW:
    START_DT;

    dwCswErr = IssueBulkTransfer( pUsbDevice->UsbFuncs,
                                  pUsbDevice->BulkIn.hPipe,
                                  DefaultTransferComplete,      // Callback
                                  pUsbDevice->BulkIn.hEvent,    // Callback Context
                                  (USB_IN_TRANSFER|USB_NO_WAIT|USB_SHORT_TRANSFER_OK), // Flags
                                 &Csw, 0,
                                  dwCswSize,
                                 &dwBytesTransferred,
                                  pUsbDevice->Timeouts.CommandStatus,
                                 &dwUsbErr );

    STOP_DT( TEXT("CSW"), 100, (Direction ? pUsbDevice->Timeouts.CommandStatus : pCommand->Timeout) );

    // Figure 2 - Status Transport Flow
    if ( ERROR_SUCCESS != dwCswErr || USB_NO_ERROR != dwUsbErr ) {

        DEBUGMSG( ZONE_ERR, (TEXT("BOT_DataTransfer error(5.3.3, dwCswErr:%d, dwUsbErr:0x%x, dwTimeout:%d)\n"), 
                dwCswErr, dwUsbErr, pUsbDevice->Timeouts.CommandStatus ));

        START_DT;

        // reset BulkIn endpoint
        dwResetErr = ResetBulkEndpoint(pUsbDevice->UsbFuncs,
                                       pUsbDevice->hUsbDevice,
                                       pUsbDevice->BulkIn.hPipe,
                                       DefaultTransferComplete,
                                       pUsbDevice->BulkIn.hEvent,
                                       pUsbDevice->BulkIn.bIndex,
                                       pUsbDevice->Timeouts.Reset );

        STOP_DT( TEXT("ResetBulkEndpoint.2"), 10, pUsbDevice->Timeouts.Reset );

        if ( ERROR_SUCCESS == dwResetErr &&
             ++ucStallCount < MAX_BOT_STALL_COUNT ) {

            DEBUGMSG( ZONE_BOT, (TEXT("Retry CSW\n")));
            
            Sleep(ONE_FRAME_PERIOD);

            goto _RetryCSW;

        } else {
            DEBUGMSG( ZONE_ERR, (TEXT("BOT_DataTransfer error(5.3.3, ResetErr:0x%x StallCount:%d)\n"), dwResetErr, ucStallCount));

            bRc = BOT_ResetRecovery(pUsbDevice);

            if (ERROR_SUCCESS == dwErr)
                dwErr = ERROR_GEN_FAILURE;

            goto BOT_SendCommandDone;
        }
    }

    //
    // Validate CSW...
    //

    // 6.3.1 Valid CSW: size
    if ( dwBytesTransferred != dwCswSize ) { 
        DEBUGMSG( ZONE_ERR, (TEXT("Invalid Csw size: %d, %d\n"), dwBytesTransferred, dwCswSize ));

        // 6.5 Host shall perform ResetRecovery for invalid CSW
        bRc = BOT_ResetRecovery(pUsbDevice);

        if (ERROR_SUCCESS == dwErr)
            dwErr = ERROR_GEN_FAILURE;

        goto BOT_SendCommandDone;
    }


    // 6.3.1 Valid CSW: Signature
    if ( CSW_SIGNATURE != Csw.dCSWSignature) { 
        DEBUGMSG( ZONE_ERR, (TEXT("Invalid Csw.dCSWSignature:0x%x\n"), Csw.dCSWSignature ));
        
        // 6.5 Host shall perform ResetRecovery for invalid CSW
        bRc = BOT_ResetRecovery(pUsbDevice);

        if (ERROR_SUCCESS == dwErr)
            dwErr = ERROR_GEN_FAILURE;

        goto BOT_SendCommandDone;
    }

    // 6.3.1 Valid CSW: Tags
    if ( Cbw.dCBWTag != Csw.dCSWTag ) {
        DEBUGMSG( ZONE_ERR, (TEXT("Mismatched Tags Cbw:0x%x Csw:0x%x\n"), Cbw.dCBWTag, Csw.dCSWTag ));

        // 6.5 Host shall perform ResetRecovery for invalid CSW
        bRc = BOT_ResetRecovery(pUsbDevice);

        if (ERROR_SUCCESS == dwErr)
            dwErr = ERROR_GEN_FAILURE;

        goto BOT_SendCommandDone;
    }

    //
    // Command Status?
    //
    if ( 0 == Csw.bCSWStatus || 1 == Csw.bCSWStatus ) {

        if (1 == Csw.bCSWStatus ) {
            DEBUGMSG( ZONE_BOT, (TEXT("Command Block Status: Command Failed\n")));
            dwErr = ERROR_GEN_FAILURE;
        }

        goto BOT_SendCommandDone;
    }

    //
    // Phase Error?
    //
    if ( 2 == Csw.bCSWStatus ) {
        DEBUGMSG( ZONE_ERR, (TEXT("Command Block Status: Phase Error\n")));

        // ignore the dCSWDataResidue
        bRc = BOT_ResetRecovery(pUsbDevice);

        if (ERROR_SUCCESS == dwErr)
            dwErr = ERROR_GEN_FAILURE;
        
        goto BOT_SendCommandDone;
    }


BOT_SendCommandDone:
    //
    // cleanup        
    //
    LeaveCriticalSection(&pUsbDevice->Lock);
   
    DEBUGMSG(ZONE_TRACE,(TEXT("USBMSC<BOT_DataTransfer:%d\n"), dwErr));    

    return dwErr;
}


//
// Set a DWORD Value into an array of chars pointed to by pbArray
// (Little Endian)
//
VOID
SetDWORD(
    __out_ecount(4) PUCHAR pbArray,
    IN DWORD dwValue
    ) 
{
    if (pbArray) {
        pbArray[3] = (UCHAR)dwValue;
        dwValue >>= 8;
        pbArray[2] = (UCHAR)dwValue;
        dwValue >>= 8;
        pbArray[1] = (UCHAR)dwValue;
        dwValue >>= 8;
        pbArray[0] = (UCHAR)dwValue;
    }
    return;
}

VOID 
SetWORD(
    __out_ecount(2) PBYTE pBytes,
    WORD wValue 
    )
{
    if (pBytes) {
        pBytes[1] = (BYTE)wValue;
        wValue >>= 8;
        pBytes[0] = (BYTE)wValue;
    }
    return;
}


//
// Gets a USHORT Value from an array of chars pointed to by pbArray.
// The return value is promoted to DWORD.
// (Little Endian)
//
DWORD 
GetDWORD(
    __inout_ecount(4) const UCHAR *pbArray
    )
{
    DWORD dwReturn=0;
    if (pbArray) {
        dwReturn=*(pbArray++);
        dwReturn=dwReturn*0x100 + *(pbArray++);
        dwReturn=dwReturn*0x100 + *(pbArray++);
        dwReturn=dwReturn*0x100 + *(pbArray++);
    }
    return dwReturn;
}

// EOF
