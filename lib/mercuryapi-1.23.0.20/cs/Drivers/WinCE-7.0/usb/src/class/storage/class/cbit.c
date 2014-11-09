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

        CBIT.C

Abstract:

        Control/Bulk/Interrupt Transport
        USB Revison 1.0, Dec. 14, 1998

--*/

#include "cbit.h"
BOOL BulkTransferMgrTransfer(LPVOID lpContent, LPTRANSFER_NOTIFY_ROUTINE NotifyRoutine,PVOID NotifyContext,DWORD Flags,
        LPVOID pBuffer,DWORD BufferLength);
BOOL BulkTransferClose(LPVOID lpContent);
BOOL BulkTransferMgrGetStatus(LPVOID lpContent, LPDWORD lpByteTransferred , LPDWORD lpdErrors);
BOOL BulkTransferWait(LPVOID lpContent,DWORD dwTicks);

DWORD
CBIT_CommandTransport(
    PUSBMSC_DEVICE     pUsbDevice,
    PTRANSPORT_COMMAND pCommand,
    PDWORD             pdwBytesTransferred
    );

BOOL
CBIT_ResetAllPipes(
    PUSBMSC_DEVICE pUsbDevice
    );

DWORD
CBIT_StatusTransport(
    PUSBMSC_DEVICE     pUsbDevice
    );


DWORD
CBIT_ResetEvent(
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
// 2.2 Command Block Reset.
//
// Note: CBIT 1.0 sections
//      2.3.2.4 Indefinite Delay, and
//      2.3.2.6 Persistant Command Block Failure 
// in the spec create a failure recovery problem on WCE devices
// since we can not yet suspend or perform a port reset.
//
DWORD
CBIT_CommandBlockReset(
    PUSBMSC_DEVICE pUsbDevice
    )
{
    TRANSPORT_COMMAND tCommand = {0};
    UCHAR             bCommandBlock[12];
        
    DWORD dwErr = 0, dwBytesTransferred = 0;
    
    DEBUGMSG(ZONE_TRACE,(TEXT("USBMSC>CBIT_CommandBlockReset\n")));

    tCommand.Flags   = DATA_OUT;
    tCommand.Timeout = pUsbDevice->Timeouts.CommandBlock;
    tCommand.Length  = USBMSC_SUBCLASS_SCSI == pUsbDevice->pUsbInterface->Descriptor.bInterfaceSubClass ? 6 : 12;
    tCommand.CommandBlock = bCommandBlock;

    memset( bCommandBlock, 0xFF, sizeof(bCommandBlock));  
    bCommandBlock[0] = 0x1D;
    bCommandBlock[1] = 0x04;

    dwErr = CBIT_CommandTransport( pUsbDevice,
                                   &tCommand,
                                   &dwBytesTransferred );

    if ( ERROR_SUCCESS == dwErr ) {
        dwErr = CBIT_StatusTransport( pUsbDevice );
    
    }

    //
    // The host shall send ClearFeature on both Bulk-In and Bulk-Out pipes,
    // which is done in Reset Recovery
    //

    DEBUGMSG(ZONE_TRACE,(TEXT("USBMSC<CBIT_CommandBlockReset:%d\n"), dwErr));

    return dwErr;
}


//
// Reset Recovery
//
BOOL
CBIT_ResetRecovery(
    PUSBMSC_DEVICE pUsbDevice
    )
{
    BOOL bRc = FALSE;
    DWORD dwErr;

    INIT_DT;

    DEBUGMSG(ZONE_TRACE,(TEXT("USBMSC>CBIT_ResetRecovery\n")));

    START_DT;
    
    // 0. reset endpoint 0
    dwErr = ResetDefaultEndpoint(pUsbDevice->UsbFuncs, 
                                 pUsbDevice->hUsbDevice);
    
    if ( ERROR_SUCCESS == dwErr ) 
    {
        // 1. send class command
        // CBI1.0: 2.2 - After a Command Block Reset completes, the Stall condition and endpoint's data toggle
        // are undefined. The host shall send Clear Feature to both bulk in and bulk out.
//        dwErr = CBIT_CommandBlockReset(pUsbDevice);
//        if ( ERROR_SUCCESS != dwErr )
//            TEST_TRAP();

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

            if ( ERROR_SUCCESS == dwErr && pUsbDevice->Interrupt.hPipe) 
            {
                // 4. reset Interrupt endpoint
                dwErr = ResetBulkEndpoint(pUsbDevice->UsbFuncs,
                                          pUsbDevice->hUsbDevice,
                                          pUsbDevice->Interrupt.hPipe,
                                          DefaultTransferComplete,
                                          pUsbDevice->Interrupt.hEvent,
                                          pUsbDevice->Interrupt.bIndex,
                                          pUsbDevice->Timeouts.Reset );
            }
        }
    }

    if (ERROR_SUCCESS == dwErr) {
        bRc = TRUE;
    }

    CBIT_ResetEvent(pUsbDevice->hEP0Event);
    CBIT_ResetEvent(pUsbDevice->BulkIn.hEvent);
    CBIT_ResetEvent(pUsbDevice->BulkOut.hEvent);
    CBIT_ResetEvent(pUsbDevice->Interrupt.hEvent);

    STOP_DT( TEXT("CBIT_ResetRecovery"), 10, pUsbDevice->Timeouts.Reset );

    DEBUGMSG(ZONE_TRACE,(TEXT("USBMSC<CBIT_ResetRecovery:%d\n"), bRc));
   
    return bRc;
}



//
// Reset All Pipes
//
BOOL
CBIT_ResetAllPipes(
    PUSBMSC_DEVICE pUsbDevice
    )
{
   BOOL bRc = FALSE;
   DWORD dwErr;

   INIT_DT;

   DEBUGMSG(ZONE_TRACE,(TEXT("USBMSC>CBIT_ResetAllPipes\n")));

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
                // 3. reset Interrupt endpoint
                bRc = pUsbDevice->Interrupt.hPipe ? ResetPipe(pUsbDevice->UsbFuncs,pUsbDevice->Interrupt.hPipe,USB_NO_WAIT) : TRUE;
            }
        }
    }

    CBIT_ResetEvent(pUsbDevice->hEP0Event);
    CBIT_ResetEvent(pUsbDevice->BulkIn.hEvent);
    CBIT_ResetEvent(pUsbDevice->BulkOut.hEvent);
    CBIT_ResetEvent(pUsbDevice->Interrupt.hEvent);

    STOP_DT( TEXT("CBIT_ResetAllPipes"), 100, pUsbDevice->Timeouts.Reset );  

    DEBUGMSG(ZONE_TRACE,(TEXT("USBMSC<CBIT_ResetAllPipes:%d\n"), bRc));
   
    return bRc;
}


//
// 4.1 Accept Device-Specific Command (ADSC)
//
DWORD
CBIT_CommandTransport(
    PUSBMSC_DEVICE     pUsbDevice,
    PTRANSPORT_COMMAND pCommand,
    PDWORD             pdwBytesTransferred
    )
{
    USB_DEVICE_REQUEST ADSC = {0};
    DWORD dwErr = 0, dwUsbErr = 0;
    UCHAR count = 0;
    
    INIT_DT;

    DEBUGMSG( ZONE_TRACE, (TEXT("USBMSC>CBIT_CommandTransport\n")));

    if ( !pCommand || !pCommand->CommandBlock || !pdwBytesTransferred ) {
        dwErr = ERROR_INVALID_PARAMETER;
        DEBUGMSG(ZONE_ERR,(TEXT("CBIT_CommandTransport error:%d\n"),dwErr));
        SetLastError(dwErr);
        return dwErr;
    }

    if ( !ACCEPT_IO(pUsbDevice) ) {
        dwErr = ERROR_ACCESS_DENIED;
        DEBUGMSG( ZONE_ERR,(TEXT("CBIT_CommandTransport error:%d\n"), dwErr));
        SetLastError(dwErr);
        return dwErr;
    }

    START_DT;

    // ADSC packet
    ADSC.bmRequestType = USB_REQUEST_HOST_TO_DEVICE | USB_REQUEST_CLASS | USB_REQUEST_FOR_INTERFACE;
    ADSC.bRequest = 0;
    ADSC.wValue   = 0;
    ADSC.wIndex   = pUsbDevice->pUsbInterface->Descriptor.bInterfaceNumber;
    ADSC.wLength  = (USHORT)pCommand->Length;

_retry:
    dwErr = IssueVendorTransfer( pUsbDevice->UsbFuncs,
                                 pUsbDevice->hUsbDevice,
                                 DefaultTransferComplete,
                                 pUsbDevice->hEP0Event,
                                 (USB_OUT_TRANSFER|USB_NO_WAIT|USB_SHORT_TRANSFER_OK),
                                 &ADSC,
                                 pCommand->CommandBlock, 0,
                                 pdwBytesTransferred,
                                 pUsbDevice->Timeouts.CommandBlock,
                                 &dwUsbErr );

    if ( ERROR_SUCCESS != dwErr || USB_NO_ERROR != dwUsbErr || *pdwBytesTransferred != pCommand->Length ) {

        DEBUGMSG( ZONE_ERR, (TEXT("CBIT_CommandTransport error(%d, 0x%x, %d, %d, %d)\n"), 
            dwErr, dwUsbErr, *pdwBytesTransferred, pCommand->Length, pUsbDevice->Timeouts.CommandBlock));

        if (ERROR_SUCCESS == ResetDefaultEndpoint(pUsbDevice->UsbFuncs, pUsbDevice->hUsbDevice) ) {
            if (++count < MAX_CBIT_STALL_COUNT) {
                DEBUGMSG( ZONE_CBIT, (TEXT("Retry CBIT_CommandTransport\n")));
                Sleep(ONE_FRAME_PERIOD);
                goto _retry;
            }
        }

        if ( USB_NO_ERROR != dwUsbErr && ERROR_SUCCESS == dwErr) {
            dwErr = ERROR_GEN_FAILURE;
        }
    }

    STOP_DT( TEXT("CBIT_CommandTransport"), 1000, pUsbDevice->Timeouts.CommandBlock );
    
    DEBUGMSG( ZONE_TRACE, (TEXT("USBMSC<CBIT_CommandTransport:%d\n"), dwErr) );

    return dwErr;
}


DWORD
CBIT_StatusTransport(
    PUSBMSC_DEVICE     pUsbDevice
    )
{
    DWORD dwBytesTransferred = 0;   
    DWORD dwErr = ERROR_SUCCESS;
    DWORD dwUsbErr = USB_NO_ERROR;

    UCHAR  ucStatus[2];
    DWORD  dwStatusSize = sizeof(ucStatus);
    
    if ( !ACCEPT_IO(pUsbDevice) ) {
        dwErr = ERROR_ACCESS_DENIED;
        DEBUGMSG( ZONE_ERR,(TEXT("CBIT_StatusTransport error:%d\n"), dwErr));
        SetLastError(dwErr);
        return dwErr;
    }

    if (pUsbDevice->Interrupt.hPipe && !pUsbDevice->Flags.IgnoreInterrupt) {

        memset( ucStatus, 0, dwStatusSize );

        START_DT;

        dwErr = IssueInterruptTransfer(pUsbDevice->UsbFuncs,
                                       pUsbDevice->Interrupt.hPipe,
                                       DefaultTransferComplete,
                                       pUsbDevice->Interrupt.hEvent,
                                       USB_IN_TRANSFER|USB_NO_WAIT|USB_SHORT_TRANSFER_OK,
                                       ucStatus, 0,
                                       dwStatusSize,
                                       &dwBytesTransferred,
                                       pUsbDevice->Timeouts.CommandStatus,
                                       &dwUsbErr );

        STOP_DT( TEXT("StatusTransport"), 100, pUsbDevice->Timeouts.CommandStatus );

        // check Status Transport bits
        if ( ERROR_SUCCESS != dwErr || USB_NO_ERROR != dwUsbErr || dwBytesTransferred != dwStatusSize ) {

            ResetPipe(pUsbDevice->UsbFuncs, pUsbDevice->Interrupt.hPipe, USB_NO_WAIT);

            pUsbDevice->InterruptErrors++;

            DEBUGMSG( ZONE_ERR, (TEXT("CBIT_Status Transport error(%d, 0x%x, %d, %d)\n"), dwErr, dwUsbErr, dwBytesTransferred, pUsbDevice->InterruptErrors ));

            goto CBIT_StatusTransportDone;
        
        } else {
            
            pUsbDevice->InterruptErrors = 0;

        }

        //
        // Check Command Status
        //
        switch (pUsbDevice->pUsbInterface->Descriptor.bInterfaceSubClass)
        {
            case USBMSC_SUBCLASS_UFI:
                //
                // The ASC & (optional) ASCQ are in the data block.
                //
                if ( ucStatus[0] ) {
                    DEBUGMSG(ZONE_ERR, (TEXT("CBIT_StatusTransport::UFI: ASC:0x%x ASCQ:0x%x\n"), ucStatus[0], ucStatus[1] ));
                    dwErr = (DWORD)ERROR_PERSISTANT;
                }
                break;

            default: 
            {
                if (CBIT_COMMAND_COMPLETION_INTERRUPT != ucStatus[0] ) {
            
                    DEBUGMSG(ZONE_ERR, (TEXT("Invalid Command Completion Interrupt: 0x%x\n"), ucStatus[0] ));

                    TEST_TRAP();
            
                    if (ERROR_SUCCESS == dwErr)
                        dwErr = ERROR_GEN_FAILURE;
        
                    goto CBIT_StatusTransportDone;
                }

                switch ( ucStatus[1] & 0x0F ) {

                    case CBIT_STATUS_SUCCESS:
                    break;

                    case CBIT_STATUS_FAIL:
                        DEBUGMSG(ZONE_ERR, (TEXT("CBIT_STATUS_FAIL\n")));
                        dwErr = ERROR_GEN_FAILURE;
                    break;

                    case CBIT_STATUS_PHASE_ERROR:
                        DEBUGMSG(ZONE_ERR, (TEXT("CBIT_STATUS_PHASE_ERROR\n")));
                        // TBD: send CommandBlockReset
                        dwErr = ERROR_GEN_FAILURE;
                    break;

                    case CBIT_STATUS_PERSISTENT_ERROR:
                        DEBUGMSG(ZONE_ERR, (TEXT("CBIT_STATUS_PERSISTENT_ERROR\n")));
                        //
                        // The CBIT spec states that a REQUEST_SENSE command block must be sent,
                        // which must be handled by the disk device.
                        //
                        dwErr = (DWORD)ERROR_PERSISTANT;
                        TEST_TRAP();
                    break;

                    default:
                        TEST_TRAP();
                        DEBUGMSG(ZONE_ERR, (TEXT("Invalid Command Completion Status: 0x%x\n"), ucStatus[1] ));
                        dwErr = ERROR_GEN_FAILURE;
                    break;
                }
            }
        }
    }

CBIT_StatusTransportDone:
    //
    // Some devices claim to use the interrupt endpoint but really do not.
    // Currently, the only way to determine if the device really uses the
    // interrupt endpoint is to try it a few times and see.
    // Exceptions:
    //  UFI Spec 2.1: Interrupt endpoint required
    //
    if (MAX_INT_RETRIES == pUsbDevice->InterruptErrors &&
        !(USBMSC_SUBCLASS_UFI == pUsbDevice->pUsbInterface->Descriptor.bInterfaceSubClass && 
          USBMSC_INTERFACE_PROTOCOL_CBIT == pUsbDevice->pUsbInterface->Descriptor.bInterfaceProtocol)
        )
    {
        DEBUGMSG( ZONE_WARN, (TEXT("USBMSC:IgnoreInterrupt:ON\n"), dwErr) );
        EnterCriticalSection(&pUsbDevice->Lock);
        pUsbDevice->Flags.IgnoreInterrupt = TRUE;        
        LeaveCriticalSection(&pUsbDevice->Lock);
        pUsbDevice->InterruptErrors++;
    }
    
    DEBUGMSG( ZONE_TRACE, (TEXT("USBMSC<CBIT_StatusTransport:%d\n"), dwErr) );
    
    return dwErr;
}


//
// Command/Data/Status Transport 
//
DWORD
CBIT_DataTransfer(
    PUSBMSC_DEVICE           pUsbDevice,
    PTRANSPORT_COMMAND       pCommand,
    OPTIONAL PTRANSPORT_DATA pData,    // OPTIONAL
    BOOL                     Direction // TRUE = Data-In, else Data-Out
    )
{
    DWORD dwBytesTransferred = 0;   
    DWORD dwErr = ERROR_SUCCESS;
    DWORD dwResetErr, dwStatusErr;

    BOOL  bRc = FALSE;

    INIT_DT;

    DEBUGMSG(ZONE_TRACE,(TEXT("USBMSC>CBIT_DataTransfer\n")));

    // parameter checks
    if ( !pCommand || !pCommand->CommandBlock /*|| pCommand->Length > MAX_CBWCB_SIZE*/ ) {
        dwErr = ERROR_INVALID_PARAMETER;
        DEBUGMSG(ZONE_ERR,(TEXT("CBIT_DataTransfer error:%d\n"),dwErr));
        SetLastError(dwErr);
        return dwErr;
    }

    if ( !ACCEPT_IO(pUsbDevice) ) {
        dwErr = ERROR_ACCESS_DENIED;
        DEBUGMSG( ZONE_ERR,(TEXT("CBIT_DataTransfer error:%d\n"), dwErr));
        SetLastError(dwErr);
        return dwErr;
    }

    //
    // We require exclusive entry into the transport.
    // we could implement command queuing.
    //
    EnterCriticalSection(&pUsbDevice->Lock);

    if ( !CBIT_ResetAllPipes(pUsbDevice) ) {
        DEBUGMSG( ZONE_ERR, (TEXT("CBIT_ResetAllPipes failed!\n")));
        dwErr = ERROR_GEN_FAILURE;
        goto CBIT_SendCommandDone;
    }

    //
    // Command Block Transport via ADSC
    //
    dwErr = CBIT_CommandTransport( pUsbDevice,
                                   pCommand,
                                   &dwBytesTransferred );

    if (ERROR_SUCCESS != dwErr || dwBytesTransferred != pCommand->Length) {
        // 2.3.2.1: if the device does STALL an ADSC, then the device shall not
        // transport the status for the command block by interrupt pipe.
        // Treat a timeout in the same manner.
        goto CBIT_SendCommandDone;
    }

    //
    // (optional) Data Transport
    //
    if (pData && pData->DataBlock && pData->RequestLength) {
        UCHAR resets = 0; // consecutive resets
        DEBUGMSG(ZONE_BOT,(TEXT("Data%s Transport - dwDataLength:%d, TimeOut:%d \n"), Direction ? TEXT("In") : TEXT("Out"),  pData->RequestLength, pCommand->Timeout ));
        while (resets < 3) {
            PIPE pipeObj = Direction ? pUsbDevice->BulkIn : pUsbDevice->BulkOut;
            DWORD dwFlags = Direction ? (USB_IN_TRANSFER|USB_NO_WAIT|USB_SHORT_TRANSFER_OK) : (USB_OUT_TRANSFER|USB_NO_WAIT|USB_SHORT_TRANSFER_OK);
            BOOL fRet ;
            DWORD dwTimeout = pCommand->Timeout;
            DWORD dwUsbErr = USB_NOT_COMPLETE_ERROR;
            DWORD dwTransferLength = 0 ;
            ResetEvent(pipeObj.hEvent);
            
            fRet = BulkTransferMgrTransfer(pipeObj.pAsyncContent, DefaultTransferComplete, pipeObj.hEvent, 
                dwFlags, pData->DataBlock, pData->RequestLength );
            if (fRet ) {
                if (!BulkTransferWait(pipeObj.pAsyncContent, dwTimeout)) {
                    fRet = BulkTransferMgrGetStatus(pipeObj.pAsyncContent, &dwTransferLength , &dwUsbErr);
                    dwUsbErr = USB_NOT_COMPLETE_ERROR;
                }
                else {
                    fRet = BulkTransferMgrGetStatus(pipeObj.pAsyncContent, &dwTransferLength , &dwUsbErr);
                }
                BulkTransferClose(pipeObj.pAsyncContent);
            }
            pData->TransferLength  = dwTransferLength;
            
            DEBUGMSG( ZONE_WARN && pData->RequestLength!= pData->TransferLength, 
                (TEXT("CBIT_DataTransfer warning(a, RequestLength:%d TransferLength:%d Err:%d UsbErr:0x%x)\n"), pData->RequestLength, pData->TransferLength, dwErr, dwUsbErr ));
            if (USB_NO_ERROR != dwUsbErr) {
                //
                // reset the Bulk pipe
                //
                UCHAR bIndex = Direction ? pUsbDevice->BulkIn.bIndex : pUsbDevice->BulkOut.bIndex;
                
                DEBUGMSG( ZONE_ERR, (TEXT("CBIT_DataTransfer error(b, dwErr:%d, dwUsbErr:0x%x, dwTimeout:%d)\n"), dwErr, dwUsbErr, dwTimeout));

                resets++;

                dwResetErr = ResetBulkEndpoint( pUsbDevice->UsbFuncs,
                                                pUsbDevice->hUsbDevice,
                                                Direction ? pUsbDevice->BulkIn.hPipe : pUsbDevice->BulkOut.hPipe,
                                                DefaultTransferComplete,
                                                Direction ? pUsbDevice->BulkIn.hEvent : pUsbDevice->BulkOut.hEvent,
                                                bIndex,
                                                pUsbDevice->Timeouts.Reset );

                if (ERROR_SUCCESS != dwResetErr) {
                    DEBUGMSG( ZONE_ERR, (TEXT("ResetBulkEndpoint.1 ERROR:%d\n"), dwResetErr));
                    dwErr = ERROR_GEN_FAILURE;
                    break;
                }
                if (dwUsbErr == USB_NOT_COMPLETE_ERROR) { // Not be able complete. It is not stall.
                    dwErr = ERROR_GEN_FAILURE;
                    break;
                }
            }
            else { // Success.
                break;
            }
        }
    }


CBIT_SendCommandDone:
    //
    // cleanup        
    //
    if (ERROR_SUCCESS != dwErr) {
        bRc = CBIT_ResetRecovery(pUsbDevice);
    }
    else {
        //
        // Status Transport
        //
        DEBUGMSG(ZONE_BOT,(TEXT("Status Transport\n")));
        
        dwStatusErr = CBIT_StatusTransport( pUsbDevice );
        
        if (ERROR_SUCCESS != dwStatusErr ) {
            
            if (ERROR_SUCCESS == dwErr)
                dwErr = dwStatusErr;
        }

    }

    LeaveCriticalSection(&pUsbDevice->Lock);
   
    DEBUGMSG(ZONE_TRACE,(TEXT("USBMSC<CBIT_DataTransfer:%d\n"), dwErr));    

    return dwErr;
}

// EOF
