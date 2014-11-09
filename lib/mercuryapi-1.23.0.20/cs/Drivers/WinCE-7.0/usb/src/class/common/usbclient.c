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

    usbclient.c

Abstract:

    Common USB Client Driver Interface

Issues:
    
    No isoch support yet

--*/

#include "usbclient.h"

#if 0
#ifdef DEBUG
DBGPARAM dpCurSettings = {
    TEXT("USBCLIENT"), {
    TEXT("Undefined"), TEXT("Undefined"), TEXT("Undefined"),   TEXT("Undefined")  
    TEXT("Undefined"), TEXT("Undefined"), TEXT("Undefined"),   TEXT("Undefined")  
    TEXT("Undefined"), TEXT("Undefined"), TEXT("Undefined"),   TEXT("Undefined")  
    TEXT("Undefined"), TEXT("Undefined"), TEXT("Undefined"),   TEXT("Undefined")  
    },
     0x0003 // ZONE_WRN|ZONE_ERR
};
#endif  // DEBUG

BOOL 
DllEntry(
   HANDLE hDllHandle, 
   DWORD  dwReason,
   LPVOID lpreserved
   ) 
{
    UNREFERENCED_PARAMETER(lpreserved);
    switch (dwReason) {

      case DLL_PROCESS_ATTACH:
           DEBUGREGISTER((HINSTANCE)hDllHandle);
           break;

      case DLL_PROCESS_DETACH:
           break;

      default:
        break;
    }
    return TRUE;
}
#endif 0

__inline
DWORD
_ResetEvent(
    HANDLE hEvent
    )
{
   DWORD dwErr = ERROR_SUCCESS;

    if ( !ResetEvent(hEvent) ) {
        dwErr = GetLastError();
        DEBUGMSG(ZONE_USBCLIENT, (TEXT("*** ResetEvent ERROR:%d ***\n"), dwErr));
        // ASSERT(0);
        return dwErr;
    }

    return TRUE;
}


BOOL
AbortTransfer(
    LPCUSB_FUNCS   pUsbFuncs,
    USB_TRANSFER   hTransfer,
    DWORD          dwFlags
    )
{
    BOOL bRc = TRUE;

    // AbortTransfer checks if the transfer has already completed
    if ( !pUsbFuncs->lpAbortTransfer(hTransfer, dwFlags) ) {
        DEBUGMSG( ZONE_USBCLIENT, (TEXT("*** AbortTransfer ERROR:%d ***\n"), GetLastError())); 
        bRc = FALSE;
    }

    return bRc;
}


BOOL
CloseTransferHandle(
    LPCUSB_FUNCS   pUsbFuncs,
    USB_TRANSFER   hTransfer
    )
{
    BOOL bRc = TRUE;

    // This assert may fail on suprise remove,
    // but should pass during normal I/O.
    // ASSERT( pUsbFuncs->lpIsTransferComplete(hTransfer) ); 

    // CloseTransfer aborts any pending transfers
    if ( !pUsbFuncs->lpCloseTransfer(hTransfer) ) {
        DEBUGMSG( ZONE_USBCLIENT, (TEXT("*** CloseTransfer ERROR:%d ***\n"), GetLastError())); 
        bRc = FALSE;
    }

    return bRc;
}


//
// Returns: 
//  Win32 error
//    
// Note: 
//    a synchronous ClearOrSetFeature call can take quite some time.
//
DWORD
ClearOrSetFeature(
    LPCUSB_FUNCS              pUsbFuncs,
    HANDLE                    hUsbDevice,
    LPTRANSFER_NOTIFY_ROUTINE NotifyRoutine,     // CallbackRoutine: signals a completion event.
    PVOID                     NotifyContext,     // CallbackContext: handle to a BulkXxx Completion Event
    DWORD                     dwFlags,           // see ClearOrSetFeature doc
    WORD                      wFeature,          // one of USB_FEATURE_*
    UCHAR                     bIndex,
    DWORD                     dwTimeout,         // Timeout in msec
    BOOL                      bSet               // TRUE to Set, FALSE to Clear
    )
{
    DWORD dwErr    = ERROR_SUCCESS;
    DWORD dwUsbErr = USB_NO_ERROR;
    DWORD dwWaitReturn;
    
    USB_TRANSFER hTransfer;

    if (NotifyContext && NotifyRoutine && dwTimeout) {

        _ResetEvent(NotifyContext); // NotifyContext *must* be an EVENT
    
    }
  
    //
    // reset endpoint on device
    //
    if (bSet)
        hTransfer = pUsbFuncs->lpSetFeature( hUsbDevice,
                                             NotifyRoutine,                  
                                             NotifyContext,
                                             dwFlags,
                                             wFeature,
                                             bIndex );
    else 
        hTransfer = pUsbFuncs->lpClearFeature( hUsbDevice,
                                               NotifyRoutine,                  
                                               NotifyContext,
                                               dwFlags,
                                               wFeature,
                                               bIndex );

    if ( hTransfer ) {
            //
            // Asynch call completed.
            // Get transfer status & number of bytes transferred
            //
            if (NotifyContext && NotifyRoutine) {

                if (!dwTimeout) {
                    return (DWORD)hTransfer;
                }

                //
                // sync the transfer completion / timer
                //
                dwWaitReturn = WaitForSingleObject( NotifyContext,
                                                    dwTimeout );

                switch (dwWaitReturn) {

                   case WAIT_OBJECT_0:
                      //
                      // The completion event was signalled by the callback.
                      // determine if it was actually cleared on the device
                      //
                      // ASSERT( pUsbFuncs->lpIsTransferComplete(hTransfer) );

                      GetTransferStatus(pUsbFuncs, hTransfer, NULL, &dwUsbErr);
                        
                      if ( USB_NO_ERROR != dwUsbErr)
                          dwErr = ERROR_GEN_FAILURE;
                      break;

                    case WAIT_TIMEOUT:
                      //
                      // The transfer reqest timed out.
                      //
                      DEBUGMSG( ZONE_USBCLIENT, (TEXT("ClearOrSetFeature:WAIT_TIMEOUT on bIndex:0x%x\n"), bIndex ));

                      GetTransferStatus(pUsbFuncs, hTransfer, NULL, &dwUsbErr);

                      //
                      // let caller know it timed out
                      //
                      dwErr = ERROR_TIMEOUT;
                      break;

                    default:
                      dwErr = ERROR_GEN_FAILURE;
                      DEBUGMSG( ZONE_USBCLIENT, (TEXT("*** Unhandled WaitReason:%d ***\n"), dwWaitReturn ));
                      break;
                }

            } else {
                //
                // Synch call completed.
                // determine if it was actually cleared on the device
                //
                // ASSERT( pUsbFuncs->lpIsTransferComplete(hTransfer) );

                GetTransferStatus(pUsbFuncs, hTransfer, NULL, &dwUsbErr);

                if ( USB_NO_ERROR != dwUsbErr)
                    dwErr = ERROR_GEN_FAILURE;
            }
        
            CloseTransferHandle(pUsbFuncs, hTransfer);

    } else {
        dwErr = GetLastError();
        DEBUGMSG( ZONE_USBCLIENT, (TEXT("*** ClearOrSetFeature on endpoint:0x%x ERROR:%d ***\n"), bIndex, dwErr ));
    }

    return dwErr;
}


//
// Generic DefaultTransferComplete callback routine.
// Simply signals the hEvent passed in when USB signals a transfer is done.
// If you prematurely close/abort a transfer then this routine will still run.
//
DWORD
DefaultTransferComplete(
   PVOID    Context
   )
{
   HANDLE hEvent = (HANDLE)Context;
   DWORD  dwErr  = ERROR_SUCCESS;

   if ( hEvent ) {
      //
      // The current operation completed, signal the event
      //
       if ( !SetEvent( hEvent) ) {
          dwErr = GetLastError();
          //SetLastError(dwErr);
          DEBUGMSG(ZONE_USBCLIENT,(TEXT("*** SetEvent ERROR:%d ***\n"), dwErr));
          // ASSERT(0);
       }

   } else {
      dwErr = ERROR_INVALID_HANDLE;
      SetLastError(dwErr);
      DEBUGMSG( ZONE_USBCLIENT,(TEXT("*** DefaultTransferComplete ERROR:%d ***\n"), dwErr));
      // ASSERT(0);
   }

   return dwErr;
}


//
// returns Win32 error
//
// Note: 
//    a synchronous GetStatus call can take quite some time.
//
DWORD
GetStatus(
    LPCUSB_FUNCS              pUsbFuncs,
    HANDLE                    hUsbDevice,
    LPTRANSFER_NOTIFY_ROUTINE NotifyRoutine,     // CallbackRoutine: signals a completion event.
    PVOID                     NotifyContext,     // CallbackContext: handle to a BulkXxx Completion Event
    DWORD                     dwFlags,
    UCHAR                     bIndex,
    LPWORD                    lpwStatus,
    DWORD                     dwTimeout         // Timeout in msec
    )
{
    DWORD dwErr    = ERROR_SUCCESS;
    DWORD dwUsbErr = USB_NO_ERROR;
    DWORD dwWaitReturn;
    DWORD dwBytesTransferred;
    USB_TRANSFER hTransfer;


    if (NotifyContext && NotifyRoutine && dwTimeout) {

        _ResetEvent(NotifyContext); // NotifyContext *must* be an EVENT

    }

    hTransfer = pUsbFuncs->lpGetStatus( hUsbDevice,
                                        NotifyRoutine,
                                        NotifyContext,
                                        dwFlags,
                                        bIndex,
                                        lpwStatus );

    if ( hTransfer ) {
            //
            // Asynch call completed.
            // Get transfer status & number of bytes transferred
            //
            if (NotifyContext && NotifyRoutine) {

                if (!dwTimeout) {
                    return (DWORD)hTransfer;
                }

                //
                // sync the transfer completion / timer
                //
                dwWaitReturn = WaitForSingleObject( NotifyContext,
                                                    dwTimeout );

                switch (dwWaitReturn) {

                   case WAIT_OBJECT_0:
                      //
                      // The completion event was signalled by the callback.
                      // Get transfer status & number of bytes transferred
                      //
                      // ASSERT( pUsbFuncs->lpIsTransferComplete(hTransfer) );

                      GetTransferStatus(pUsbFuncs, hTransfer, &dwBytesTransferred, &dwUsbErr);
                      break;

                    case WAIT_TIMEOUT:
                      //
                      // The transfer reqest timed out.
                      // Get transfer status & number of bytes transferred
                      //
                      DEBUGMSG( ZONE_USBCLIENT, (TEXT("GetStatus:WAIT_TIMEOUT on bIndex:0x%x\n"), bIndex ));

                      GetTransferStatus(pUsbFuncs, hTransfer, &dwBytesTransferred, &dwUsbErr);

                      //
                      // let caller know it timed out
                      //
                      dwErr = ERROR_TIMEOUT;
                      break;

                    default:
                      dwErr = ERROR_GEN_FAILURE;
                      DEBUGMSG( ZONE_USBCLIENT, (TEXT("*** Unhandled WaitReason:%d ***\n"), dwWaitReturn));
                      break;
                }

            } else {
                //
                // Synch call completed.
                // Get transfer status & number of bytes transferred
                //
                // ASSERT( pUsbFuncs->lpIsTransferComplete(hTransfer) );

                GetTransferStatus(pUsbFuncs, hTransfer, &dwBytesTransferred, &dwUsbErr);
                // ASSERT( USB_NO_ERROR == dwUsbErr);
            }

            CloseTransferHandle(pUsbFuncs, hTransfer);

    } else {
        dwErr = GetLastError();
        DEBUGMSG( ZONE_USBCLIENT, (TEXT("*** GetStatus on endpoint:0x%x failed with ERROR:%d ***\n"), bIndex, dwErr ));
    }

    if ( USB_NO_ERROR != dwUsbErr && ERROR_SUCCESS != dwErr ) {
        dwErr = ERROR_GEN_FAILURE;
    }

    if ( ERROR_SUCCESS != dwErr ) {
        SetLastError(dwErr);
    }

    return dwErr;
}


// returns TRUE if successful
BOOL
GetTransferStatus(
    LPCUSB_FUNCS   pUsbFuncs,
    USB_TRANSFER   hTransfer,
    LPDWORD        pBytesTransferred, // OPTIONAL returns number of bytes transferred
    PUSB_ERROR     pUsbError          // returns USB error code
    )
{
    BOOL bRc = TRUE;

    if ( pUsbFuncs->lpGetTransferStatus(hTransfer, pBytesTransferred, pUsbError) ) {
        if ( USB_NO_ERROR != *pUsbError ) {
            DEBUGMSG( ZONE_USBCLIENT, (TEXT("GetTransferStatus (BytesTransferred:%d, UsbError:0x%x)\n"), pBytesTransferred?*pBytesTransferred:-1, pUsbError?*pUsbError:-1 )); 
        }
    } else {
        DEBUGMSG( ZONE_USBCLIENT, (TEXT("*** GetTransferStatus ERROR:%d ***\n"), GetLastError())); 
        *pUsbError = USB_CANCELED_ERROR;
        bRc = FALSE;
    }

    return bRc;
}


/* ++

IssueBulkTransfer: generic USB bulk transfer handler.

If NotifyRoutine and NotifyContext are NULL, then the call is made synchronously.

If NotifyRoutine and NotifyContext are not NULL, then the call is made asynchronously
with the following restrictions:

    If dwTimeout not zero, then IssueBulkTransfer waits for either the NotifyContext,
    which is must be an initialized EVENT, or the timeout duration.

    If dwTimeout is zero, then IssueBulkTransfer returns immediately. 
    The Transfer handle is returned in the pUsbError parameter. 
    It is up to the caller to check transfer status, close the transfer handle, etc.

Notes:
    It's up to the caller to determine if the correct number of bytes were transferred.
    It's up to the caller to determine any Win32 or USB error codes.
    It's up to the caller to handle any USB errors.

Return:
    Number of bytes transferred by USB, Win32 error code, and either USB_ERROR or USB_TRANSFER.

-- */
DWORD
IssueBulkTransfer( 
   LPCUSB_FUNCS              pUsbFuncs,
   USB_PIPE                  hPipe,
   LPTRANSFER_NOTIFY_ROUTINE NotifyRoutine,     // Transfer completion routine.
   PVOID                     NotifyContext,     // Single argument passed to the completion routine
   DWORD                     Flags,             // USB_XXX flags describing the transfer
   LPVOID                    pBuffer,           // Pointer to transfer buffer
   ULONG                     PhysAddr,          // Specifies the physical address, which may be NULL, of the data buffer
   DWORD                     BufferLength,      // Length of transfer buffer in bytes
   LPDWORD                   pBytesTransferred, // Returns number of bytes transferred by USB
   DWORD                     dwTimeout,         // Timeout in msec
   PUSB_ERROR                pUsbRc             // Returns USB_ERROR or USB_TRANSFER
   )
{
    USB_TRANSFER hTransfer = NULL;
    DWORD dwWaitReturn = 0;
    DWORD dwErr = ERROR_SUCCESS;

    if ( pUsbFuncs && hPipe && pBytesTransferred && pUsbRc ) {

        *pBytesTransferred = 0;
        *pUsbRc = USB_NO_ERROR;

        if (NotifyContext && NotifyRoutine && dwTimeout) {

            _ResetEvent(NotifyContext); // NotifyContext *must* be an EVENT
        
        }

        hTransfer = pUsbFuncs->lpIssueBulkTransfer( hPipe,
                                                   NotifyRoutine,
                                                   NotifyContext,
                                                   Flags,
                                                   BufferLength,
                                                   pBuffer,
                                                   PhysAddr );

        if ( hTransfer ) {
            //
            // Asynch call succeeded.
            // Get transfer status & number of bytes transferred
            //
            if (NotifyContext && NotifyRoutine) {

                if (!dwTimeout) {
                    *pUsbRc = (USB_ERROR)hTransfer;
                    return dwErr;
                }

                //
                // sync the transfer completion / timer
                //
                dwWaitReturn = WaitForSingleObject( NotifyContext,
                                                    dwTimeout );

                switch (dwWaitReturn) {

                   case WAIT_OBJECT_0:
                      //
                      // The completion event was signalled by the callback.
                      // Get transfer status & number of bytes transferred
                      //
                      // ASSERT( pUsbFuncs->lpIsTransferComplete(hTransfer) );

                      GetTransferStatus(pUsbFuncs, hTransfer, pBytesTransferred, pUsbRc);

                      break;

                    case WAIT_TIMEOUT:
                      //
                      // The transfer reqest timed out.
                      // Get transfer status & number of bytes transferred
                      //
                      DEBUGMSG( ZONE_USBCLIENT, (TEXT("%s:WAIT_TIMEOUT on hT:0x%x\n"), (Flags & USB_IN_TRANSFER) ? TEXT("IN") : TEXT("OUT"), hTransfer ));

                      GetTransferStatus(pUsbFuncs, hTransfer, pBytesTransferred, pUsbRc);

                      //
                      // let caller know it timed out
                      //
                      dwErr = ERROR_TIMEOUT;
                      break;

                    default:
                      dwErr = ERROR_GEN_FAILURE;
                      DEBUGMSG( ZONE_USBCLIENT, (TEXT("*** Unhandled WaitReason:%d ***\n"), dwWaitReturn, hTransfer ));
                      break;
                }
            
            } else {
                //
                // Synch call completed.
                // Get transfer status & number of bytes transferred
                //
                // ASSERT( pUsbFuncs->lpIsTransferComplete(hTransfer) );

                GetTransferStatus(pUsbFuncs, hTransfer, pBytesTransferred, pUsbRc);
            }

            CloseTransferHandle(pUsbFuncs, hTransfer);

        } else {
            dwErr = GetLastError();
            DEBUGMSG( ZONE_USBCLIENT, (TEXT("*** IssueBulkTransfer ERROR(3, %d) ***\n"), dwErr ));
        }
   
    } else {
        dwErr = ERROR_INVALID_PARAMETER;
    }

    if ( pUsbRc && USB_NO_ERROR != *pUsbRc && ERROR_SUCCESS == dwErr) {
        dwErr = ERROR_GEN_FAILURE;
    }

    if ( ERROR_SUCCESS != dwErr ) {
        SetLastError(dwErr);
        DEBUGMSG( ZONE_USBCLIENT, (TEXT("IssueBulkTransfer ERROR(5, BytesTransferred:%d, Win32Err:%d, UsbError:0x%x)\n"), pBytesTransferred?*pBytesTransferred:-1, dwErr, pUsbRc?*pUsbRc:-1 ));
    }

    return dwErr;
}


/* ++

  IssueInterruptTransfer: generic USB interrupt transfer handler.

If NotifyRoutine and NotifyContext are NULL, then the call is made synchronously.

If NotifyRoutine and NotifyContext are not NULL, then the call is made asynchronously
with the following restrictions:

    If dwTimeout not zero, then IssueBulkTransfer waits for either the NotifyContext,
    which is must be an initialized EVENT, or the timeout duration.

    If dwTimeout is zero, then IssueBulkTransfer returns immediately. 
    The Transfer handle is returned in the pUsbError parameter. 
    It is up to the caller to check transfer status, close the transfer handle, etc.

Notes:
    It's up to the caller to determine if the correct number of bytes were transferred.
    It's up to the caller to determine any Win32 or USB error codes.
    It's up to the caller to handle any USB errors.

Return:
    Number of bytes transferred by USB, Win32 error code, and either USB_ERROR or USB_TRANSFER.

-- */
DWORD
IssueInterruptTransfer( 
   LPCUSB_FUNCS              pUsbFuncs,
   USB_PIPE                  hPipe,
   LPTRANSFER_NOTIFY_ROUTINE NotifyRoutine,     // Transfer completion routine.
   PVOID                     NotifyContext,     // Single argument passed to the completion routine
   DWORD                     Flags,             // USB_XXX flags describing the transfer
   LPVOID                    pBuffer,           // Ppointer to transfer buffer
   ULONG                     PhysAddr,          // Specifies the physical address, which may be NULL, of the data buffer
   DWORD                     BufferLength,      // Length of transfer buffer in bytes
   LPDWORD                   pBytesTransferred, // Number of bytes transferred by USB
   DWORD                     dwTimeout,         // Timeout in msec
   PUSB_ERROR                pUsbRc             // Returns USB_ERROR or USB_TRANSFER
   )
{
    USB_TRANSFER hTransfer = NULL;
    DWORD dwWaitReturn = 0;
    DWORD dwErr = ERROR_SUCCESS;

    if ( pUsbFuncs && hPipe && pBytesTransferred && pUsbRc ) {

        *pBytesTransferred = 0;
        *pUsbRc = USB_NO_ERROR;

        if (NotifyContext && NotifyRoutine && dwTimeout) {

            _ResetEvent(NotifyContext); // NotifyContext *must* be an EVENT
        
        }

        hTransfer = pUsbFuncs->lpIssueInterruptTransfer( hPipe,
                                                         NotifyRoutine,
                                                         NotifyContext,
                                                         Flags,
                                                         BufferLength,
                                                         pBuffer,
                                                         PhysAddr );
        if ( hTransfer ) {
            //
            // Asynch call succeeded.
            // Get transfer status & number of bytes transferred
            //
            if (NotifyContext && NotifyRoutine) {

                if (!dwTimeout) {
                    *pUsbRc = (USB_ERROR)hTransfer;
                    return dwErr;
                }

                //
                // sync the transfer completion / timer
                //
                dwWaitReturn = WaitForSingleObject( NotifyContext,
                                                    dwTimeout );

                switch (dwWaitReturn) {

                   case WAIT_OBJECT_0:
                      //
                      // The completion event was signalled by the callback.
                      // Get transfer status & number of bytes transferred
                      //
                      // ASSERT( pUsbFuncs->lpIsTransferComplete(hTransfer) );

                      GetTransferStatus(pUsbFuncs, hTransfer, pBytesTransferred, pUsbRc);

                      break;

                    case WAIT_TIMEOUT:
                      //
                      // The transfer reqest timed out.
                      // Get transfer status & number of bytes transferred
                      //
                      DEBUGMSG( ZONE_USBCLIENT, (TEXT("%s:WAIT_TIMEOUT on hT:0x%x\n"), (Flags & USB_IN_TRANSFER) ? TEXT("IN") : TEXT("OUT"), hTransfer ));

                      GetTransferStatus(pUsbFuncs, hTransfer, pBytesTransferred, pUsbRc);

                      //
                      // let caller know it timed out
                      //
                      dwErr = ERROR_TIMEOUT;
                      break;

                    default:
                      dwErr = ERROR_GEN_FAILURE;
                      DEBUGMSG( ZONE_USBCLIENT, (TEXT("*** Unhandled WaitReason:%d ***\n"), dwWaitReturn, hTransfer ));
                      break;
                }

            } else {
                //
                // Synch call completed.
                // Get transfer status & number of bytes transferred
                //
                // ASSERT( pUsbFuncs->lpIsTransferComplete(hTransfer) );

                GetTransferStatus(pUsbFuncs, hTransfer, pBytesTransferred, pUsbRc);
            }

            CloseTransferHandle(pUsbFuncs, hTransfer);

        } else {
            dwErr = ERROR_GEN_FAILURE;
            DEBUGMSG( ZONE_USBCLIENT, (TEXT("*** IssueInterruptTransfer ERROR(3, %d) ***\n"), dwErr ));
        }
   
    } else {
        dwErr = ERROR_INVALID_PARAMETER;
    }

    if ( pUsbRc && USB_NO_ERROR != *pUsbRc && ERROR_SUCCESS == dwErr) {
        dwErr = ERROR_GEN_FAILURE;
    }

    if ( ERROR_SUCCESS != dwErr ) {
        SetLastError(dwErr);
        DEBUGMSG( ZONE_USBCLIENT, (TEXT("IssueInterruptTransfer ERROR(5, BytesTransferred:%d, Win32Err:%d, UsbError:0x%x)\n"), pBytesTransferred?*pBytesTransferred:-1, dwErr, pUsbRc?*pUsbRc:-1 ));
    }

    return dwErr;
}


/* ++

IssueVendorTransfer: generic USB class specific transfer hanlder.

If NotifyRoutine and NotifyContext are NULL, then the call is made synchronously.

If NotifyRoutine and NotifyContext are not NULL, then the call is made asynchronously
with the following restrictions:

    If dwTimeout not zero, then IssueBulkTransfer waits for either the NotifyContext,
    which is must be an initialized EVENT, or the timeout duration.

    If dwTimeout is zero, then IssueBulkTransfer returns immediately. 
    The Transfer handle is returned in the pUsbError parameter. 
    It is up to the caller to check transfer status, close the transfer handle, etc.

Notes:
    It's up to the caller to determine if the correct number of bytes were transferred.
    It's up to the caller to determine any Win32 or USB error codes.
    It's up to the caller to handle any USB errors.

Return:
    Number of bytes transferred by USB, Win32 error code, and either USB_ERROR or USB_TRANSFER.

-- */
DWORD   
IssueVendorTransfer(
   LPCUSB_FUNCS              pUsbFuncs,
   HANDLE                    hUsbDevice,
   LPTRANSFER_NOTIFY_ROUTINE NotifyRoutine,       // Transfer completion routine.
   PVOID                     NotifyContext,       // Single argument passed to the completion routine
   DWORD                     Flags,               // USB_XXX flags describing the transfer
   PUSB_DEVICE_REQUEST       pControlHeader,      // Request header
   LPVOID                    pBuffer,             // Pointer to transfer buffer
   ULONG                     PhysAddr,            // Specifies the physical address, which may be NULL, of the data buffer
   LPDWORD                   pBytesTransferred,   // Number of bytes transferred by USB
   DWORD                     dwTimeout,           // Timeout in msec
   PUSB_ERROR                pUsbRc               // Returns USB_ERROR or USB_TRANSFER
   )
{
    USB_TRANSFER hTransfer;
    DWORD  dwErr = ERROR_SUCCESS;
    DWORD  dwWaitReturn;

    if ( pUsbFuncs && hUsbDevice && pControlHeader && pBytesTransferred && pUsbRc ) {

        *pUsbRc = USB_NO_ERROR;
        *pBytesTransferred = 0;

        if (NotifyContext && NotifyRoutine && dwTimeout) {

            _ResetEvent(NotifyContext); // NotifyContext *must* be an EVENT
        
        }

        hTransfer = pUsbFuncs->lpIssueVendorTransfer( hUsbDevice,
                                                     NotifyRoutine,
                                                     NotifyContext,
                                                     Flags,
                                                     pControlHeader,
                                                     pBuffer,
                                                     PhysAddr );
        if ( hTransfer ) {
            //
            // Asynch call completed.
            // Get transfer status & number of bytes transferred
            //
            if (NotifyContext && NotifyRoutine) {

                if (!dwTimeout) {
                    *pUsbRc = (USB_ERROR)hTransfer;
                    return dwErr;
                }

                //
                // sync the transfer completion / timer
                //
                dwWaitReturn = WaitForSingleObject( NotifyContext,
                                                    dwTimeout );

                switch (dwWaitReturn) {

                   case WAIT_OBJECT_0:
                      //
                      // The completion event was signalled by the callback.
                      // Get transfer status & number of bytes transferred
                      //
                      // ASSERT( pUsbFuncs->lpIsTransferComplete(hTransfer) );

                      GetTransferStatus(pUsbFuncs, hTransfer, pBytesTransferred, pUsbRc);

                      break;

                   case WAIT_TIMEOUT:
                      //
                      // The transfer reqest timed out.
                      // Get transfer status & number of bytes transferred
                      //
                      DEBUGMSG( ZONE_USBCLIENT, (TEXT("%s:WAIT_TIMEOUT on hT:0x%x\n"), (Flags & USB_IN_TRANSFER) ? TEXT("IN") : TEXT("OUT"), hTransfer ));

                      GetTransferStatus(pUsbFuncs, hTransfer, pBytesTransferred, pUsbRc);

                      //
                      // let caller know it timed out
                      //
                      dwErr = ERROR_TIMEOUT;
                      break;
       
                   default:
                      dwErr = ERROR_GEN_FAILURE;
                      DEBUGMSG( ZONE_USBCLIENT, (TEXT("*** Unhandled WaitReason:%d ***\n"), dwWaitReturn ));
                      // ASSERT(0);
                      break;
                }

            } else {
                //
                // Synch call completed.
                // Get transfer status & number of bytes transferred
                //
                // ASSERT( pUsbFuncs->lpIsTransferComplete(hTransfer) );

                GetTransferStatus(pUsbFuncs, hTransfer, pBytesTransferred, pUsbRc);
            }

            CloseTransferHandle(pUsbFuncs, hTransfer);

        } else {
            dwErr = ERROR_GEN_FAILURE;
            DEBUGMSG( ZONE_USBCLIENT, (TEXT("*** IssueVendorTransfer ERROR(3, 0x%x) ***\n"), dwErr ));
        }
    
    } else {
        dwErr = ERROR_INVALID_PARAMETER;
    }

    if ( pUsbRc && ERROR_SUCCESS == dwErr && USB_NO_ERROR != *pUsbRc) {
        dwErr = ERROR_GEN_FAILURE;
    }

    if ( ERROR_SUCCESS != dwErr ) {
        SetLastError(dwErr);
        DEBUGMSG( ZONE_USBCLIENT, (TEXT("IssueVendorTransfer ERROR(5, BytesTransferred:%d, Win32Err:%d, UsbError:0x%x)\n"), pBytesTransferred?*pBytesTransferred:-1, dwErr, pUsbRc?*pUsbRc:-1)); 
    }

    return dwErr;
}


/*++

ResetDefaultEndpoint:

  Tries to reset the default endpoint 0.

Return:
  
    Win32 error code.

Notes:

  In really bad cases you could see something like the following trace:

  CQueuedPipe(Control)::CheckForDoneTransfers - failure on TD 0x1358c0, address = 1, endpoint = 0, errorCounter = 0, status field = 0x22
  CFunction::SetOrClearFeature - recipient = 2, wIndex = 0, set/clear = 0x1, feature = 0x0, FAILED
  CHub(Root tier 0)::HubStatusChangeThread - device on port 1 is connected but has been disabled. Trying to detach & re-attach

The status field is a bitmask (bits 17-22 of the status field defined in the UHCI spec):
    0x01    bitstuff error
    0x02    CRC or timeout error (cannot distinguish the two)
    0x04    NAK (you should not see this bit set in normal operation)
    0x08    babble detected
    0x10    data buffer overrun
    0x20    STALL response from device

--*/
DWORD
ResetDefaultEndpoint(
    LPCUSB_FUNCS   pUsbFuncs,
    HANDLE         hUsbDevice
    )
{
    DWORD dwErr = ERROR_SUCCESS;
    BOOL  bRc, bHalted;

    if ( !pUsbFuncs || !hUsbDevice ) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // test & reset the default pipe
    //
    bRc = pUsbFuncs->lpIsDefaultPipeHalted(hUsbDevice, &bHalted);

    if ( bRc && bHalted ) {
        //
        // ResetDefaultPipe does a clear feature on endpoint 0.
        //
        if ( !pUsbFuncs->lpResetDefaultPipe(hUsbDevice) ) {
            //
            // We can not reset the port on WCE. However, a device must automatically un-stall 
            // EP0 as soon as another SETUP packet is received on it. Any stall on EP0 is 
            // considered a "protocol STALL" rather than a "functional STALL" (see the USB 
            // spec for details). Calling the USBD function to clear the default pipe's stall 
            // condition really only tweaks a bit in an internal structure; you should be ready 
            // to go again immediately.
            //
            DEBUGMSG( ZONE_USBCLIENT, (TEXT("*** ResetDefaultPipe failure ***\n")));
            // return ERROR in case caller needs protocol recovery.
            dwErr = ERROR_GEN_FAILURE;
        }

    } else if (!bRc) {
        dwErr= GetLastError(); //ERROR_GEN_FAILURE;
        DEBUGMSG( ZONE_USBCLIENT, (TEXT("*** IsDefaultPipeHalted ERROR:%d ***\n"), dwErr));
    }

    return dwErr;
}


//
// ResetBulkEndpoint: resets the pipe, then checks the endpoint status on the device.
// If the endpoint is halted then send it a clear feature.
//
//  If dwTimeout not zero, then ResetBulkEndpoint waits for either the NotifyContext,
//  which is must be an initialized EVENT, or the timeout duration.
//
// Returns Win32 error
//
DWORD
ResetBulkEndpoint(
    LPCUSB_FUNCS              pUsbFuncs,
    HANDLE                    hUsbDevice,
    USB_PIPE                  hPipe,
    LPTRANSFER_NOTIFY_ROUTINE NotifyRoutine,    // Transfer completion routine.
    PVOID                     NotifyContext,    // Single argument passed to the completion routine
    UCHAR                     bIndex,
    DWORD                     dwTimeout         // Timeout in msec
    )
{
    DWORD dwErr    = ERROR_SUCCESS;
    DWORD dwFlags = (NotifyRoutine && NotifyContext) ? USB_NO_WAIT : 0;
    BOOL  bHalted=TRUE;
    
    UNREFERENCED_PARAMETER(dwTimeout);
    UNREFERENCED_PARAMETER(bIndex);
    UNREFERENCED_PARAMETER(hUsbDevice);

    if ( pUsbFuncs->lpIsPipeHalted(hPipe, &bHalted) && !bHalted) {
        if (NotifyRoutine)
            (*NotifyRoutine)(NotifyContext);
        return  dwErr ;
    }
    //    
    // reset this pipe
    //
    ResetPipe(pUsbFuncs, hPipe, dwFlags);
/*
    if (NotifyContext && NotifyRoutine && dwTimeout) {

        _ResetEvent(NotifyContext); // NotifyContext *must* be an EVENT

#if GET_ENDPOINT_STATUS
        // GetStatus can take quite some time, and will sometimes hang EP0 on a bad device. 
        // Since we can't recover via port reset then do it blindy.
        dwErr = GetStatus( pUsbFuncs, 
                           hUsbDevice,
                           NotifyRoutine,
                           NotifyContext,
                           USB_SEND_TO_ENDPOINT,
                           bIndex,
                           &wHalt,
                           dwTimeout );
#else
        // just do it
        wHalt = 0x01;
#endif

        // See USB 1.1 Spec, 9.4.5: D0 set. 
        // USBD may return garbage if the device was unpluged.
        if ( ERROR_SUCCESS == dwErr && 0x01 == wHalt ) {
            DEBUGMSG( ZONE_USBCLIENT, (TEXT("Enpoint:0x%x wStatus:0x%x\n"), bIndex, wHalt ));
                            
            dwErr = ClearOrSetFeature(pUsbFuncs, 
                                      hUsbDevice,
                                      NotifyRoutine,
                                      NotifyContext,
                                      USB_SEND_TO_ENDPOINT | dwFlags,
                                      USB_FEATURE_ENDPOINT_STALL,
                                      bIndex,
                                      dwTimeout,
                                      FALSE ); // clear
        }
    }
*/
    if ( ERROR_SUCCESS != dwErr ) {
        SetLastError(dwErr);
    }

    return dwErr;
}


//
// returns Win32 error
//
BOOL
ResetPipe(
    LPCUSB_FUNCS    pUsbFuncs,
    USB_PIPE        hPipe,
    DWORD           dwFlags
    )
{
    BOOL  bHalted;
    BOOL bRc = TRUE;

#if 1
    UNREFERENCED_PARAMETER(dwFlags);
#else
    //
    // Abort any pending transfers on this pipe.
    // This does not guarantee that no more transfers will be completed; 
    // any outstanding transfers will complete even after this function is called. 
    // Any registered completion callbacks will be invoked.
    //
    if (!pUsbFuncs->lpAbortPipeTransfers(hPipe, dwFlags )) {
        DEBUGMSG( ZONE_USBCLIENT, (TEXT("*** AbortPipeTransfers ERROR:d ***\n"), GetLastError()));
        bRc = FALSE;
    }
#endif
    //
    // test & reset pipe state within the USB stack
    //
    if ( pUsbFuncs->lpIsPipeHalted(hPipe, &bHalted) && bHalted) {
        // reset the pipe
        if (!pUsbFuncs->lpResetPipe(hPipe)) {
            DEBUGMSG( ZONE_USBCLIENT, (TEXT("*** ResetPipe ERROR:%d ***\n"), GetLastError()));
            //ASSERT(0);
            bRc = FALSE;
        }
    }
    
    return bRc;
}


// EOF
