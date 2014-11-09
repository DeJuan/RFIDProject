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
    lpt.c

Abstract:
   LPT_Xxx Streams interface for USB Print Client Driver.

Functions:

Notes:

--*/


#include "usbprn.h"


const TCHAR g_sPortsKey[] = TEXT("Printers\\Ports");


BOOL
RegisterLPTName(
   LPCTSTR ActivePath,
   __out_ecount(PORTNAME_LEN) LPTSTR PortName
   );

BOOL
DeregisterLPTName(
   LPCTSTR PortName
   );

BOOL
RegisterPrinterSettings(
   PUSBPRN_CONTEXT pUsbPrn
   );

BOOL
DeregisterPrinterSettings(
   PUSBPRN_CONTEXT pUsbPrn
   );

PUSBPRN_CONTEXT
GetContextFromReg(
   LPCTSTR  ActivePath
   );

DWORD
LPT_ReadComplete(
   PVOID    Context
   );

DWORD
LPT_WriteComplete(
   PVOID    Context
   );


/*++
Called by Device Manager to initialize the streams interface in response to ActivateDevice.
We passed ActivateDevice a pointer to our device context, but must read it out of the registry as "ClientInfo".

Returns context used in XXX_Open, XXX_PowerDown, XXX_PowerUp, and XXX_Deinit
--*/
PUSBPRN_CONTEXT
LPT_Init(
   PVOID Context
   )
{
   LPTSTR ActivePath = (LPTSTR)Context; // HKLM\Drivers\Active\xx
   PUSBPRN_CONTEXT pUsbPrn = NULL;
   BOOL bRc = FALSE;

   DEBUGMSG(ZONE_LPT_INIT, (TEXT(">LPT_Init(%p)\n"), Context));

   //
   // get our Context
   //
   pUsbPrn = GetContextFromReg( ActivePath );

   //
   // Register our file device (LPT) name
   //
   if ( VALID_CONTEXT( pUsbPrn ) ) {

      EnterCriticalSection(&pUsbPrn->Lock);

      do {

         if ( pUsbPrn->ActivePath ) {
            DEBUGMSG(ZONE_ERR, (TEXT("Existing ActivePath: %s\n"), pUsbPrn->ActivePath));
            TEST_TRAP();
            break;
         }

         pUsbPrn->ActivePath = ActivePath;

         memset( pUsbPrn->PortName, 0, sizeof(pUsbPrn->PortName));

         bRc = RegisterLPTName( ActivePath,
                                pUsbPrn->PortName );
         if ( !bRc ) {
            DEBUGMSG(ZONE_ERR, (TEXT("RegisterLPTName Failed\n")));
            break;
         }

         bRc = RegisterPrinterSettings( pUsbPrn );

         if ( !bRc ) {
            DEBUGMSG(ZONE_ERR, (TEXT("RegisterPrinterSettings Failed\n")));
            break;
         }
#pragma warning(suppress:4127)
      } while (0);

      LeaveCriticalSection(&pUsbPrn->Lock);
   }

   DEBUGMSG(ZONE_LPT_INIT, (TEXT("<LPT_Init:0x%x\n"), pUsbPrn ));

   return (bRc ? pUsbPrn : NULL);
}


BOOL
LPT_Deinit(
   PUSBPRN_CONTEXT pUsbPrn
   )
{
   BOOL  bRc = FALSE;

   DEBUGMSG(ZONE_LPT_INIT, (TEXT(">LPT_Deinit\n")));

   if ( VALID_CONTEXT( pUsbPrn ) ) {

      EnterCriticalSection( &pUsbPrn->Lock );

#if DEBUG
      if (pUsbPrn->Flags.Open) {
         DEBUGMSG(ZONE_ERR, (TEXT("LPT_Deinit on Open Device!\n")));
         TEST_TRAP();
      }
#endif

      bRc = DeregisterLPTName( pUsbPrn->PortName );
      if ( !bRc ) {
         DEBUGMSG(ZONE_ERR, (TEXT("DeregisterLPTName Failed\n")));
      }

      memset( pUsbPrn->PortName, 0, sizeof(pUsbPrn->PortName));

      bRc = DeregisterPrinterSettings( pUsbPrn );
      if ( !bRc ) {
         DEBUGMSG(ZONE_ERR, (TEXT("DeregisterPrinterSettings Failed\n")));
      }

      LeaveCriticalSection(&pUsbPrn->Lock);

   }

   DEBUGMSG(ZONE_LPT_INIT, (TEXT("<LPT_Deinit:%d\n"), bRc));

   return bRc;
}


//
// Returns open context to be used in the
// XXX_Read, XXX_Write, XXX_Seek, and XXX_IOControl functions.
// If the device cannot be opened, this function returns NULL.
//
PUSBPRN_CONTEXT
LPT_Open(
   PUSBPRN_CONTEXT Context,      // context returned by LPT_Init.
   DWORD           AccessCode,   // @parm access code
   DWORD           ShareMode     // @parm share mode
   )
{
   PUSBPRN_CONTEXT pUsbPrn = Context;
   BOOL bRc = TRUE;

   UNREFERENCED_PARAMETER(ShareMode);
   UNREFERENCED_PARAMETER(AccessCode);

   DEBUGMSG(ZONE_LPT_INIT,(TEXT(">LPT_Open(0x%x, 0x%x, 0x%x)\n"),pUsbPrn, AccessCode, ShareMode));

   if ( VALID_CONTEXT( pUsbPrn ) ) {

      EnterCriticalSection(&pUsbPrn->Lock);

        if ( !pUsbPrn->Flags.Open &&
           !pUsbPrn->Flags.UnloadPending ) {

            pUsbPrn->Flags.Open = TRUE;

         ResetEvent( pUsbPrn->hCloseEvent ); // non-signaled

        } else {
            DEBUGMSG( ZONE_ERR,(TEXT("LPT_Open: ERROR_ACCESS_DENIED\n")));
            SetLastError(ERROR_ACCESS_DENIED);
            bRc = FALSE;
        }

      LeaveCriticalSection(&pUsbPrn->Lock);

   } else {
      DEBUGMSG( ZONE_ERR,(TEXT("LPT_Open: ERROR_FILE_NOT_FOUND\n")));
        SetLastError(ERROR_FILE_NOT_FOUND);
      bRc = FALSE;
    }

   DEBUGMSG(ZONE_LPT_INIT,(TEXT("<LPT_Open:%d\n"), bRc ));

   return (bRc ? pUsbPrn : NULL);
}


BOOL
LPT_Close(
   PUSBPRN_CONTEXT Context
   )
{
   PUSBPRN_CONTEXT pUsbPrn = Context;

   DEBUGMSG(ZONE_LPT_INIT,(TEXT("LPT_Close(0x%x)\n"),pUsbPrn));

   if ( VALID_CONTEXT( pUsbPrn ) ) {

      EnterCriticalSection(&pUsbPrn->Lock);

      pUsbPrn->Flags.Open = FALSE;

      LeaveCriticalSection(&pUsbPrn->Lock);

      //
      // Note: any waiters are run as soon as we signal this event
      //
      return SetEvent( pUsbPrn->hCloseEvent );

   } else {
      DEBUGMSG( ZONE_ERR,(TEXT("LPT_Close: ERROR_INVALID_HANDLE\n")));
        SetLastError(ERROR_INVALID_HANDLE);
      return FALSE;
   }
}

#define MAX_USBTRANSFER_SIZE 0x1000 // 4k
//
// Printers are sequential byte processing devices,
// so no need for anything fancy
//
ULONG
LPT_Write(
   PUSBPRN_CONTEXT pUsbPrn,
   __out_bcount(BufferLength) PUCHAR pBuffer,
   ULONG  BufferLength
   )
{
   DWORD dwTimeout;
   DWORD dwTransferSize;
   DWORD dwBytesTransferred ;
   DWORD dwTatalTransferred = 0 ;
   DWORD dwErr = ERROR_SUCCESS;
   DWORD dwUsbErr = USB_NO_ERROR;

   DEBUGMSG(ZONE_LPT_WRITE,(TEXT(">LPT_Write(0x%x, %d)\n"),pBuffer, BufferLength));

   if ( ACCEPT_IO( pUsbPrn ) ) {

      while  ( pBuffer && BufferLength) {

         dwTransferSize=min(BufferLength,MAX_USBTRANSFER_SIZE);
         dwTimeout = dwTransferSize * pUsbPrn->Timeouts.WriteTotalTimeoutMultiplier +
                     pUsbPrn->Timeouts.WriteTotalTimeoutConstant;

         if (!dwTimeout) {
            dwTimeout = INFINITE;
         }

         DEBUGMSG(ZONE_LPT_WRITE, (TEXT("LPT_Write timeout due in %d msec\n"), dwTimeout));
         EnterCriticalSection( &pUsbPrn->Lock );
         dwBytesTransferred = 0 ;
         dwErr = IssueBulkTransfer( pUsbPrn->UsbFuncs,
                                    pUsbPrn->BulkOut.hPipe,
                                    DefaultTransferComplete,  // Callback
                                    pUsbPrn->BulkOut.hEvent,  // Context
                                    (USB_OUT_TRANSFER /*| USB_SHORT_TRANSFER_OK*/), // Flags
                                    pBuffer, 0,
                                    dwTransferSize,
                                   &dwBytesTransferred,
                                    dwTimeout,
                                   &dwUsbErr );

         if ( ERROR_SUCCESS != dwErr || USB_NO_ERROR != dwUsbErr) {
            DEBUGMSG( ZONE_ERR, (TEXT("IssueBulkTransfer error:%d, 0x%x\n"), dwErr, dwUsbErr));
            IoErrorHandler( pUsbPrn, pUsbPrn->BulkOut.hPipe, pUsbPrn->BulkOut.bIndex, dwUsbErr);
            LeaveCriticalSection( &pUsbPrn->Lock );
            break;
         }
         else {
            LeaveCriticalSection( &pUsbPrn->Lock );
            pBuffer +=dwTransferSize;
            BufferLength -=dwTransferSize;
            dwTatalTransferred +=dwTransferSize;
         }

      };
   } else {
      DEBUGMSG( ZONE_ERR,(TEXT("LPT_Write: ERROR_INVALID_HANDLE\n")));
      dwTatalTransferred=(DWORD)-1;
      SetLastError(ERROR_INVALID_HANDLE);
    }

   DEBUGMSG(ZONE_LPT_WRITE,(TEXT("<LPT_Write:%d\n"), dwTatalTransferred ));

   return dwTatalTransferred;
}


ULONG
LPT_Read(
   PUSBPRN_CONTEXT pUsbPrn,
   PUCHAR pBuffer,
   ULONG  BufferLength
   )
{
   DWORD dwTimeout;
   DWORD dwBytesTransferred = 0;
   DWORD dwErr = ERROR_SUCCESS;
   DWORD dwUsbErr = USB_NO_ERROR;

   DEBUGMSG(ZONE_LPT_READ,(TEXT(">LPT_Read(0x%x, %d)\n"),pBuffer, BufferLength));

   if ( ACCEPT_IO( pUsbPrn ) ) {

      if ( pBuffer && BufferLength ) {

         EnterCriticalSection( &pUsbPrn->Lock );

         dwTimeout = BufferLength * pUsbPrn->Timeouts.ReadTotalTimeoutMultiplier +
                     pUsbPrn->Timeouts.ReadTotalTimeoutConstant;

         if (!dwTimeout) {
            dwTimeout = INFINITE;
         }

         DEBUGMSG(ZONE_LPT_READ, (TEXT("LPT_Read timeout due in %d msec\n"), dwTimeout));

         dwErr = IssueBulkTransfer( pUsbPrn->UsbFuncs,
                                    pUsbPrn->BulkIn.hPipe,
                                    DefaultTransferComplete,    // Callback
                                    pUsbPrn->BulkIn.hEvent,     // Context
                                    (USB_IN_TRANSFER | USB_SHORT_TRANSFER_OK), // Flags
                                    pBuffer, 0,
                                    BufferLength,
                                   &dwBytesTransferred,
                                    dwTimeout,
                                   &dwUsbErr );

         if ( ERROR_SUCCESS != dwErr || USB_NO_ERROR != dwUsbErr) {
            DEBUGMSG( ZONE_ERR, (TEXT("IssueBulkTransfer error:%d, 0x%x\n"), dwErr, dwUsbErr));
            IoErrorHandler( pUsbPrn, pUsbPrn->BulkOut.hPipe, pUsbPrn->BulkOut.bIndex, dwUsbErr);
         }

         LeaveCriticalSection( &pUsbPrn->Lock );

      } else {
         DEBUGMSG( ZONE_ERR,(TEXT("LPT_Read: ERROR_INVALID_PARAMETER\n")));
           SetLastError(ERROR_INVALID_PARAMETER );
       }


   } else {
      DEBUGMSG( ZONE_ERR,(TEXT("LPT_Read: ERROR_INVALID_HANDLE\n")));
        SetLastError(ERROR_INVALID_HANDLE);
    }

   DEBUGMSG(ZONE_LPT_READ,(TEXT("<LPT_Read:%d\n"), dwBytesTransferred));

   return dwBytesTransferred;
}



BOOL
LPT_IOControl(
    PUSBPRN_CONTEXT pUsbPrn,
    DWORD dwCode,
    BYTE const*const pBufIn,
    DWORD dwLenIn,
    __out_bcount(dwLenOut) PBYTE pBufOut,
    DWORD dwLenOut,
    PDWORD pdwActualOut
   )
{
    BOOL bRc = TRUE;

    DEBUGMSG(ZONE_LPT_IOCTL,(TEXT(">LPT_IOControl(0x%x, 0x%x, %d, 0x%x)\n"),
        dwCode, pBufIn, dwLenIn, pBufOut, dwLenOut ));

    if ( ACCEPT_IO( pUsbPrn ) ) {
        BOOL fWriteActualOut = FALSE;
        DWORD cbActualOut = 0;

        switch ( dwCode ) {

            case IOCTL_SERIAL_SET_TIMEOUTS:
            case IOCTL_PARALLEL_SET_TIMEOUTS:
                DEBUGMSG(ZONE_LPT_IOCTL, (TEXT("IOCTL_PARALLEL_SET_TIMEOUTS\n")));

                if ( dwLenIn >= sizeof(COMMTIMEOUTS) ) {
                    EnterCriticalSection(&pUsbPrn->Lock);
                    CeSafeCopyMemory(&pUsbPrn->Timeouts, pBufIn, sizeof(pUsbPrn->Timeouts));
                    LeaveCriticalSection(&pUsbPrn->Lock);
                } else {
                    DEBUGMSG( ZONE_ERR,(TEXT("LPT_IOControl:ERROR_INVALID_PARAMETER\n")));
                    SetLastError(ERROR_INVALID_PARAMETER);
                    bRc = FALSE;
                }
                break;

            case IOCTL_SERIAL_GET_TIMEOUTS:
            case IOCTL_PARALLEL_GET_TIMEOUTS:
                DEBUGMSG(ZONE_LPT_IOCTL,(TEXT("IOCTL_PARALLEL_GET_TIMEOUTS\n")));

                fWriteActualOut = TRUE;

                if ( dwLenOut >= sizeof(COMMTIMEOUTS) ) {
                    EnterCriticalSection(&pUsbPrn->Lock);
                    cbActualOut =
                        CeSafeCopyMemory(pBufOut, &pUsbPrn->Timeouts, sizeof(pUsbPrn->Timeouts)) ? sizeof(pUsbPrn->Timeouts) : 0;
                    LeaveCriticalSection(&pUsbPrn->Lock);
                } else {
                    DEBUGMSG( ZONE_ERR,(TEXT("LPT_IOControl:ERROR_INVALID_PARAMETER\n")));
                    SetLastError(ERROR_INVALID_PARAMETER);
                    cbActualOut = 0;
                    bRc = FALSE;
                }
                break;

            case IOCTL_PARALLEL_GETDEVICEID:
                DEBUGMSG(ZONE_LPT_IOCTL,(TEXT("IOCTL_PARALLEL_GETDEVICEID\n")));
                cbActualOut = GetDeviceId(pUsbPrn, pBufOut, dwLenOut);
                fWriteActualOut = TRUE;
                break;

            case IOCTL_PARALLEL_WRITE:
                DEBUGMSG(ZONE_LPT_IOCTL,(TEXT("IOCTL_PARALLEL_WRITE\n")));
                cbActualOut = LPT_Write( pUsbPrn, pBufOut, dwLenOut );
                fWriteActualOut = TRUE;
                break;

            case IOCTL_PARALLEL_STATUS:
                DEBUGMSG(ZONE_LPT_IOCTL,(TEXT("IOCTL_PARALLEL_STATUS\n")));

                fWriteActualOut = TRUE;

                if ( dwLenOut < 4 ) {
                    DEBUGMSG( ZONE_ERR,(TEXT("LPT_IOControl:ERROR_INVALID_PARAMETER\n")));
                    SetLastError(ERROR_INVALID_PARAMETER);
                    cbActualOut = 0;
                    bRc = FALSE;
                } else {
                    //
                    // Get IEEE-1284 status bits, then convert to CE LPTx status.
                    //
                    USHORT usStatus, usBit;
                    DWORD  dwCommErrors = CE_DNS | CE_IOE; // (Device Not Selected | I/O Error)

                    usStatus = GetPortStatus( pUsbPrn );

                    // walk the 1284 bits, converting to CE bits
                    for ( usBit = 0x1; usBit != 0; usBit *= 2 ) {
                        switch ( usBit & usStatus ) {
                            case USBPRN_STATUS_SELECT:
                                DEBUGMSG(ZONE_LPT_IOCTL,(TEXT("USBPRN_STATUS_SELECT\n")));
                                dwCommErrors &= ~CE_DNS;
                                break;

                            case USBPRN_STATUS_NOTERROR:
                                DEBUGMSG(ZONE_LPT_IOCTL,(TEXT("USBPRN_STATUS_NOTERROR\n")));
                                dwCommErrors &= ~CE_IOE;
                                break;

                            case USBPRN_STATUS_PAPEREMPTY:
                                DEBUGMSG(ZONE_LPT_IOCTL,(TEXT("USBPRN_STATUS_PAPEREMPTY\n")));
                                dwCommErrors |= CE_OOP; // Out Of Paper
                                break;

                            case USBPRN_STATUS_TIMEOUT:
                                DEBUGMSG(ZONE_LPT_IOCTL,(TEXT("USBPRN_STATUS_TIMEOUT\n")));
                                dwCommErrors |= CE_PTO; // Printer Time Out
                                break;

                            default:
                                break;
                        }
                    }

                    DEBUGMSG(ZONE_LPT_IOCTL,(TEXT("CE LPTx Status: 0x%x\n"),dwCommErrors));

                    cbActualOut = CeSafeCopyMemory(pBufOut, &dwCommErrors, sizeof(DWORD)) ? sizeof(DWORD) : 0;
                }
                break;

            default:
                DEBUGMSG( ZONE_ERR,(TEXT("LPT_IOControl(0x%x) : ERROR_NOT_SUPPORTED\n"), dwCode));
                SetLastError(ERROR_NOT_SUPPORTED);
                bRc = FALSE;
                break;
        }

        if (fWriteActualOut) {
            CeSafeCopyMemory(pdwActualOut, &cbActualOut, sizeof(DWORD));
        }

    } else {
        DEBUGMSG( ZONE_ERR,(TEXT("LPT_IOControl: ERROR_INVALID_HANDLE\n")));
        SetLastError(ERROR_INVALID_HANDLE);
        bRc = FALSE;
    }

    DEBUGMSG(ZONE_LPT_IOCTL,(TEXT("<LPT_IOControl:%d\n"), bRc));

    return bRc;
}


ULONG
LPT_Seek(
   PVOID Context,
   LONG  Position,
   DWORD Type
   )
{
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(Position);
    UNREFERENCED_PARAMETER(Type);
    DEBUGMSG( ZONE_LPT_INIT, (TEXT("LPT_Seek\n")));
    return (ULONG)-1;
}


BOOL
LPT_PowerUp(
   PVOID Context
   )
{
    UNREFERENCED_PARAMETER(Context);
    return 1;
}


BOOL
LPT_PowerDown(
   PVOID Context
   )
{
    UNREFERENCED_PARAMETER(Context);
    return 1;
}


PUSBPRN_CONTEXT
GetContextFromReg(
   LPCTSTR  ActivePath
   )
{
   PUSBPRN_CONTEXT pUsbPrn = NULL;
   HKEY hKey;
   long lStatus;

   if (ActivePath) {
      //
      // open the registry and read out our context pointer
      // since Dev Mgr doesn't pass it in.
      //
      lStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                              ActivePath,
                              0,
                              0,
                              &hKey);

       if (lStatus==ERROR_SUCCESS) {
         DWORD dwVal;
         DWORD dwType = REG_DWORD;
         DWORD dwValLen = sizeof(dwVal);
         lStatus = RegQueryValueEx( hKey,
                                    TEXT("ClientInfo"),
                                    NULL,
                                    &dwType,
                                    (LPBYTE)(&dwVal),
                                    &dwValLen);

         if (lStatus == ERROR_SUCCESS) {
            // check the signature
            pUsbPrn = (PUSBPRN_CONTEXT)dwVal;
            if ( USB_PRN_SIG != pUsbPrn->Sig ) {
               DEBUGMSG(ZONE_ERR, (TEXT("Invalid signature!!\n")));
               TEST_TRAP();
               pUsbPrn = NULL;
            } else {
               DEBUGMSG(ZONE_LPT_INIT, (TEXT("ActivePath: %s)\n"), ActivePath));
            }
         }

         RegCloseKey(hKey);

      } else {
         DEBUGMSG(ZONE_ERR, (TEXT("Open ActivePath failed\n")));
      }
   }

   return pUsbPrn;
}


BOOL
RegisterLPTName(
   LPCTSTR ActivePath,
   __out_ecount(PORTNAME_LEN) LPTSTR PortName
   )
{
    HKEY hKey;
    BOOL bReturn=FALSE;
    long lStatus;

    if (!ActivePath || !PortName) {
        DEBUGMSG(ZONE_ERR, (TEXT("Invalid parameter\n")));
        return FALSE;
    }

    DEBUGMSG(ZONE_LPT_INIT, (TEXT(">RegisterLPTName\n")));

    //
    // Open \Drivers\Active key reading our "LPTx:" device name,
    // which must then be registered under \Printers\Ports
    //
    lStatus = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        ActivePath,
        0,
        0,
        &hKey);

    if (lStatus==ERROR_SUCCESS) {
        //
        // Query our indexed file device name, e.g. "LPT1:"
        //
        TCHAR DevName[DEVNAME_LEN];
        DWORD dwType=REG_SZ;
        DWORD dwValLen = sizeof(DevName);
        lStatus = RegQueryValueEx(
            hKey,
            TEXT("Name"),
            NULL,
            &dwType,
            (PUCHAR)DevName,
            &dwValLen);

        if (lStatus == ERROR_SUCCESS) {
            //
            // write our device name out to the next available "PortX" value under \Printers\Ports
            //
            HKEY hPrint;
            DWORD dwDisposition;
            TCHAR portStr[MAX_PATH];
            TCHAR portName[PORTNAME_LEN];
            lStatus = RegCreateKeyEx(
                HKEY_LOCAL_MACHINE,
                g_sPortsKey,
                0,
                NULL,
                REG_OPTION_NON_VOLATILE,
                0,
                NULL,
                &hPrint,     // HKEY result
                &dwDisposition);

            if (lStatus==ERROR_SUCCESS) {
                DWORD dwIndex;
                for (dwIndex = 1; dwIndex < MAX_LPT_NAME_INDEX; dwIndex++) {
                    VERIFY(SUCCEEDED(StringCchPrintf(portName,PORTNAME_LEN,TEXT("Port%d"),dwIndex)));
                    portName[PORTNAME_LEN-1]=0;
                    dwType=REG_SZ;
                    dwValLen = sizeof(portStr);
                    lStatus = RegQueryValueEx(
                        hPrint,
                        portName,
                        NULL,
                        &dwType,
                        (PUCHAR)portStr,
                        &dwValLen);
                    if (lStatus!=ERROR_SUCCESS) {
                        break;
                    } else if(_tcsncmp(portStr, DevName, DEVNAME_LEN) == 0) {
                        // found a duplicate entry for this device, use that
                        DEBUGMSG(ZONE_INIT,
                            (_T("RegisterLPTName: found pre-existing entry for '%s' at '%s'\r\n"),
                            DevName, portName));
                        bReturn = TRUE;
                        break;
                    }
                }

                // only create a new entry if necessary
                if(lStatus != ERROR_SUCCESS) {
                    ASSERT(dwIndex<MAX_LPT_NAME_INDEX);
                    dwType=REG_SZ;
                    lStatus = RegSetValueEx(
                        hPrint,
                        portName,
                        0, //NULL,
                        dwType,
                        (PUCHAR)DevName,
                        (_tcslen(DevName)+1)*sizeof(TCHAR));

                    if (lStatus==ERROR_SUCCESS) {
                        // copy to caller
                        VERIFY(SUCCEEDED(StringCchCopy(PortName, PORTNAME_LEN, portName)));
                        bReturn=TRUE;
                        DEBUGMSG(ZONE_LPT_INIT, (TEXT("\\%s\\%s\n"),g_sPortsKey,PortName));
                    }
                }

                RegCloseKey(hPrint);
            }

        } else {
            DEBUGMSG(ZONE_ERR,(TEXT("RegisterLPTName can not get value: %s\\Name\n"),ActivePath));
        }

        RegCloseKey(hKey);

    } else {
        DEBUGMSG(ZONE_ERR,(TEXT("RegisterLPTName can not open: %s\n"),ActivePath));
    }

    DEBUGMSG(ZONE_LPT_INIT, (TEXT("<RegisterLPTName\n")));

    return bReturn;
}


BOOL
DeregisterLPTName(
   LPCTSTR PortName
   )
{
   HKEY hKey;
   long lStatus;

   if (!PortName) {
      DEBUGMSG(ZONE_ERR, (TEXT("Invalid parameter\n")));
      return FALSE;
   }

   DEBUGMSG(ZONE_LPT_INIT,(TEXT("DeregisterLPTName: \\%s\\%s\n"),g_sPortsKey,PortName));

   lStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                           g_sPortsKey,
                           0,
                           0,
                           &hKey);

    if (lStatus==ERROR_SUCCESS) {
        BOOL bReturn = (RegDeleteValue(hKey,PortName)==ERROR_SUCCESS);
        RegCloseKey(hKey);
        return bReturn;
    }
    else
        return FALSE;
}


//#pragma warning( push )
//#pragma warning( disable : 4706 ) // assignment within conditional expression
/*++

Populates the Registry with our Printer Driver Settings
[HKEY_LOCAL_MACHINE\Printers\<DESCRIPTION>
   "Driver"="pcl.dll"      // names the DLL that contains the printer driver
   "High Quality"="300"    // the resolution of high-quality mode
   "Draft Quality"="150"   // the resolution of draft-quality mode (optional)
   "Color"="Monochrome"    // literal string "Color" -or- "Monochrome"

--*/
BOOL
RegisterPrinterSettings(
   PUSBPRN_CONTEXT pUsbPrn
   )
{

#define MAX_LEN 900

   char *p;
   int i;
   BOOL bRc = TRUE;
   BYTE buf[MAX_LEN];
   TCHAR tBuffer[MAX_LEN];

   BYTE cDesc[MAX_LEN] = "Printers\\";

   DWORD dwIdLen,dwIndex;

   REG_VALUE_DESCR keyValues[] = {
      (TEXT("Driver")),        REG_SZ, 0, (PBYTE)(TEXT("pcl.dll")),     // 0 default: WCE currently does not support others, e.g. PostScript
      (TEXT("High Quality")),  REG_SZ, 0, (PBYTE)(TEXT("300")),         // 1 default
      (TEXT("Draft Quality")), REG_SZ, 0, (PBYTE)(TEXT("150")),         // 2 default
      (TEXT("Color")),         REG_SZ, 0, (PBYTE)(TEXT("Monochrome")),  // 3 default
      NULL, 0, 0, NULL                                                  // 4
   };


   DEBUGMSG(ZONE_LPT_INIT, (TEXT(">RegisterPrinterSettings\n")));

   if (!pUsbPrn) {
      DEBUGMSG(ZONE_ERR, (TEXT("Invalid parameter\n")));
      return FALSE;
   }

   //
   // read the ID string from the printer
   //
   dwIdLen = GetDeviceId( pUsbPrn, buf, MAX_LEN);

   if (dwIdLen) {
      //
      // parse for CMD
      //
      // ...TBD

      //
      // parse for Color support
      //
      p = (char*)buf;
      while( (p = strchr(p, '$'))!=NULL ) {
         if (p[2] == 'C') {
            keyValues[3].Data = (PBYTE)(TEXT("Color"));
            break;
         }
         p++;
      }

      //
      // parse for a Description string
      //
      if ((p = strstr((const char*)buf, "DESCRIPTION:"))!=NULL)
           p += sizeof("DESCRIPTION:") - 1;

      else if ((p = strstr((const char*)buf, "DES:"))!=NULL)
           p += sizeof("DES:") - 1;

      else if ((p = strstr((const char*)buf, "MODEL:"))!=NULL)
           p += sizeof("MODEL:") - 1;

      else if ((p = strstr((const char*)buf, "MDL:"))!=NULL)
           p += sizeof("MDL:") -1; 

      if (p) {
         if (strchr(p, ';')) {
            i = strchr(p, ';') - p;
            p[i] = 0;
         } else {
            i = strlen(p);
         }

        // concat string to \Printers\<DESCRIPTION>
         VERIFY(SUCCEEDED(StringCchCatA((char*)cDesc, _countof(cDesc), p)));
        for (dwIndex =0 ; dwIndex< MAX_LEN-1 &&  cDesc[dwIndex]!=0 ;dwIndex++)
            tBuffer[dwIndex]=cDesc[dwIndex];
        tBuffer[dwIndex]=0; // Terminated.

         DEBUGMSG(ZONE_LPT_INIT, (TEXT("DESCRIPTION: %s\n"), tBuffer ));

         //
         // finally, add it to HKEY_LOCAL_MACHINE\Printers\<DESCRIPTION> if it does not already exist.
         //
         bRc = GetSetKeyValues(tBuffer,
                                &keyValues[0],
                                SET,
                                FALSE );

         if ( !bRc ) {
            DEBUGMSG( ZONE_ERR, (TEXT("GetSetKeyValues failed!\n")));
         }

      } else {
         DEBUGMSG(ZONE_ERR, (TEXT("Failed to find DES!\n")));
         bRc = FALSE;
      }

   } else {
      bRc = FALSE;
   }

   DEBUGMSG(ZONE_LPT_INIT, (TEXT("<RegisterPrinterSettings:%d\n"), bRc));

   return bRc;
}
//#pragma warning( pop )


//
// Currently leave the info in the registry.
//
BOOL
DeregisterPrinterSettings(
   PUSBPRN_CONTEXT pUsbPrn
   )
{
   BOOL bRc = TRUE;

   if (!pUsbPrn) {
      DEBUGMSG(ZONE_ERR, (TEXT("Invalid parameter\n")));
      return FALSE;
   }

   DEBUGMSG(ZONE_LPT_INIT, (TEXT(">DeregisterPrinterSettings\n")));

   // RegDeleteKey ...

   DEBUGMSG(ZONE_LPT_INIT, (TEXT("<DeregisterPrinterSettings:%d\n"), bRc));

   return bRc;
}

// EOF
