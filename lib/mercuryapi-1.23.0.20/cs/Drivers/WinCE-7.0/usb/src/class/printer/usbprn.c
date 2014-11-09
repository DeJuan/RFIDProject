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
    usbprn.c

Abstract:
    USB Client Driver for Print Device Class.

Functions:

Notes: 

--*/

#include "usbprn.h"

#ifdef DEBUG
DBGPARAM dpCurSettings = {
    TEXT("USBPRN"), {
    TEXT("Errors"),    TEXT("Warnings"),  TEXT("Init"),        TEXT("Trace"),
    TEXT("LPT_INIT"),  TEXT("LPT_READ"),  TEXT("LPT_WRITE"),   TEXT("LPT_IOCTL"),
    TEXT("USB_PARSE"), TEXT("USB_INIT"),  TEXT("USB_CONTROL"), TEXT("USB_BULK"),
    TEXT("Undefined"), TEXT("Undefined"), TEXT("Undefined"),   TEXT("USBCLIENT")  
    },
     0x0003 // ZONE_WRN|ZONE_ERR
};
#endif  // DEBUG


BOOL WINAPI 
DeviceNotify(
   LPVOID lpvNotifyParameter,
   DWORD dwCode,
   LPDWORD * dwInfo1,
   LPDWORD * dwInfo2,
   LPDWORD * dwInfo3,
   LPDWORD * dwInfo4
   );

VOID
RemoveDeviceContext(
   PUSBPRN_CONTEXT pUsbPrn
   );


#ifdef DEBUG
BOOL
UsbPrnTest(
   PUSBPRN_CONTEXT  pUsbPrn
   );
#endif


BOOL 
USBInstallDriver(
   LPCWSTR szDriverLibFile 
   )
{
    BOOL  bRc;

    const WCHAR wsUsbDeviceID[] = CLASS_NAME_SZ;
    WCHAR wsSubClassRegKey[sizeof(CLIENT_REGKEY_SZ)+16] = CLIENT_REGKEY_SZ;

    USB_DRIVER_SETTINGS usbDriverSettings = { DRIVER_SETTINGS };

    DWORD dwPortStatusTimeout = GET_PORT_STATUS_TIMEOUT;
    DWORD dwDeviceIdTimeout   = GET_DEVICE_ID_TIMEOUT;
    DWORD dwSoftResetTimeout  = SOFT_RESET_TIMEOUT;

    DWORD dwReadTimeoutMultiplier  = READ_TIMEOUT_MULTIPLIER;
    DWORD dwReadTimeoutConstant    = READ_TIMEOUT_CONSTANT;
    DWORD dwWriteTimeoutMultiplier = WRITE_TIMEOUT_MULTIPLIER;
    DWORD dwWriteTimeoutConstant   = WRITE_TIMEOUT_CONSTANT;

    REG_VALUE_DESCR usbPrnKeyValues[] = {
        (TEXT("Dll")),               REG_SZ,    0, (PBYTE)(DRIVER_NAME),
        (TEXT("Prefix")),            REG_SZ,    0, (PBYTE)(DEVICE_PREFIX),
        GET_PORT_STATUS_TIMEOUT_SZ,  REG_DWORD, 0, NULL,
        GET_DEVICE_ID_TIMEOUT_SZ,    REG_DWORD, 0, NULL,
        SOFT_RESET_TIMEOUT_SZ,       REG_DWORD, 0, NULL,
        READ_TIMEOUT_MULTIPLIER_SZ,  REG_DWORD, 0, NULL,
        READ_TIMEOUT_CONSTANT_SZ,    REG_DWORD, 0, NULL,
        WRITE_TIMEOUT_MULTIPLIER_SZ, REG_DWORD, 0, NULL,
        WRITE_TIMEOUT_CONSTANT_SZ,   REG_DWORD, 0, NULL,
        NULL, 0, 0, NULL
    };
    usbPrnKeyValues[2].Data = (PUCHAR)(&dwPortStatusTimeout);
    usbPrnKeyValues[3].Data = (PUCHAR)(&dwDeviceIdTimeout);
    usbPrnKeyValues[4].Data = (PUCHAR)(&dwSoftResetTimeout);
    usbPrnKeyValues[5].Data = (PUCHAR)(&dwReadTimeoutMultiplier);
    usbPrnKeyValues[6].Data = (PUCHAR)(&dwReadTimeoutConstant);
    usbPrnKeyValues[7].Data = (PUCHAR)(&dwWriteTimeoutMultiplier);
    usbPrnKeyValues[8].Data = (PUCHAR)(&dwWriteTimeoutConstant);

   DEBUGMSG( ZONE_USB_INIT, (TEXT(">USBInstallDriver(%s)\n"), szDriverLibFile ));
    
    //
   // register with USBD
   //   
   bRc = RegisterClientDriverID( wsUsbDeviceID );
   if ( !bRc ) {
      DEBUGMSG( ZONE_ERR, (TEXT("RegisterClientDriverID error:%d\n"), GetLastError()));
      return FALSE;
   }
        
   bRc = RegisterClientSettings( szDriverLibFile,
                                 wsUsbDeviceID, 
                                 NULL, 
                                 &usbDriverSettings );
   if ( !bRc ) {
      DEBUGMSG( ZONE_ERR, (TEXT("RegisterClientSettings error:%d\n"), GetLastError()));
      return FALSE;
   }

    //
    // Add our default values to the reg
    //
    if ( !GetSetKeyValues( wsSubClassRegKey,
                           &usbPrnKeyValues[0],
                           SET,
                           TRUE ) ) {
        DEBUGMSG( ZONE_ERR, (TEXT("GetSetKeyValues failed!\n")));
        TEST_TRAP();
    }
 
    DEBUGMSG( ZONE_USB_INIT, (TEXT("<USBInstallDriver:%d\n"), bRc ));

   return bRc;
}


BOOL 
USBUnInstallDriver(
   VOID
   )
{
   BOOL bRc;
   const WCHAR wsUsbDeviceID[] = CLASS_NAME_SZ;
   USB_DRIVER_SETTINGS usbDriverSettings = { DRIVER_SETTINGS };

   DEBUGMSG( ZONE_USB_INIT, (TEXT(">USBUnInstallDriver\n")));

   bRc = UnRegisterClientSettings( wsUsbDeviceID,
                                   NULL,
                                   &usbDriverSettings );

   bRc = bRc & UnRegisterClientDriverID( wsUsbDeviceID );

   DEBUGMSG( ZONE_USB_INIT, (TEXT("<USBUnInstallDriver:%d\n"), bRc));

   return bRc;
}


BOOL
USBDeviceAttach(
   USB_HANDLE       hDevice,
   LPCUSB_FUNCS     UsbFuncs,      
   LPCUSB_INTERFACE UsbInterface,
   LPCWSTR          UniqueDriverId,   
   LPBOOL           AcceptControl,      
   LPCUSB_DRIVER_SETTINGS UsbDriverSettings,
   DWORD Unused
    )
{
   BOOL bRc = TRUE;
   PUSBPRN_CONTEXT  pUsbPrn = NULL;
   LPCUSB_INTERFACE pUsbInterface = NULL;
   ULONG ulConfigIndex = 0;

   WCHAR wsSubClassRegKey[sizeof(CLIENT_REGKEY_SZ)+16] = CLIENT_REGKEY_SZ;

   REG_VALUE_DESCR rdTimeouts[] = {
        GET_PORT_STATUS_TIMEOUT_SZ,  REG_DWORD, sizeof(DWORD), (PUCHAR)(NULL),
        GET_DEVICE_ID_TIMEOUT_SZ,    REG_DWORD, sizeof(DWORD), (PUCHAR)(NULL),
        SOFT_RESET_TIMEOUT_SZ,       REG_DWORD, sizeof(DWORD), (PUCHAR)(NULL),
        READ_TIMEOUT_MULTIPLIER_SZ,  REG_DWORD, sizeof(DWORD), (PUCHAR)(NULL),
        READ_TIMEOUT_CONSTANT_SZ,    REG_DWORD, sizeof(DWORD), (PUCHAR)(NULL),
        WRITE_TIMEOUT_MULTIPLIER_SZ, REG_DWORD, sizeof(DWORD), (PUCHAR)(NULL),
        WRITE_TIMEOUT_CONSTANT_SZ,   REG_DWORD, sizeof(DWORD), (PUCHAR)(NULL),
        NULL, 0, 0, NULL
   };

   UNREFERENCED_PARAMETER(UniqueDriverId);
   UNREFERENCED_PARAMETER(UsbDriverSettings);
   UNREFERENCED_PARAMETER(Unused);

   DEBUGMSG( ZONE_USB_INIT, (TEXT(">USBDeviceAttach(0x%x, %s)\n"), hDevice, UniqueDriverId));

   //
   // Determine if we control this USB peripheral...
   //
   *AcceptControl = FALSE;

   do {
      //
      // Parse USB Descriptors
      //
      pUsbInterface = ParseUsbDescriptors(hDevice, 
                                          UsbFuncs, 
                                          UsbInterface,
                                          (LPUSHORT)&ulConfigIndex );
      
      if ( !pUsbInterface ) {
         DEBUGMSG( ZONE_ERR, (TEXT("ParseUsbDescriptors failed!\n") ));
         bRc = FALSE;
         break;
      }

      //
      // we found a device & interface we control, so create our device context
      //
      pUsbPrn = (PUSBPRN_CONTEXT)LocalAlloc( LPTR, sizeof(USBPRN_CONTEXT) );
      if ( !pUsbPrn ) {
         DEBUGMSG( ZONE_ERR, (TEXT("LocalAlloc error:%d\n"), GetLastError() ));
         bRc = FALSE;
         break;
      }

      pUsbPrn->Sig = USB_PRN_SIG;

      InitializeCriticalSection( &pUsbPrn->Lock );

      pUsbPrn->hUsbDevice = hDevice;

      pUsbPrn->bInterfaceNumber = pUsbInterface->Descriptor.bInterfaceNumber;
      pUsbPrn->bAlternateSetting= pUsbInterface->Descriptor.bAlternateSetting;
      pUsbPrn->ConfigIndex      = (USHORT)ulConfigIndex;

      pUsbPrn->UsbFuncs = UsbFuncs;
   
      pUsbPrn->Flags.Open = FALSE;
      pUsbPrn->Flags.UnloadPending = FALSE;

      // create endpoint 0 event
      pUsbPrn->hEP0Event = CreateEvent( NULL, MANUAL_RESET_EVENT, FALSE, NULL);
      if ( !pUsbPrn->hEP0Event ) {
          DEBUGMSG( ZONE_ERR, (TEXT("CreateEvent error:%d\n"), GetLastError() ));
          bRc = FALSE;
          break;
      }

      pUsbPrn->hCloseEvent = CreateEvent( NULL, AUTO_RESET_EVENT, FALSE, NULL);
      if ( !pUsbPrn->hCloseEvent ) {
         DEBUGMSG( ZONE_ERR, (TEXT("CreateEvent error:%d\n"), GetLastError() ));
         bRc = FALSE;
         break;
      }

      //
      // set the USB interface/pipes
      //
      bRc = SetUsbInterface( pUsbPrn, 
                             pUsbInterface );
      if ( !bRc ) {
         DEBUGMSG( ZONE_ERR, (TEXT("SetUsbInterface failed!\n")));
         break;
      }

        //
        // Read the timeout values from the registry
        //
        rdTimeouts[0].Data = (PUCHAR)(&pUsbPrn->UsbTimeouts.PortStatusTimeout);
        rdTimeouts[1].Data = (PUCHAR)(&pUsbPrn->UsbTimeouts.DeviceIdTimeout);
        rdTimeouts[2].Data = (PUCHAR)(&pUsbPrn->UsbTimeouts.SoftResetTimeout);

        rdTimeouts[3].Data = (PUCHAR)(&pUsbPrn->Timeouts.ReadTotalTimeoutMultiplier);
        rdTimeouts[4].Data = (PUCHAR)(&pUsbPrn->Timeouts.ReadTotalTimeoutConstant);
        rdTimeouts[5].Data = (PUCHAR)(&pUsbPrn->Timeouts.WriteTotalTimeoutMultiplier);
        rdTimeouts[6].Data = (PUCHAR)(&pUsbPrn->Timeouts.WriteTotalTimeoutConstant);
        
        if ( !GetSetKeyValues(wsSubClassRegKey,
                                  &rdTimeouts[0],
                                  GET, 
                                  FALSE) ) {
            //
            // use defaults
            //
            pUsbPrn->UsbTimeouts.PortStatusTimeout = GET_PORT_STATUS_TIMEOUT;
            pUsbPrn->UsbTimeouts.DeviceIdTimeout   = GET_DEVICE_ID_TIMEOUT;
            pUsbPrn->UsbTimeouts.SoftResetTimeout  = SOFT_RESET_TIMEOUT;

            pUsbPrn->Timeouts.ReadIntervalTimeout         = READ_TIMEOUT_INTERVAL; // not used
            pUsbPrn->Timeouts.ReadTotalTimeoutMultiplier  = READ_TIMEOUT_MULTIPLIER;
            pUsbPrn->Timeouts.ReadTotalTimeoutConstant    = READ_TIMEOUT_CONSTANT;
            pUsbPrn->Timeouts.WriteTotalTimeoutMultiplier = WRITE_TIMEOUT_MULTIPLIER;
            pUsbPrn->Timeouts.WriteTotalTimeoutConstant   = WRITE_TIMEOUT_CONSTANT;
      }

#ifdef DEBUG
      UsbPrnTest( pUsbPrn );
#endif

      //
      // kick start the stream driver interface
      //
      pUsbPrn->hStreamDevice = ActivateDevice( wsSubClassRegKey,
                                               (DWORD)pUsbPrn );

      if ( pUsbPrn->hStreamDevice ) {
         //
         // register for USB callbacks
         //
         bRc = UsbFuncs->lpRegisterNotificationRoutine( hDevice,
                                                        DeviceNotify,
                                                        pUsbPrn );
         if ( !bRc ) {
            DEBUGMSG( ZONE_ERR, (TEXT("RegisterNotificationRoutine error:%d\n"), GetLastError() ));
            break;
         }

      } else {
         //
         // the streams interface failed to init, no use starting.
         //
         DEBUGMSG( ZONE_ERR, (TEXT("ActivateDevice error:%d\n"), GetLastError() ));
         bRc = FALSE;
         break;
      }
#pragma warning(suppress:4127)
   } while(0);

   if (!bRc) {
      //
      // If not our device, or error, then clean up
      //
      RemoveDeviceContext( pUsbPrn );

   } else {

      *AcceptControl = TRUE;
   
   }

   DEBUGMSG( ZONE_USB_INIT, (TEXT("<USBDeviceAttach:%d\n"), *AcceptControl ));

   return bRc;
}


LPCUSB_INTERFACE
ParseUsbDescriptors(
   USB_HANDLE       hUsbDevice,
   LPCUSB_FUNCS     UsbFuncs,
   LPCUSB_INTERFACE CurInterface,
   LPUSHORT         ConfigIndex
   )
{
   LPCUSB_DEVICE      pDevice;
   LPCUSB_INTERFACE   pUsbInterface;
   LPCUSB_INTERFACE   pDesiredInterface = NULL;
   
   DWORD dwNumInterfaces;
   DWORD bProtocol, dwIndex;

   DEBUGMSG( ZONE_USB_INIT, (TEXT(">ParseUsbDescriptors\n")));

   if ( !hUsbDevice || !UsbFuncs || !CurInterface || !ConfigIndex) {
      DEBUGMSG( ZONE_ERR, (TEXT("Invalid parameter\n")));
      return NULL;
   }

   //
   // get the descriptors
   //
    pDevice = UsbFuncs->lpGetDeviceInfo( hUsbDevice );
   if ( !pDevice ) {
      DEBUGMSG( ZONE_ERR, (TEXT("GetDeviceInfo error:%d\n"), GetLastError() ));
      return NULL;
   }

   DUMP_USB_DEVICE_DESCRIPTOR( pDevice->Descriptor );
   DUMP_USB_CONFIGURATION_DESCRIPTOR( pDevice->lpActiveConfig->Descriptor );  

   // get config index
    for ( *ConfigIndex = 0; *ConfigIndex < (USHORT)pDevice->Descriptor.bNumConfigurations; (*ConfigIndex)++) {
        if (pDevice->lpActiveConfig == (pDevice->lpConfigs + *ConfigIndex)) {
            DEBUGMSG( ZONE_USB_INIT, (TEXT("ConfigIndex:%d\n"), *ConfigIndex));
         break;
      }
    }

    pUsbInterface   = pDevice->lpActiveConfig->lpInterfaces;
   dwNumInterfaces = pDevice->lpActiveConfig->dwNumInterfaces;

   // walk the interfaces searching for best fit
   for ( bProtocol = 0, dwIndex = 0; dwIndex < dwNumInterfaces; pUsbInterface++, dwIndex++) {

      DUMP_USB_INTERFACE_DESCRIPTOR( pUsbInterface->Descriptor, dwIndex );

        if ( pUsbInterface->Descriptor.bInterfaceNumber == CurInterface->Descriptor.bInterfaceNumber ) {
            if (pUsbInterface->Descriptor.bInterfaceClass     == USB_PRN_INTERFACE_CLASS &&
                pUsbInterface->Descriptor.bInterfaceSubClass  == USB_PRN_INTERFACE_SUBCLASS &&
                (pUsbInterface->Descriptor.bInterfaceProtocol == USB_PRN_INTERFACE_PROTOCOL_BI || 
                pUsbInterface->Descriptor.bInterfaceProtocol  == USB_PRN_INTERFACE_PROTOCOL_UNI) ) 
         {
            if (pUsbInterface->Descriptor.bInterfaceProtocol > bProtocol) {
                   pDesiredInterface = pUsbInterface;
                    bProtocol = pUsbInterface->Descriptor.bInterfaceProtocol;
               DEBUGMSG( ZONE_USB_INIT, (TEXT("*** Found interface @ index: %d ***\n"), dwIndex));
               }
            }
        }
    }
  
   DEBUGMSG( ZONE_USB_INIT, (TEXT("<ParseUsbDescriptors:0x%x\n"), pDesiredInterface ));

   return pDesiredInterface;
}


BOOL
SetUsbInterface(
   PUSBPRN_CONTEXT  pUsbPrn,
   LPCUSB_INTERFACE UsbInterface
   )
{
   USB_TRANSFER hTransfer;
   BOOL bRc = FALSE;
   DWORD dwIndex;

   DEBUGMSG( ZONE_USB_INIT, (TEXT(">SetUsbInterface\n")));

   if ( !VALID_CONTEXT( pUsbPrn ) || !UsbInterface ) {
      DEBUGMSG( ZONE_ERR, (TEXT("Invalid parameter\n")));
      return FALSE;
   }

   hTransfer = pUsbPrn->UsbFuncs->lpSetInterface( pUsbPrn->hUsbDevice,
                                                  NULL,
                                                  NULL,
                                                  0, // synchronous
                                                  UsbInterface->Descriptor.bInterfaceNumber,
                                                  UsbInterface->Descriptor.bAlternateSetting );

   if ( !hTransfer ) {
      DEBUGMSG( ZONE_ERR, (TEXT("SetUsbInterface error:%d\n"), GetLastError() ));
      return FALSE;
   }

   //
   // now parse the endpoints
   //
    for ( dwIndex = 0; dwIndex < UsbInterface->Descriptor.bNumEndpoints; dwIndex++) {
    
      LPCUSB_ENDPOINT pEndpoint;
      
      pEndpoint = UsbInterface->lpEndpoints + dwIndex;

      DUMP_USB_ENDPOINT_DESCRIPTOR(pEndpoint->Descriptor);

    // 
    // Printer Class supports 1 mandatory Bulk OUT and 1 optional Bulk IN
    // 
    if ( USB_ENDPOINT_DIRECTION_OUT( pEndpoint->Descriptor.bEndpointAddress ) ) {
            if ( NULL == pUsbPrn->BulkOut.hPipe  &&
              (pEndpoint->Descriptor.bmAttributes & USB_ENDPOINT_TYPE_MASK) == USB_ENDPOINT_TYPE_BULK) {
               //
               // create the Bulk OUT pipe
               //
               pUsbPrn->BulkOut.hPipe = pUsbPrn->UsbFuncs->lpOpenPipe( pUsbPrn->hUsbDevice,
                                                                  &pEndpoint->Descriptor );
               if ( !pUsbPrn->BulkOut.hPipe ) {
                  DEBUGMSG( ZONE_ERR, (TEXT("OpenPipe error:%d\n"), GetLastError() ));
                  bRc = FALSE;
                  TEST_TRAP();
                  break;
               }

               //
               // setup any endpoint specific timers, buffers, context, etc.
               //
               pUsbPrn->BulkOut.hEvent = CreateEvent( NULL, MANUAL_RESET_EVENT, FALSE, NULL);
               if ( !pUsbPrn->BulkOut.hEvent ) {
                  DEBUGMSG( ZONE_ERR, (TEXT("CreateEvent error:%d\n"), GetLastError() ));
                  bRc = FALSE;
                  TEST_TRAP();
                  break;
               }

               pUsbPrn->BulkOut.bIndex = pEndpoint->Descriptor.bEndpointAddress;
         }

        } else if (USB_ENDPOINT_DIRECTION_IN( pEndpoint->Descriptor.bEndpointAddress ) ) {
         if ( NULL == pUsbPrn->BulkIn.hPipe && 
              USB_PRN_INTERFACE_PROTOCOL_BI == UsbInterface->Descriptor.bInterfaceProtocol &&
                  (pEndpoint->Descriptor.bmAttributes & USB_ENDPOINT_TYPE_MASK) == USB_ENDPOINT_TYPE_BULK) {
               //
               // create the Bulk IN pipe
               //
               pUsbPrn->BulkIn.hPipe = pUsbPrn->UsbFuncs->lpOpenPipe( pUsbPrn->hUsbDevice,
                                                                 &pEndpoint->Descriptor );
               if ( !pUsbPrn->BulkIn.hPipe ) {
                  DEBUGMSG( ZONE_ERR, (TEXT("OpenPipe error: %d\n"), GetLastError() ));
                  bRc = FALSE;
                  TEST_TRAP();
                  break;
               }

               //
               // setup any endpoint specific timers, buffers, context, etc.
               //
               pUsbPrn->BulkIn.hEvent = CreateEvent( NULL, MANUAL_RESET_EVENT, FALSE, NULL);
               if ( !pUsbPrn->BulkIn.hEvent ) {
                  DEBUGMSG( ZONE_ERR, (TEXT("CreateEvent error:%d\n"), GetLastError() ));
                  bRc = FALSE;
                  TEST_TRAP();
                  break;
               }

               pUsbPrn->BulkIn.bIndex = pEndpoint->Descriptor.bEndpointAddress;
            }
        
      } else {
         DEBUGMSG( ZONE_WARN, (TEXT("Unsupported Endpoint:0x%x\n"), pEndpoint->Descriptor.bEndpointAddress));
      }
    }

   //
   // did we find our endpoints?
   //
    if ( USB_PRN_INTERFACE_PROTOCOL_UNI == UsbInterface->Descriptor.bInterfaceProtocol ) {
      
      bRc = pUsbPrn->BulkOut.hPipe ? TRUE : FALSE;
   
   } else {
      
      bRc = (pUsbPrn->BulkOut.hPipe && pUsbPrn->BulkIn.hPipe) ? TRUE : FALSE;
   }

   CloseTransferHandle(pUsbPrn->UsbFuncs, hTransfer);

   //
   // if we failed to find all of our endpoints then cleanup will occur later
   //

    DEBUGMSG( ZONE_USB_INIT, (TEXT("<SetUsbInterface:%d\n"), bRc));

    return (bRc);
}


BOOL WINAPI 
DeviceNotify(
   LPVOID lpvNotifyParameter,
   DWORD dwCode,
   LPDWORD * dwInfo1,
   LPDWORD * dwInfo2,
   LPDWORD * dwInfo3,
   LPDWORD * dwInfo4
   )
{
   PUSBPRN_CONTEXT pUsbPrn = (PUSBPRN_CONTEXT )lpvNotifyParameter;
   DWORD dwWaitReturn;
   BOOL bRc;

    UNREFERENCED_PARAMETER(dwInfo1);
    UNREFERENCED_PARAMETER(dwInfo2);
    UNREFERENCED_PARAMETER(dwInfo3);
    UNREFERENCED_PARAMETER(dwInfo4);

   DEBUGMSG( ZONE_USB_INIT, (TEXT(">DeviceNotify\n")));

   if ( !VALID_CONTEXT( pUsbPrn ) ) {
      DEBUGMSG( ZONE_ERR, (TEXT("Invalid Context!\n")));
      return FALSE;
   }
   
   switch(dwCode) {

      case USB_CLOSE_DEVICE:
        DEBUGMSG( ZONE_USB_INIT, (TEXT("USB_CLOSE_DEVICE\n")));
        if (pUsbPrn->Flags.Open) {
           //
           // When an app has an open handle to this device and then the user unplugs the USB cable
           // the driver will unload here. However, the app can a) hang on a removed handle, or
           // b) pass a call to a removed function, e.g., CloseHandle and cause an AV.
           // Set state to reject all I/O and wait for cleanup signal.
           //
           EnterCriticalSection( &pUsbPrn->Lock );
           pUsbPrn->Flags.Open = FALSE;
           pUsbPrn->Flags.UnloadPending = TRUE;
           LeaveCriticalSection( &pUsbPrn->Lock );

           DEBUGMSG( ZONE_USB_INIT, (TEXT("Waiting for CloseEvent...\n")));
           dwWaitReturn = WaitForSingleObject( pUsbPrn->hCloseEvent, INFINITE );

           switch (dwWaitReturn) {

               case WAIT_OBJECT_0:
                  DEBUGMSG( ZONE_USB_INIT, (TEXT("...CloseEvent signalled\n")));
                  break;

               case WAIT_FAILED:
                  DEBUGMSG( ZONE_ERR, (TEXT("CloseEvent error:%d\n"), GetLastError()));
                  TEST_TRAP();
                  break;
               
               default:
                  DEBUGMSG( ZONE_ERR, (TEXT("Unhandled WaitReason:%d\n"), dwWaitReturn ));
                  TEST_TRAP();
                  break;
            }
        
           //
           // Warning: DeactivateDevice forces Device Manager to call our LPT_Deinit, which then
           // causes COREDLL!xxX_CloseHandle to AV when PRNPORT!PrinterSend calls CloseHandle(hPrinter).
           // So, give it a chance to clean up.
           //
           Sleep(1000);
        }

        DEBUGMSG( ZONE_USB_INIT, (TEXT("DeactivateDevice\n")));

        bRc = DeactivateDevice( pUsbPrn->hStreamDevice );

        if ( !bRc ) {
            DEBUGMSG( ZONE_ERR, (TEXT("DeactivateDevice error: %d\n"), GetLastError() ));
            TEST_TRAP();
        }

        RemoveDeviceContext( pUsbPrn );

        return TRUE;

      default:
         DEBUGMSG( ZONE_ERR, (TEXT("Unhandled code:%d\n"), dwCode));
         TEST_TRAP();
         break;
    }
   
   DEBUGMSG( ZONE_USB_INIT, (TEXT("<DeviceNotify\n")));
   
   return FALSE;
}


/*++
Class specific command:
   GET_DEVICE_ID

Return: number of bytesin the Device ID string.
--*/
DWORD
GetDeviceId(
   PUSBPRN_CONTEXT pUsbPrn,
   __out_bcount(dwLen) PBYTE           pBuffer,
   DWORD           dwLen
   )
{
    USB_DEVICE_REQUEST ControlHeader;
    DWORD dwBytesTransferred = 0;
    DWORD dwErr = ERROR_SUCCESS;
    DWORD dwUsbErr = USB_NO_ERROR;

    DEBUGMSG( ZONE_USB_CONTROL, (TEXT(">GetDeviceId\n")));

    if ( !VALID_CONTEXT( pUsbPrn ) || !pBuffer || dwLen <= 2) {
        DEBUGMSG( ZONE_ERR, (TEXT("Invalid parameter\n")));
    } else {
   
        ControlHeader.bmRequestType = USB_REQUEST_DEVICE_TO_HOST | USB_REQUEST_CLASS | USB_REQUEST_FOR_INTERFACE;
        ControlHeader.bRequest = 0;
        ControlHeader.wValue = pUsbPrn->ConfigIndex;
        ControlHeader.wIndex = pUsbPrn->bInterfaceNumber*0x100 + pUsbPrn->bAlternateSetting;
        ControlHeader.wLength = (USHORT)dwLen;

        dwErr = IssueVendorTransfer( pUsbPrn->UsbFuncs,
                                     pUsbPrn->hUsbDevice,
                                     DefaultTransferComplete,
                                     pUsbPrn->hEP0Event,
                                     (USB_IN_TRANSFER | USB_SHORT_TRANSFER_OK),
                                    &ControlHeader,
                                     pBuffer, 0,
                                    &dwBytesTransferred,
                                     pUsbPrn->UsbTimeouts.DeviceIdTimeout,
                                    &dwUsbErr );
      
      if ( ERROR_SUCCESS == dwErr && USB_NO_ERROR == dwUsbErr && dwBytesTransferred ) {
         //
         // remove the 2 byte IEEE-1284 Device ID string length for Port Monitor
         //
          if ( dwBytesTransferred > 2) {
              dwBytesTransferred -= 2;
              // Wrap accesses to pBuffer in a __try/__except
              __try {
                memmove(pBuffer, pBuffer + 2, dwBytesTransferred);
                pBuffer[dwBytesTransferred] = 0;
              }
              __except(EXCEPTION_EXECUTE_HANDLER) {
                dwBytesTransferred = 0;
              }
          } else {
              dwBytesTransferred = 0;
          };

      } else {
         DEBUGMSG( ZONE_ERR, (TEXT("IssueVendorTransfer ERROR:%d 0x%x\n"), dwErr, dwUsbErr));
         IoErrorHandler( pUsbPrn, NULL, 0, dwUsbErr);
      }
   }

    DEBUGMSG( ZONE_USB_CONTROL, (TEXT("<GetDeviceId:%d\n"), dwBytesTransferred));

   return dwBytesTransferred;
}


/*++
Class specific command:
   GET_PORT_STATUS

Return: printer status bits.
--*/
USHORT
GetPortStatus( 
   PUSBPRN_CONTEXT pUsbPrn
   )
{
    USB_DEVICE_REQUEST ControlHeader;
    DWORD  dwBytesTransferred = 0;
    DWORD  dwUsbErr = USB_NO_ERROR;
    DWORD  dwErr = ERROR_SUCCESS;
    USHORT usStatus = 0;

    DEBUGMSG( ZONE_USB_CONTROL, (TEXT(">GetPortStatus\n")));

    if ( !VALID_CONTEXT( pUsbPrn)  ) {
        DEBUGMSG( ZONE_ERR, (TEXT("Invalid parameter\n")));
    } else {

        ControlHeader.bmRequestType = USB_REQUEST_DEVICE_TO_HOST | USB_REQUEST_CLASS | USB_REQUEST_FOR_INTERFACE;
        ControlHeader.bRequest = 1;
        ControlHeader.wValue   = 0;
        ControlHeader.wIndex   = pUsbPrn->bInterfaceNumber;
        ControlHeader.wLength  = 1;

        dwErr = IssueVendorTransfer( pUsbPrn->UsbFuncs,
                                     pUsbPrn->hUsbDevice,
                                     DefaultTransferComplete,
                                     pUsbPrn->hEP0Event,
                                     (USB_IN_TRANSFER | USB_SHORT_TRANSFER_OK),
                                     &ControlHeader,
                                     &usStatus, 0,
                                     &dwBytesTransferred,
                                     pUsbPrn->UsbTimeouts.PortStatusTimeout,
                                     &dwUsbErr );

        if ( ERROR_SUCCESS != dwErr || USB_NO_ERROR != dwUsbErr) {
            DEBUGMSG( ZONE_ERR, (TEXT("IssueVendorTransfer ERROR:%d 0x%x\n"), dwErr, dwUsbErr));
            IoErrorHandler( pUsbPrn, NULL, 0, dwUsbErr);
        }
    }

    DEBUGMSG( ZONE_USB_CONTROL, (TEXT("<GetPortStatus:0x%x\n"), usStatus));

    return usStatus;
}


/*++
Class specific command:
   SOFT_RESET

Return: TRUE if successful, else FALSE.

Note: SoftReset currently only works on the Epson Stylus Color 740,
but times out on the HP Deskjet 810C. H/W problem on the HP?

--*/
BOOL
SoftReset( 
   PUSBPRN_CONTEXT pUsbPrn
   )
{
    USB_DEVICE_REQUEST ControlHeader;
    DWORD  dwBytesTransferred = 0;
    DWORD  dwUsbErr = USB_NO_ERROR;
    DWORD  dwErr = ERROR_SUCCESS;
    BOOL bRc = TRUE;

    DEBUGMSG( ZONE_USB_CONTROL, (TEXT(">SoftReset\n")));

    if ( !VALID_CONTEXT( pUsbPrn) ) {
        DEBUGMSG( ZONE_ERR, (TEXT("Invalid parameter\n")));
        bRc = FALSE;
   } else {

        ControlHeader.bmRequestType = USB_REQUEST_HOST_TO_DEVICE | USB_REQUEST_CLASS | USB_REQUEST_FOR_INTERFACE;
        ControlHeader.bRequest = 2;
        ControlHeader.wValue   = 0;
        ControlHeader.wIndex   = pUsbPrn->bInterfaceNumber;
        ControlHeader.wLength  = 0;

        dwErr = IssueVendorTransfer( pUsbPrn->UsbFuncs,
                                     pUsbPrn->hUsbDevice,
                                     DefaultTransferComplete,
                                     pUsbPrn->hEP0Event,
                                     (USB_OUT_TRANSFER | USB_SHORT_TRANSFER_OK),
                                     &ControlHeader,
                                     NULL, 0,
                                     &dwBytesTransferred,
                                     pUsbPrn->UsbTimeouts.SoftResetTimeout,
                                     &dwUsbErr );

        if ( ERROR_SUCCESS != dwErr || USB_NO_ERROR != dwUsbErr) {
            DEBUGMSG( ZONE_ERR, (TEXT("IssueVendorTransfer ERROR:%d 0x%x\n"), dwErr, dwUsbErr));
            IoErrorHandler( pUsbPrn, NULL, 0, dwUsbErr);
        }
    }

    DEBUGMSG( ZONE_USB_CONTROL, (TEXT("<SoftReset:%d\n"), bRc));

    return bRc;
}


//
// returns Win32 error
//
DWORD 
IoErrorHandler( 
   PUSBPRN_CONTEXT pUsbPrn,
   USB_PIPE        hPipe, // if NULL then uses default endpoint,
   UCHAR           bIndex,
   USB_ERROR       dwUsbErr
   )
{
   DWORD dwErr = ERROR_SUCCESS;

   DEBUGMSG(ZONE_ERR|ZONE_TRACE,(TEXT(">IoErrorHandler(%p, 0x%x)\n"), hPipe, dwUsbErr));

   if ( VALID_CONTEXT( pUsbPrn ) ) {

      switch (dwUsbErr) {

         case USB_STALL_ERROR:
            DEBUGMSG(ZONE_ERR,(TEXT("USB_STALL_ERROR\n")));
            if (hPipe) {
               //
               // reset the pipe in the USB stack
               //
               if (ResetPipe( pUsbPrn->UsbFuncs, hPipe, 0) )
                  //
                  // clear the stall on the device
                  //
                  dwErr = ClearOrSetFeature( pUsbPrn->UsbFuncs,
                                             pUsbPrn->hUsbDevice,
                                             DefaultTransferComplete,
                                             pUsbPrn->hEP0Event,
                                             USB_SEND_TO_ENDPOINT,
                                             USB_FEATURE_ENDPOINT_STALL,
                                             bIndex,
                                             1000,
                                             FALSE );
            } else {
               //
               // reset the default endpoint
               //
               dwErr = ResetDefaultEndpoint(pUsbPrn->UsbFuncs, pUsbPrn->hUsbDevice);
            
            }
            break;

         case USB_DEVICE_NOT_RESPONDING_ERROR:
            DEBUGMSG(ZONE_ERR,(TEXT("USB_DEVICE_NOT_RESPONDING_ERROR\n")));
            //
            // try a SoftReset
            //
            if ( !SoftReset(pUsbPrn) ) {
               //
               // If the device is not responding and we can not soft reset then assume it's dead.
               // WCE can't reset the port yet.
               //
               DEBUGMSG( ZONE_ERR, (TEXT("Dead device\n")));
            }
            break;

         default:
            DEBUGMSG(ZONE_ERR,(TEXT("Unhandled error: 0x%x\n"), dwUsbErr));
            break;
      }

   } else {
      dwErr = ERROR_INVALID_PARAMETER;
      DEBUGMSG(ZONE_ERR,(TEXT("Invalid Parameter\n")));
   }

   DEBUGMSG(ZONE_ERR|ZONE_TRACE,(TEXT("<IoErrorHandler:%d\n"), dwErr));

   return dwErr;
}


BOOL 
DllEntry(
   HANDLE hDllHandle,
   DWORD  dwReason,
   LPVOID lpreserved
   ) 
{
    UNREFERENCED_PARAMETER(hDllHandle);
    UNREFERENCED_PARAMETER(lpreserved);
    switch (dwReason) {

      case DLL_PROCESS_ATTACH:
           DEBUGREGISTER((HINSTANCE)hDllHandle);
       DisableThreadLibraryCalls((HMODULE) hDllHandle);
           break;

      case DLL_PROCESS_DETACH:
           break;

      default:
        break;
    }
    return TRUE;
}


VOID
RemoveDeviceContext(
   PUSBPRN_CONTEXT pUsbPrn
   )
{
   DEBUGMSG(ZONE_INIT,(TEXT(">RemoveDeviceContext(%p)\n"), pUsbPrn));

   if ( VALID_CONTEXT( pUsbPrn ) ) {

      if ( pUsbPrn->Flags.Open ) {
         DEBUGMSG(ZONE_ERR,(TEXT("RemoveDeviceContext on open device!\n")));
         TEST_TRAP();
         return;
      }

      if (pUsbPrn->BulkIn.hPipe && pUsbPrn->UsbFuncs) {
         pUsbPrn->UsbFuncs->lpClosePipe(pUsbPrn->BulkIn.hPipe);
      }
      
      if (pUsbPrn->BulkIn.hEvent) {
         CloseHandle(pUsbPrn->BulkIn.hEvent);
      }
      
      if (pUsbPrn->BulkOut.hPipe && pUsbPrn->UsbFuncs) {
         pUsbPrn->UsbFuncs->lpClosePipe(pUsbPrn->BulkOut.hPipe);
      }

      if (pUsbPrn->hEP0Event) {
         CloseHandle(pUsbPrn->hEP0Event);
      }

      if (pUsbPrn->BulkOut.hEvent) {
         CloseHandle(pUsbPrn->BulkOut.hEvent);
      }

      if (pUsbPrn->hCloseEvent) {
         CloseHandle(pUsbPrn->hCloseEvent);
      }
      
      if (&pUsbPrn->Lock) {
         DeleteCriticalSection( &pUsbPrn->Lock );
      }
      
      LocalFree(pUsbPrn);
   
   } else {
      DEBUGMSG(ZONE_ERR,(TEXT("Invalid Parameter\n")));
   }

   DEBUGMSG(ZONE_INIT,(TEXT("<RemoveDeviceContext\n")));
   
   return;
}


#ifdef DEBUG
BOOL
UsbPrnTest(
   PUSBPRN_CONTEXT  pUsbPrn
   )
{
   BOOL bRc;
   USHORT usStatus;
   DWORD dwErr;

   #define BUF_SIZE 1024
   UCHAR buf[BUF_SIZE];
   DWORD dwLen;

   DEBUGMSG(ZONE_TRACE,(TEXT(">UsbPrnTest\n")));

   if ( !pUsbPrn ) {
      DEBUGMSG( ZONE_ERR, (TEXT("Invalid parameter\n")));
      return FALSE;
   }

   //
   // set stall on the BulkOut endpoint
   //
   dwErr = ClearOrSetFeature( pUsbPrn->UsbFuncs,
                              pUsbPrn->hUsbDevice,
                              DefaultTransferComplete,
                              pUsbPrn->hEP0Event,
                              USB_SEND_TO_ENDPOINT,
                              USB_FEATURE_ENDPOINT_STALL,
                              pUsbPrn->BulkOut.bIndex,
                              1000,
                              TRUE );
   
   if ( ERROR_SUCCESS != dwErr ) {
      DEBUGMSG( ZONE_ERR, (TEXT("SetFeature error:%d\n"), dwErr ));
   }
   
   //
   // clear it
   //
   dwErr = ClearOrSetFeature( pUsbPrn->UsbFuncs,
                              pUsbPrn->hUsbDevice,
                              DefaultTransferComplete,
                              pUsbPrn->hEP0Event,
                              USB_SEND_TO_ENDPOINT,
                              USB_FEATURE_ENDPOINT_STALL,
                              pUsbPrn->BulkOut.bIndex,
                              1000,
                              FALSE );
   
   if ( ERROR_SUCCESS != dwErr ) {
      DEBUGMSG( ZONE_ERR, (TEXT("ClearFeature error:%d\n"), dwErr ));
   }


   if (pUsbPrn->BulkIn.hPipe) {
       //
       // set stall on the BulkIn endpoint
       //
       dwErr = ClearOrSetFeature( pUsbPrn->UsbFuncs,
                                  pUsbPrn->hUsbDevice,
                                  DefaultTransferComplete,
                                  pUsbPrn->hEP0Event,
                                  USB_SEND_TO_ENDPOINT,
                                  USB_FEATURE_ENDPOINT_STALL,
                                  pUsbPrn->BulkIn.bIndex,
                                  1000,
                                  TRUE );
   
       if ( ERROR_SUCCESS != dwErr ) {
          DEBUGMSG( ZONE_ERR, (TEXT("SetFeature error:%d\n"), dwErr ));
       }
   
       //
       // clear it
       //
       dwErr = ClearOrSetFeature( pUsbPrn->UsbFuncs,
                                  pUsbPrn->hUsbDevice,
                                  DefaultTransferComplete,
                                  pUsbPrn->hEP0Event,
                                  USB_SEND_TO_ENDPOINT,
                                  USB_FEATURE_ENDPOINT_STALL,
                                  pUsbPrn->BulkIn.bIndex,
                                  1000,
                                  FALSE );
   
       if ( ERROR_SUCCESS != dwErr ) {
          DEBUGMSG( ZONE_ERR, (TEXT("ClearFeature error:%d\n"), dwErr ));
       }
   }



   //
   // run through class commands
   //
   bRc = SoftReset( pUsbPrn );
   usStatus = GetPortStatus( pUsbPrn );
   dwLen = GetDeviceId( pUsbPrn, buf, BUF_SIZE );

   DEBUGMSG(ZONE_TRACE,(TEXT("<UsbPrnTest\n")));

   return TRUE;
}
#endif

// EOF
