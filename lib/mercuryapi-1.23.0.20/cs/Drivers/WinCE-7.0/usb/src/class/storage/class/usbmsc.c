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

    usbmsc.c

Abstract:

    USB Mass Storage Class.
        Bulk-Only Transport 1.0 (BOT)
        Control/Bulk/Interrupt Transport 1.0 (CBIT and CBT)

Functions:

Notes: 

--*/


#include "usbmscp.h"

#include "bot.h"
#include "cbit.h"

#include <Pkfuncs.h> // for LoadDriver


LONG g_NumDevices = 0;


#ifdef DEBUG
DBGPARAM dpCurSettings = {
    TEXT("USBMSC"), {
    TEXT("Errors"),    TEXT("Warnings"),  TEXT("Init"),        TEXT("Trace"),
    TEXT("USB_PARSE"), TEXT("USB_INIT"),  TEXT("USB_CONTROL"), TEXT("USB_BULK"),
    TEXT("USB_INT"),   TEXT("BOT"),       TEXT("CBI"),         TEXT("CBIT"),
    TEXT("TIME"),      TEXT("Undefined"), TEXT("Undefined"),   TEXT("Undefined")  
    },
    ZONEMASK_WARN | ZONEMASK_ERR
};
#endif  // DEBUG

//*****************************************************************************
//
// F U N C T I O N    P R O T O T Y P E S
//
//*****************************************************************************

LPCUSB_INTERFACE
ParseUsbDescriptors(
   USB_HANDLE       hUsbDevice,
   LPCUSB_FUNCS     UsbFuncs,
   LPCUSB_INTERFACE CurInterface,
   __out LPUSHORT         ConfigIndex
   );

VOID
RemoveDeviceContext(
   PUSBMSC_DEVICE pUsbDevice
   );

BOOL
SetUsbInterface(
   PUSBMSC_DEVICE pUsbDevice,
   LPCWSTR        UniqueDriverId
   );

BOOL WINAPI 
UsbDeviceNotify(
   LPVOID lpvNotifyParameter,
   DWORD  dwCode,
   LPDWORD *dwInfo1,
   LPDWORD *dwInfo2,
   LPDWORD *dwInfo3,
   LPDWORD *dwInfo4
   );

BOOL
UsbDeviceTest(
   PUSBMSC_DEVICE  pUsbDevice
   );

DWORD 
GetMaxLUN(HANDLE hTransport, PUCHAR pLun);

PVOID CreateBulkTransferMgr(
    LPCUSB_FUNCS  lpUsbFuncs, USB_PIPE hPipe, LPCUSB_ENDPOINT_DESCRIPTOR lpEndpointDesc,LPCTSTR szUniqueDriverId
    );

VOID DeleteBulkTransferMgr(LPVOID lpContent);

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


//*****************************************************************************
//
//  U S B D      I N T E R F A C E
//
//*****************************************************************************
BOOL 
USBInstallDriver(
   LPCWSTR szDriverLibFile 
   )
{
    BOOL  bRc;
    UCHAR i;

    const WCHAR wsUsbDeviceID[] = CLASS_NAME_SZ;

    USB_DRIVER_SETTINGS usbDriverSettings = { USBMSC_DRIVER_SETTINGS };

    // reg entries for Disk SubClass drivers
    WCHAR wsSubClassRegKey[sizeof(CLIENT_REGKEY_SZ)+16] = CLIENT_REGKEY_SZ;
    const ULONG index = (sizeof(CLIENT_REGKEY_SZ)-2)/2;
    DWORD dwIoctl = DEFAULT_IOCTL;

    REG_VALUE_DESCR rdSubClassValues[] = {
        DLL_SZ,    REG_SZ,    0, (PUCHAR)(DEFAULT_DISK_SZ),
        PREFIX_SZ, REG_SZ,    0, (PUCHAR)(DEFAULT_PREFIX_SZ),
        FSD_SZ,    REG_SZ,    0, (PUCHAR)(DEFAULT_FSD_SZ),
        FOLDER_SZ, REG_SZ,    0, (PUCHAR)(DEFAULT_FOLDER_SZ),
        IOCTL_SZ,  REG_DWORD, 0, NULL,
        NULL, 0, 0, NULL
    };

    DWORD dwReset   = RESET_TIMEOUT;
    DWORD dwCommand = COMMAND_BLOCK_TIMEOUT;
    DWORD dwStatus  = COMMAND_STATUS_TIMEOUT;

    REG_VALUE_DESCR rdTimeouts[] = {
        RESET_TIMEOUT_SZ,           REG_DWORD, 0, NULL,
        COMMAND_BLOCK_TIMEOUT_SZ,   REG_DWORD, 0, NULL,
        COMMAND_STATUS_TIMEOUT_SZ,  REG_DWORD, 0, NULL,
        NULL, 0, 0, NULL
    };
    rdSubClassValues[4].Data = (PUCHAR)(&dwIoctl);

    rdTimeouts[0].Data = (PUCHAR)(&dwReset);
    rdTimeouts[1].Data = (PUCHAR)(&dwCommand);
    rdTimeouts[2].Data = (PUCHAR)(&dwStatus);


    DEBUGMSG( ZONE_USB_INIT, (TEXT("USBMSC>USBInstallDriver(%s)\n"), szDriverLibFile ));

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
    // Add our default Timeout values to the reg
    //
    if ( !GetSetKeyValues( wsSubClassRegKey,
                               &rdTimeouts[0],
                               SET,
                               TRUE ) ) {

        DEBUGMSG( ZONE_ERR, (TEXT("GetSetKeyValues failed!\n")));
        TEST_TRAP();
    }

    //
    // Create our Disk SubClass Reg entries.
    //
    for (i = USBMSC_SUBCLASS_RBC; i <= USBMSC_SUBCLASS_SCSI; i++) {
        
        VERIFY(SUCCEEDED(StringCchPrintf( &wsSubClassRegKey[index], _countof(wsSubClassRegKey)-index, TEXT("\\%d"), i )));
        
        if ( !GetSetKeyValues( wsSubClassRegKey,
                                   rdSubClassValues,
                                   SET,
                                   FALSE ) ) {

            DEBUGMSG( ZONE_ERR, (TEXT("GetSetKeyValues failed!\n")));
            TEST_TRAP();
        }
    }

    DEBUGMSG( ZONE_USB_INIT, (TEXT("USBMSC<USBInstallDriver:%d\n"), bRc ));

    return bRc;
}


BOOL 
USBUnInstallDriver(
   VOID
   )
{
   BOOL bRc;

   const WCHAR wsUsbDeviceID[] = CLASS_NAME_SZ;
   
   USB_DRIVER_SETTINGS usbDriverSettings = { USBMSC_DRIVER_SETTINGS };

   DEBUGMSG( ZONE_USB_INIT, (TEXT("USBMSC>USBUnInstallDriver\n")));

   TEST_TRAP();

   bRc = UnRegisterClientSettings( wsUsbDeviceID,
                                   NULL,
                                   &usbDriverSettings );

   bRc = bRc & UnRegisterClientDriverID( wsUsbDeviceID );

   DEBUGMSG( ZONE_USB_INIT, (TEXT("USBMSC<USBUnInstallDriver:%d\n"), bRc));

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
    BOOL             bRc = TRUE;
    ULONG            ulConfigIndex = 0;
    PUSBMSC_DEVICE   pUsbDevice = NULL;
    LPCUSB_INTERFACE pUsbInterface = NULL;
    PUSBDISK_ATTACH  DiskAttach;
    UCHAR bInterfaceSubClass;
    UCHAR bTempInterfaceSubClass = 0xFF; // invalid subclass
    UCHAR uMaxLun;
    UINT    uiIndex; // Working Unit.

    // reg entries for Disk SubClass drivers
    WCHAR wsSubClassRegKey[sizeof(CLIENT_REGKEY_SZ)+16] = CLIENT_REGKEY_SZ;
    const ULONG index = (sizeof(CLIENT_REGKEY_SZ)-2)/2;
    REG_VALUE_DESCR RegVal[2] = {0};
    WCHAR wsDriverName[MAX_DLL_LEN];

    REG_VALUE_DESCR rdTimeouts[] = {
        RESET_TIMEOUT_SZ,           REG_DWORD, sizeof(DWORD), (PUCHAR)(NULL),
        COMMAND_BLOCK_TIMEOUT_SZ,   REG_DWORD, sizeof(DWORD), (PUCHAR)(NULL),
        COMMAND_STATUS_TIMEOUT_SZ,  REG_DWORD, sizeof(DWORD), (PUCHAR)(NULL),
        NULL, 0, 0, NULL
    };

    UNREFERENCED_PARAMETER(UniqueDriverId);
    UNREFERENCED_PARAMETER(Unused);


    DEBUGMSG( ZONE_USB_INIT, (TEXT("USBMSC>USBDeviceAttach(0x%x, %s)\n"), hDevice, UniqueDriverId));

    //
    // Determine if we control this USB peripheral...
    //
    *AcceptControl = FALSE;

    do {

        if ( USBMSC_INTERFACE_CLASS != UsbDriverSettings->dwInterfaceClass) {
            DEBUGMSG( ZONE_ERR, (TEXT("Not our Device Class:0x%x\n"), UsbDriverSettings->dwInterfaceClass ));
            bRc = FALSE;
            break;
        }
                
        //
        // Parse USB Descriptors
        //
        pUsbInterface = ParseUsbDescriptors( hDevice, 
                                             UsbFuncs, 
                                             UsbInterface,
                                             (LPUSHORT)&ulConfigIndex );

        if ( !pUsbInterface ) {
            DEBUGMSG( ZONE_ERR, (TEXT("ParseUsbDescriptors failed!\n") ));
            bRc = FALSE;
            break;
        }

        //
        // we found a device, interface, & protocol we can control, so create our device context
        //
        pUsbDevice = (PUSBMSC_DEVICE)LocalAlloc( LPTR, sizeof(USBMSC_DEVICE) );
        if ( !pUsbDevice ) {
            DEBUGMSG( ZONE_ERR, (TEXT("LocalAlloc error:%d\n"), GetLastError() ));
            bRc = FALSE;
            break;
        }

        pUsbDevice->Sig = USBMSC_SIG;

        InitializeCriticalSection( &pUsbDevice->Lock );

        pUsbDevice->hUsbDevice = hDevice;

        pUsbDevice->pUsbInterface = pUsbInterface;
        pUsbDevice->ConfigIndex   = (USHORT)ulConfigIndex;

        pUsbDevice->UsbFuncs = UsbFuncs;

        pUsbDevice->Flags.AcceptIo      = FALSE;
        pUsbDevice->Flags.DeviceRemoved = FALSE;

        pUsbDevice->dwMaxLun=0;
        //
        // set the USB interface/pipes
        //
        bRc = SetUsbInterface( pUsbDevice, UniqueDriverId );
        if ( !bRc ) {
            DEBUGMSG( ZONE_ERR, (TEXT("SetUsbInterface failed!\n")));
            break;
        }

        // create endpoint 0 event
        pUsbDevice->hEP0Event = CreateEvent( NULL, MANUAL_RESET_EVENT, FALSE, NULL);
        if ( !pUsbDevice->hEP0Event ) {
            DEBUGMSG( ZONE_ERR, (TEXT("CreateEvent error:%d\n"), GetLastError() ));
            bRc = FALSE;
            break;
        }

        //
        // Read the timeout values from the registry
        //
        rdTimeouts[0].Data = (PUCHAR)(&pUsbDevice->Timeouts.Reset);
        rdTimeouts[1].Data = (PUCHAR)(&pUsbDevice->Timeouts.CommandBlock);
        rdTimeouts[2].Data = (PUCHAR)(&pUsbDevice->Timeouts.CommandStatus);
        
        if ( !GetSetKeyValues(wsSubClassRegKey,
                                  &rdTimeouts[0],
                                  GET, 
                                  FALSE) ) {
            //
            // use defaults
            //
            pUsbDevice->Timeouts.Reset         = RESET_TIMEOUT;
            pUsbDevice->Timeouts.CommandBlock  = COMMAND_BLOCK_TIMEOUT;
            pUsbDevice->Timeouts.CommandStatus = COMMAND_STATUS_TIMEOUT;
        }

        if (!pUsbDevice->Timeouts.Reset)
            pUsbDevice->Timeouts.Reset = RESET_TIMEOUT;

        if (!pUsbDevice->Timeouts.CommandBlock)
            pUsbDevice->Timeouts.CommandBlock = COMMAND_BLOCK_TIMEOUT;

        if (!pUsbDevice->Timeouts.CommandStatus)
            pUsbDevice->Timeouts.CommandStatus = COMMAND_STATUS_TIMEOUT;

        //
        // Load the USB Disk Driver based on the bInterfaceSubClass code.
        // The USB Disk Driver is named by convention USBDISKxx.DLL, 
        // where 'xx' is a valid bInterfaceSubClass code.
        //
        // To override the default disk driver stuff the replacement driver subkey in the registry.
        // If the named subkey does not exist then retry with SCSI as the default driver.
        //
        bInterfaceSubClass = pUsbDevice->pUsbInterface->Descriptor.bInterfaceSubClass;
        ASSERT( (bInterfaceSubClass >= USBMSC_SUBCLASS_RBC) && 
                (bInterfaceSubClass <= USBMSC_SUBCLASS_SCSI) ||
                bInterfaceSubClass == USBMSC_SUBCLASS_RESERVED);

_retryDefault:
        VERIFY(SUCCEEDED(StringCchPrintf( &wsSubClassRegKey[index], _countof(wsSubClassRegKey)-index, TEXT("\\%d"), bInterfaceSubClass ))); 
        
        RegVal[0].Name = DLL_SZ;
        RegVal[0].Type = REG_SZ;
        RegVal[0].Size = MAX_DLL_LEN;
        RegVal[0].Data = (PUCHAR)wsDriverName;

// We do not care about 0xFF == bTempInterfaceSubClass causing problems with different locales
// as the USB stack is not concerned with locale info
#pragma warning(push)
#pragma warning(disable:30033)

        if ( !GetSetKeyValues( wsSubClassRegKey,
                               &RegVal[0],
                               GET, 
                               FALSE ) ) {

            if (0xFF == bTempInterfaceSubClass) {
                // retry using SCSI
                DEBUGMSG( ZONE_WARN, (TEXT("Retry SubClass:0x%x with 0x%x\n"), bInterfaceSubClass, USBMSC_SUBCLASS_SCSI));
                bTempInterfaceSubClass = bInterfaceSubClass;
                bInterfaceSubClass = USBMSC_SUBCLASS_SCSI;
                goto _retryDefault;
            } else {
                DEBUGMSG( ZONE_ERR, (TEXT("GetSetKeyValues failed!\n")));
                bRc = FALSE;
                break;
            }
        }

        if (0xFF != bTempInterfaceSubClass) {
            bInterfaceSubClass = bTempInterfaceSubClass;
        }

#pragma warning(pop)

        pUsbDevice->hDiskDriver = LoadDriver( wsDriverName );
        if ( !pUsbDevice->hDiskDriver ) {
            DEBUGMSG( ZONE_ERR, (TEXT("LoadDriver error:%d on %s\n"), GetLastError(), wsDriverName ));
            bRc = FALSE;
            break;
        }
        pUsbDevice->Index = InterlockedIncrement(&g_NumDevices);

        //
        // get DiskAttach
        //
        DiskAttach = (PUSBDISK_ATTACH)GetProcAddress( pUsbDevice->hDiskDriver, TEXT("UsbDiskAttach") );
        if ( !DiskAttach ) {
            DEBUGMSG( ZONE_ERR, (TEXT("GetProcAddress error:%d on %s\n"), GetLastError(), wsDriverName ));
            bRc = FALSE;
            break;
        }

        //
        // Save DiskDetach callback & Context to call when we get the device Notify
        //
        pUsbDevice->DiskDetach = (PUSBDISK_DETACH)GetProcAddress( pUsbDevice->hDiskDriver, TEXT("UsbDiskDetach") );
        if ( !pUsbDevice->DiskDetach ) {
            DEBUGMSG( ZONE_ERR, (TEXT("GetProcAddress error:%d on %s\n"), GetLastError(), wsDriverName ));
            bRc = FALSE;
            break;
        }

        //
        // register for USB callbacks
        //
        bRc = UsbFuncs->lpRegisterNotificationRoutine( hDevice,
                                                       UsbDeviceNotify,
                                                       pUsbDevice );
        if ( !bRc ) {
            DEBUGMSG( ZONE_ERR, (TEXT("RegisterNotificationRoutine error:%d\n"), GetLastError() ));
            break;
        }

        // signal we can take I/O
        pUsbDevice->Flags.AcceptIo = TRUE;

        //
        // Call the driver's DiskAttach.
        // DiskAttach returns non-null Context on success, else null.
        //
        for (uiIndex=0;uiIndex<MAX_LUN;uiIndex++)
           pUsbDevice->DiskContext[uiIndex]=NULL;
        // Get Max LUN.
        if (GetMaxLUN(pUsbDevice, &uMaxLun)!=ERROR_SUCCESS)
            uMaxLun=1;// Using 1 as default;
        pUsbDevice->dwMaxLun= uMaxLun;
        ASSERT(pUsbDevice->dwMaxLun>=1);
        for (uiIndex=0;uiIndex<uMaxLun;uiIndex++) {
           __try {
                pUsbDevice->DiskContext[uiIndex]= DiskAttach((HANDLE)pUsbDevice, 
                                                     wsSubClassRegKey,
                                                     (DWORD)uiIndex,
                                                     bInterfaceSubClass);
                if ( !pUsbDevice->DiskContext[uiIndex] ) {
                    DEBUGMSG( ZONE_ERR, (TEXT("DiskAttach error:%d on %s\n"), GetLastError(), wsDriverName ));
                    pUsbDevice->Flags.AcceptIo = FALSE;
                    bRc = FALSE;
                    break;
                }
            } __except ( EXCEPTION_EXECUTE_HANDLER ) {
                DEBUGMSG(ZONE_ERR,(TEXT("USBMSC::DiskAttach:EXCEPTION:0x%x\n"), GetExceptionCode()));
                TEST_TRAP();
                bRc = FALSE;
            }
        }
#pragma warning(suppress:4127)
    } while(0);

    if (!bRc) {
        //
        // If not our device, or error, then clean up
        //
        RemoveDeviceContext( pUsbDevice );

    } else {

        *AcceptControl = TRUE;

        UsbDeviceTest( pUsbDevice );

    }

    DEBUGMSG( ZONE_USB_INIT, (TEXT("USBMSC<USBDeviceAttach:%d\n"), *AcceptControl ));

    return bRc;
}


//
// Warning: do not assume that the USB stack will call this routine immediately after the device is unplugged.
// It could take some time for it to cleanup and get back to us. 
//
BOOL WINAPI 
UsbDeviceNotify(
   LPVOID lpvNotifyParameter,
   DWORD dwCode,
   LPDWORD * dwInfo1,
   LPDWORD * dwInfo2,
   LPDWORD * dwInfo3,
   LPDWORD * dwInfo4
   )
{
   PUSBMSC_DEVICE pUsbDevice = (PUSBMSC_DEVICE)lpvNotifyParameter;

   UNREFERENCED_PARAMETER(dwInfo1);
   UNREFERENCED_PARAMETER(dwInfo2);
   UNREFERENCED_PARAMETER(dwInfo3);
   UNREFERENCED_PARAMETER(dwInfo4);

   DEBUGMSG( ZONE_USB_INIT, (TEXT("USBMSC>UsbDeviceNotify\n")));

   if ( !VALID_CONTEXT( pUsbDevice ) ) {
      DEBUGMSG( ZONE_ERR, (TEXT("Invalid Context!\n")));
      TEST_TRAP();
      return FALSE;
   }

   switch(dwCode) {

      case USB_CLOSE_DEVICE:
        DEBUGMSG( ZONE_USB_INIT, (TEXT("USB_CLOSE_DEVICE\n")));
        if (pUsbDevice->Flags.AcceptIo) {
           //
           // set state that we are being removed
           // and no longer accepting I/O
           // 
           EnterCriticalSection( &pUsbDevice->Lock );
           pUsbDevice->Flags.AcceptIo = FALSE;
           pUsbDevice->Flags.DeviceRemoved = TRUE;
           LeaveCriticalSection( &pUsbDevice->Lock );
           
           //
           // cancel any outstanding I/O
           // ...


           //
           // call the Disk's DiskDetach callback. DiskDetach should be able to cope with NULL Context.
           //
           __try {
                if ( pUsbDevice->DiskDetach ) {
                    DWORD dwIndex;
                    for (dwIndex=0;dwIndex<pUsbDevice->dwMaxLun;dwIndex++)
                        if (pUsbDevice->DiskContext[dwIndex]==NULL ||
                                !pUsbDevice->DiskDetach(pUsbDevice->DiskContext[dwIndex]) ) {
                            DEBUGMSG( ZONE_ERR, (TEXT("DiskDetach(Lun=%d) error:%d\n"),dwIndex, GetLastError() ));
                        }
                }
            } __except ( EXCEPTION_EXECUTE_HANDLER ) {
                DEBUGMSG(ZONE_ERR,(TEXT("USBMSC::DiskDetach:EXCEPTION:0x%x\n"), GetExceptionCode()));
                TEST_TRAP();
            }
            
            if (0 == InterlockedDecrement(&g_NumDevices) ) {
                if ( pUsbDevice->hDiskDriver && !FreeLibrary( pUsbDevice->hDiskDriver) ) {
                   DEBUGMSG( ZONE_ERR, (TEXT("FreeLibrary error:%d\n"), GetLastError() ));
                } 
                else 
                    pUsbDevice->hDiskDriver=NULL;
            }
            ASSERT(g_NumDevices >= 0);

        } else {
            ASSERT(0);
        }

        //
        // finally, cleanup this device context
        //
        RemoveDeviceContext( pUsbDevice );

        return TRUE;

      default:
         DEBUGMSG( ZONE_ERR, (TEXT("Unhandled code:%d\n"), dwCode));
         TEST_TRAP();
         break;
    }
   
   DEBUGMSG( ZONE_USB_INIT, (TEXT("USBMSC<UsbDeviceNotify\n")));
   
   return FALSE;
}

#define MAX_MSC_START_DELAY     10000   /* 10 seconds */

LPCUSB_INTERFACE
ParseUsbDescriptors(
   USB_HANDLE       hUsbDevice,
   LPCUSB_FUNCS     UsbFuncs,
   LPCUSB_INTERFACE CurInterface,
   __out LPUSHORT   ConfigIndex
   )
{
    LPCUSB_DEVICE      pDevice;
    LPCUSB_INTERFACE   pUsbInterface;
    LPCUSB_INTERFACE   pDesiredInterface = NULL;

    WCHAR              productRegKey[512];

    DWORD dwNumInterfaces;
    DWORD dwIndex;

    DEBUGMSG( ZONE_USB_INIT, (TEXT("USBMSC>ParseUsbDescriptors\n")));

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

    //
    // pull the product id descriptor so we can see if we
    // need a delay in its startup
    //
    {
        WCHAR *pEnd;
        DWORD dwDelayMs = (DWORD)-1;
        WCHAR pbBuffer[256];
        WORD  cbBuffer = 256;
        WORD  wLangId  = 0x0409;
        BYTE  bIdx     = pDevice->Descriptor.iProduct;
        REG_VALUE_DESCR rdValDesc[] = {
            TEXT("StartDelayMs"), REG_DWORD, sizeof(DWORD), NULL,
            NULL, 0, 0, NULL
        };

        rdValDesc[0].Data = (PUCHAR)(&dwDelayMs);

        wcscpy_s(productRegKey, 512, CLIENT_REGKEY_SZ);
        productRegKey[511] = 0;
        pEnd = productRegKey;
        while (*pEnd)
            pEnd++;

        if (bIdx)
        {
           UsbFuncs->lpGetDescriptor( hUsbDevice, NULL, NULL, USB_SHORT_TRANSFER_OK,
                                      USB_STRING_DESCRIPTOR_TYPE, bIdx,
                                      wLangId, cbBuffer, pbBuffer);
           if (pbBuffer[0])
           {
               /* we have a product name we can test with to see if we need a delay in this
                  msc being able to start */
               swprintf_s(pEnd,256,TEXT("\\%.*s"),pbBuffer[0]/sizeof(WCHAR),&pbBuffer[1]);
               DEBUGMSG( ZONE_USB_INIT, (TEXT("Checking startup delay for MSC \"%s\"\r\n"),productRegKey));

               if (GetSetKeyValues(productRegKey, rdValDesc, GET, FALSE))
               {
                   DEBUGMSG( ZONE_USB_INIT, (TEXT("Got product-specific delay of %d.\r\n"),dwDelayMs));
               }
               else
               {
                   DEBUGMSG( ZONE_USB_INIT, (TEXT("No product-specific delay found.\r\n")));
                   dwDelayMs = (DWORD)-1;
               }

               *pEnd = 0;
           }
        }

        if (dwDelayMs == (DWORD)-1)
        {
            /* get default delay from registry */
            if (GetSetKeyValues(productRegKey, rdValDesc, GET, FALSE))
            {
                DEBUGMSG( ZONE_USB_INIT, (TEXT("Got default MSC delay of %d.\r\n"),dwDelayMs));
            }
            else
            {
                DEBUGMSG( ZONE_USB_INIT, (TEXT("No default MSC delay found.\r\n")));
                dwDelayMs = 0;
            }
        }

        if (dwDelayMs)
        {
            if (dwDelayMs > MAX_MSC_START_DELAY)
                dwDelayMs = MAX_MSC_START_DELAY;
            DEBUGMSG( ZONE_USB_INIT, (TEXT("Doing MSC delay of %d ms\r\n"),dwDelayMs));
            Sleep(dwDelayMs);
        }
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
    for ( dwIndex = 0; dwIndex < dwNumInterfaces; pUsbInterface++, dwIndex++) 
    {
        DUMP_USB_INTERFACE_DESCRIPTOR( pUsbInterface->Descriptor, dwIndex );
        if ( pUsbInterface->Descriptor.bInterfaceNumber == CurInterface->Descriptor.bInterfaceNumber ) 
        {
            if (  pUsbInterface->Descriptor.bInterfaceClass == USBMSC_INTERFACE_CLASS &&
                 ((pUsbInterface->Descriptor.bInterfaceSubClass >= USBMSC_SUBCLASS_RBC && pUsbInterface->Descriptor.bInterfaceSubClass <= USBMSC_SUBCLASS_SCSI) || pUsbInterface->Descriptor.bInterfaceSubClass == USBMSC_SUBCLASS_RESERVED) &&
                 (pUsbInterface->Descriptor.bInterfaceProtocol == USBMSC_INTERFACE_PROTOCOL_CBIT || pUsbInterface->Descriptor.bInterfaceProtocol == USBMSC_INTERFACE_PROTOCOL_CBT || pUsbInterface->Descriptor.bInterfaceProtocol == USBMSC_INTERFACE_PROTOCOL_BOT) )
            {
                //
                // if we do not already have an interface, or the selected Protocol is not Bulk-Only
                // (I personally prefer Bulk-Only since it is well defined) 
                //
                if ( !pDesiredInterface || pDesiredInterface->Descriptor.bInterfaceProtocol != USBMSC_INTERFACE_PROTOCOL_BOT)
                {
                    pDesiredInterface = pUsbInterface;
                    DEBUGMSG( ZONE_USB_INIT, (TEXT("*** Found interface @ index: %d ***\n"), dwIndex));
                }
            }
        }
    }
  
   DEBUGMSG( ZONE_USB_INIT, (TEXT("USBMSC<ParseUsbDescriptors:0x%x\n"), pDesiredInterface ));

   return pDesiredInterface;
}


BOOL
SetUsbInterface(
   PUSBMSC_DEVICE   pUsbDevice,
   LPCWSTR          UniqueDriverId   
   )
{
   USB_TRANSFER hTransfer = 0;
   BOOL bRc = FALSE;
   DWORD dwIndex;

   DEBUGMSG( ZONE_USB_INIT, (TEXT("USBMSC>SetUsbInterface\n")));

   if ( !VALID_CONTEXT( pUsbDevice ) || !pUsbDevice->pUsbInterface ) {
      DEBUGMSG( ZONE_ERR, (TEXT("Invalid parameter\n")));
      return FALSE;
   }

   if (pUsbDevice->pUsbInterface->Descriptor.bAlternateSetting != 0) {
      hTransfer = pUsbDevice->UsbFuncs->lpSetInterface( pUsbDevice->hUsbDevice,
                                                        NULL,
                                                        NULL,
                                                        0, // Flags
                                                        pUsbDevice->pUsbInterface->Descriptor.bInterfaceNumber,
                                                        pUsbDevice->pUsbInterface->Descriptor.bAlternateSetting );

      if ( !hTransfer ) {
         DEBUGMSG( ZONE_ERR, (TEXT("SetUsbInterface error:%d\n"), GetLastError() ));
         return FALSE;
      }
   }

    //
    // now parse the endpoints
    //
    for ( dwIndex = 0; dwIndex < pUsbDevice->pUsbInterface->Descriptor.bNumEndpoints; dwIndex++) 
    {
        LPCUSB_ENDPOINT pEndpoint;
        pEndpoint = pUsbDevice->pUsbInterface->lpEndpoints + dwIndex;

        DUMP_USB_ENDPOINT_DESCRIPTOR( pEndpoint->Descriptor );

        // 
        // Mass Storage Class supports 1 mandatory Bulk OUT, 1 mandatory Bulk IN, and 1 optional INTERRUPT
        // 
        if ( USB_ENDPOINT_DIRECTION_OUT( pEndpoint->Descriptor.bEndpointAddress ) ) {
            if ( NULL == pUsbDevice->BulkOut.hPipe &&
                (pEndpoint->Descriptor.bmAttributes & USB_ENDPOINT_TYPE_MASK) == USB_ENDPOINT_TYPE_BULK) 
            {
                //
                // create the Bulk OUT pipe
                //
                pUsbDevice->BulkOut.hPipe = pUsbDevice->UsbFuncs->lpOpenPipe( pUsbDevice->hUsbDevice,
                                                                              &pEndpoint->Descriptor );
                if ( !pUsbDevice->BulkOut.hPipe ) {
                    DEBUGMSG( ZONE_ERR, (TEXT("OpenPipe error:%d\n"), GetLastError() ));
                    bRc = FALSE;
                    TEST_TRAP();
                    break;
                }
                //
                // Set Async Content
                pUsbDevice->BulkOut.pAsyncContent = CreateBulkTransferMgr(
                        pUsbDevice->UsbFuncs,pUsbDevice->BulkOut.hPipe,&pEndpoint->Descriptor,UniqueDriverId
                        );
                
                if (!pUsbDevice->BulkOut.pAsyncContent)  {
                    DEBUGMSG( ZONE_ERR, (TEXT("CreateBulkTransferMgr error:%d\n"), GetLastError() ));
                    bRc = FALSE;
                    TEST_TRAP();
                    break;
                }
                //
                // setup any endpoint specific timers, buffers, context, etc.
                //
                pUsbDevice->BulkOut.hEvent = CreateEvent( NULL, MANUAL_RESET_EVENT, FALSE, NULL);
                if ( !pUsbDevice->BulkOut.hEvent ) {
                    DEBUGMSG( ZONE_ERR, (TEXT("CreateEvent error:%d\n"), GetLastError() ));
                    bRc = FALSE;
                    TEST_TRAP();
                    break;
                }

                pUsbDevice->BulkOut.bIndex         = pEndpoint->Descriptor.bEndpointAddress;
                pUsbDevice->BulkOut.wMaxPacketSize = pEndpoint->Descriptor.wMaxPacketSize;
            }

        } else if (USB_ENDPOINT_DIRECTION_IN( pEndpoint->Descriptor.bEndpointAddress ) ) {
            if ( NULL == pUsbDevice->BulkIn.hPipe && 
                (pEndpoint->Descriptor.bmAttributes & USB_ENDPOINT_TYPE_MASK) == USB_ENDPOINT_TYPE_BULK) 
            {
                //
                // create the Bulk IN pipe
                //
                pUsbDevice->BulkIn.hPipe = pUsbDevice->UsbFuncs->lpOpenPipe( pUsbDevice->hUsbDevice,
                                                                            &pEndpoint->Descriptor );
                if ( !pUsbDevice->BulkIn.hPipe ) {
                    DEBUGMSG( ZONE_ERR, (TEXT("OpenPipe error: %d\n"), GetLastError() ));
                    bRc = FALSE;
                    TEST_TRAP();
                    break;
                }
                //
                // Set Async Content
                pUsbDevice->BulkIn.pAsyncContent = CreateBulkTransferMgr(
                        pUsbDevice->UsbFuncs,pUsbDevice->BulkIn.hPipe,&pEndpoint->Descriptor,UniqueDriverId
                        );
                if (!pUsbDevice->BulkIn.pAsyncContent)  {
                    DEBUGMSG( ZONE_ERR, (TEXT("CreateBulkTransferMgr error:%d\n"), GetLastError() ));
                    bRc = FALSE;
                    TEST_TRAP();
                    break;
                } ;

                //
                // setup any endpoint specific timers, buffers, context, etc.
                //
                pUsbDevice->BulkIn.hEvent = CreateEvent( NULL, MANUAL_RESET_EVENT, FALSE, NULL);
                if ( !pUsbDevice->BulkIn.hEvent ) {
                    DEBUGMSG( ZONE_ERR, (TEXT("CreateEvent error:%d\n"), GetLastError() ));
                    bRc = FALSE;
                    TEST_TRAP();
                    break;
                }

                pUsbDevice->BulkIn.bIndex         = pEndpoint->Descriptor.bEndpointAddress;
                pUsbDevice->BulkIn.wMaxPacketSize = pEndpoint->Descriptor.wMaxPacketSize;
            
            } else if ( NULL == pUsbDevice->Interrupt.hPipe &&
                (pEndpoint->Descriptor.bmAttributes & USB_ENDPOINT_TYPE_MASK) == USB_ENDPOINT_TYPE_INTERRUPT) 
            {
                //
                // create the Interrupt pipe
                //
                pUsbDevice->Interrupt.hPipe = pUsbDevice->UsbFuncs->lpOpenPipe( pUsbDevice->hUsbDevice,
                                                                                &pEndpoint->Descriptor );
               if ( !pUsbDevice->Interrupt.hPipe ) {
                    DEBUGMSG( ZONE_ERR, (TEXT("OpenPipe error: %d\n"), GetLastError() ));
                    bRc = FALSE;
                    TEST_TRAP();
                    break;
               }

               //
               // setup any endpoint specific timers, buffers, context, etc.
               //
               pUsbDevice->Interrupt.hEvent = CreateEvent( NULL, MANUAL_RESET_EVENT, FALSE, NULL);
               if ( !pUsbDevice->Interrupt.hEvent ) {
                    DEBUGMSG( ZONE_ERR, (TEXT("CreateEvent error:%d\n"), GetLastError() ));
                    bRc = FALSE;
                    TEST_TRAP();
                    break;
               }

               pUsbDevice->Interrupt.bIndex         = pEndpoint->Descriptor.bEndpointAddress;
               pUsbDevice->Interrupt.wMaxPacketSize = pEndpoint->Descriptor.wMaxPacketSize;
            }
      
        } else {
         DEBUGMSG( ZONE_WARN, (TEXT("Unsupported Endpoint:0x%x\n"), pEndpoint->Descriptor.bEndpointAddress));
         TEST_TRAP();
        }
    }

    //
    // did we find our endpoints?
    //
    bRc = (pUsbDevice->BulkOut.hPipe && pUsbDevice->BulkIn.hPipe) ? TRUE : FALSE;
    switch (pUsbDevice->pUsbInterface->Descriptor.bInterfaceProtocol) {
        case USBMSC_INTERFACE_PROTOCOL_CBIT:
            //
            // CBI Transport 3.4.3: *shall* support the interrupt endpoint
            //
            bRc &= (pUsbDevice->Interrupt.hPipe) ? TRUE : FALSE;
            ASSERTMSG((TEXT("CBI device missing Interrupt endpoint; fix your device!!\n")), bRc );
            break;

         case USBMSC_INTERFACE_PROTOCOL_CBT:
            break;

        case USBMSC_INTERFACE_PROTOCOL_BOT:
            break;

        default:
            bRc = FALSE;
            DEBUGMSG( ZONE_ERR, (TEXT("Unsupported device\n")));
            TEST_TRAP();
            break;
    }

    //
    // close the transfer handle.
    //
    if (hTransfer) {
       bRc = CloseTransferHandle( pUsbDevice->UsbFuncs, hTransfer );
       ASSERT(bRc);
    }

    //
    // if we failed to find all of our endpoints then cleanup will occur later
    //

    DEBUGMSG( ZONE_USB_INIT, (TEXT("USBMSC<SetUsbInterface:%d\n"), bRc));

    return (bRc);
}


VOID
ClosePipe(
    PUSBMSC_DEVICE pUsbDevice,
    PPIPE pPipe
    )
{
    if ( !pUsbDevice || !pUsbDevice->UsbFuncs || !pPipe) {
        DEBUGMSG(ZONE_ERR,(TEXT("Invalid Parameter!\n")));
        return;
    }

    if (pPipe->hPipe) {
        pUsbDevice->UsbFuncs->lpClosePipe(pPipe->hPipe);
    }

    if (pPipe->hEvent) {
        CloseHandle(pPipe->hEvent);
    }

    return;
}


VOID
RemoveDeviceContext(
   PUSBMSC_DEVICE pUsbDevice
   )
{
   DEBUGMSG(ZONE_INIT,(TEXT("USBMSC>RemoveDeviceContext(%p)\n"), pUsbDevice));

   if ( VALID_CONTEXT( pUsbDevice ) ) {

    if ( pUsbDevice->Flags.AcceptIo ) {
        DEBUGMSG(ZONE_ERR,(TEXT("RemoveDeviceContext on open device!!!\n")));
        //TEST_TRAP();
        return;
    }

    ClosePipe( pUsbDevice, &pUsbDevice->BulkOut );
    ClosePipe( pUsbDevice, &pUsbDevice->BulkIn );
    ClosePipe( pUsbDevice, &pUsbDevice->Interrupt );
    DeleteBulkTransferMgr(pUsbDevice->BulkOut.pAsyncContent);
    DeleteBulkTransferMgr(pUsbDevice->BulkIn.pAsyncContent);

    if (pUsbDevice->hEP0Event) {
        CloseHandle(pUsbDevice->hEP0Event);
    }
      
    if (&pUsbDevice->Lock) {
        DeleteCriticalSection( &pUsbDevice->Lock );
    }

    if (pUsbDevice->hDiskDriver) {
        FreeLibrary(pUsbDevice->hDiskDriver);
    }
      
    LocalFree(pUsbDevice);
   
   } else {
      DEBUGMSG(ZONE_ERR,(TEXT("Invalid Parameter\n")));
   }

   DEBUGMSG(ZONE_INIT,(TEXT("USBMSC<RemoveDeviceContext\n")));
   
   return;
}


//*****************************************************************************
//
//  U S B M S C      I N T E R F A C E
//
//*****************************************************************************


/*++

UsbsDataTransfer:
   Called by the USB Disk driver to place the block command on the USB.
   Used for reads, writes, commands, etc.

 hTransport:
   The Transport handle passed to DiskAttach.

 pCommand:
    Pointer to Command Block for this transaction.

 pData:
    Pointer to Data Block for this transaction. May be NULL.

 Returns:
   Win32 error code.

--*/
DWORD
UsbsDataTransfer(
    HANDLE             hTransport,
    PTRANSPORT_COMMAND pCommand,
    OPTIONAL PTRANSPORT_DATA pData
    )
{
    PUSBMSC_DEVICE pUsbDevice = (PUSBMSC_DEVICE)hTransport;
    DWORD dwErr = ERROR_SUCCESS;

    DEBUGMSG(ZONE_INIT,(TEXT("USBMSC>UsbsDataTransfer\n")));    

    if ( hTransport && pCommand )
    {
        if ( ACCEPT_IO(pUsbDevice) ) 
        {
            if (pData) {
                // preset the number of bytes transferred
                pData->TransferLength = 0; 
            }

            //
            // MUX the Transport Protocol
            //
            switch ( pUsbDevice->pUsbInterface->Descriptor.bInterfaceProtocol ) {

                case USBMSC_INTERFACE_PROTOCOL_CBIT:
                    DEBUGMSG(ZONE_INIT,(TEXT("USBMSC_INTERFACE_PROTOCOL_CBIT\n")));
                    dwErr = CBIT_DataTransfer( pUsbDevice, pCommand, pData, (pCommand->Flags & DATA_IN));
                    break;

                case USBMSC_INTERFACE_PROTOCOL_CBT:
                    DEBUGMSG(ZONE_INIT,(TEXT("USBMSC_INTERFACE_PROTOCOL_CBT\n")));
                    // TEST_TRAP(); // !! this is currently untested - need a device !!
                    dwErr = CBIT_DataTransfer( pUsbDevice, pCommand, pData, (pCommand->Flags & DATA_IN));
                    break;
        
                case USBMSC_INTERFACE_PROTOCOL_BOT:
                    DEBUGMSG(ZONE_INIT,(TEXT("USBMSC_INTERFACE_PROTOCOL_BOT\n")));
                    dwErr = BOT_DataTransfer( pUsbDevice, pCommand, pData, (pCommand->Flags & DATA_IN));
                    break;

                default:
                    DEBUGMSG(ZONE_ERR,(TEXT("ERROR_INVALID_DRIVE\n")));
                    dwErr = ERROR_INVALID_DRIVE;
                    SetLastError(dwErr);
                    break;
            }

        } else {
            dwErr = ERROR_ACCESS_DENIED;
            SetLastError( dwErr );
            DEBUGMSG(ZONE_ERR,(TEXT("UsbsDataTransfer error: %d\n"), dwErr));
        }

    } else {
        dwErr = ERROR_INVALID_PARAMETER;
        SetLastError( dwErr );
    }

    DEBUGMSG(ZONE_INIT,(TEXT("USBMSC<UsbsDataTransfer:%d\n"), dwErr));
    return dwErr;
}

DWORD GetMaxLUN(HANDLE hTransport, PUCHAR pLun)
{
    PUSBMSC_DEVICE pUsbDevice = (PUSBMSC_DEVICE)hTransport;
    DWORD dwErr = ERROR_INVALID_PARAMETER;

    DEBUGMSG(ZONE_INIT,(TEXT("USBMSC>GetMaxLUN\n")));    

    if ( hTransport  && pLun ) {
        if ( ACCEPT_IO(pUsbDevice) ) 
            switch ( pUsbDevice->pUsbInterface->Descriptor.bInterfaceProtocol ) {
                case USBMSC_INTERFACE_PROTOCOL_BOT:
                    DEBUGMSG(ZONE_INIT,(TEXT("USBMSC_INTERFACE_PROTOCOL_BOT\n")));
                    dwErr = BOT_GetMaxLUN( pUsbDevice,pLun );
                    break;
                default:
                    DEBUGMSG(ZONE_INIT,(TEXT("Default\n")));
                    *pLun = 1;
                    break;
            }
        DEBUGMSG(ZONE_INIT,(TEXT("USBMSC>GetMaxLUN return dwErr=%d,*pLun=%d\n"),dwErr,*pLun));    
    }
    return dwErr;
}
//*****************************************************************************
//
//      E X P O R T E D     U T I L S
//
//*****************************************************************************


//
// Returns the context pointer stored by Device Manager in the driver's Active path.
// The caller should check the pointer's validity.
//
PVOID
UsbsGetContextFromReg(
   LPCTSTR  ActivePath
   )
{
    BOOL bRc;
    PVOID pClientInfo = NULL;
    DWORD dwValue = 0;
    REG_VALUE_DESCR rdValDesc[] = {
        TEXT("ClientInfo"), REG_DWORD, sizeof(DWORD), NULL,
        NULL, 0, 0, NULL
    };
    rdValDesc[0].Data = (PUCHAR)(&dwValue);

    if ( ActivePath ) {

        bRc = GetSetKeyValues( ActivePath,
                                   rdValDesc,
                                   GET, 
                                   FALSE );

        // return the val
        if ( bRc ) {
            pClientInfo = (PVOID)dwValue;
        }

    } else {
        DEBUGMSG(ZONE_ERR, (TEXT("Invalid Parameter\n")));
    }

    return pClientInfo;
}


BOOL
UsbDeviceTest(
   PUSBMSC_DEVICE  pUsbDevice
   )
{
#if 1
    UNREFERENCED_PARAMETER(pUsbDevice);
#else
    DWORD dwErr;

    //
   // set stall on EP0
   //
   dwErr = ClearOrSetFeature( pUsbDevice->UsbFuncs,
                              pUsbDevice->hUsbDevice,
                              DefaultTransferComplete,
                              pUsbDevice->hEP0Event,
                              USB_SEND_TO_ENDPOINT,
                              USB_FEATURE_ENDPOINT_STALL,
                              0, // bIndex
                              1000,
                              TRUE );

   if ( ERROR_SUCCESS != dwErr ) {
      DEBUGMSG( ZONE_ERR, (TEXT("SetFeature error:%d\n"), dwErr ));
      TEST_TRAP();
   }

   //
   // clear EP0
   //
   ResetDefaultEndpoint(pUsbDevice->UsbFuncs, pUsbDevice->hUsbDevice);

   //
   // set stall on the BulkOut endpoint
   //
   dwErr = ClearOrSetFeature( pUsbDevice->UsbFuncs,
                              pUsbDevice->hUsbDevice,
                              DefaultTransferComplete,
                              pUsbDevice->hEP0Event,
                              USB_SEND_TO_ENDPOINT,
                              USB_FEATURE_ENDPOINT_STALL,
                              pUsbDevice->BulkOut.bIndex,
                              1000,
                              TRUE );
   
   if ( ERROR_SUCCESS != dwErr ) {
      DEBUGMSG( ZONE_ERR, (TEXT("SetFeature error:%d\n"), dwErr));
      TEST_TRAP();
   }
   
   //
   // clear it
   //
   dwErr = ClearOrSetFeature( pUsbDevice->UsbFuncs,
                              pUsbDevice->hUsbDevice,
                              DefaultTransferComplete,
                              pUsbDevice->hEP0Event,
                              USB_SEND_TO_ENDPOINT,
                              USB_FEATURE_ENDPOINT_STALL,
                              pUsbDevice->BulkOut.bIndex,
                              1000,
                              FALSE );
   
   if ( ERROR_SUCCESS != dwErr ) {
      DEBUGMSG( ZONE_ERR, (TEXT("ClearFeature error:%d\n"), dwErr));
      TEST_TRAP();
   }


   //
   // set stall on the BulkIn endpoint
   //
   dwErr = ClearOrSetFeature( pUsbDevice->UsbFuncs,
                              pUsbDevice->hUsbDevice,
                              DefaultTransferComplete,
                              pUsbDevice->hEP0Event,
                              USB_SEND_TO_ENDPOINT,
                              USB_FEATURE_ENDPOINT_STALL,
                              pUsbDevice->BulkIn.bIndex,
                              1000,
                              TRUE );

   if ( ERROR_SUCCESS != dwErr ) {
      DEBUGMSG( ZONE_ERR, (TEXT("SetFeature error:%d\n"), dwErr));
      TEST_TRAP();
   }

   //
   // clear it
   //
   dwErr = ClearOrSetFeature( pUsbDevice->UsbFuncs,
                              pUsbDevice->hUsbDevice,
                              DefaultTransferComplete,
                              pUsbDevice->hEP0Event,
                              USB_SEND_TO_ENDPOINT,
                              USB_FEATURE_ENDPOINT_STALL,
                              pUsbDevice->BulkIn.bIndex,
                              1000,
                              FALSE );

   if ( ERROR_SUCCESS != dwErr ) {
      DEBUGMSG( ZONE_ERR, (TEXT("ClearFeature error:%d\n"), dwErr ));
      TEST_TRAP();
   }

#endif // 0

   return TRUE;
}

// EOF
