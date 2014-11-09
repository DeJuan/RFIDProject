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
    usbprn.h

Abstract:
    USB Client Driver for Print Device Class.

Functions:

Notes:

--*/

#if !defined( _USBPRN_ )
#define _USBPRN_

#include <windows.h>
#include <devload.h>
#include <usbdi.h>
#include <pegdser.h>
#include <pegdpar.h>

#include <ntcompat.h>

#include "usbclient.h"


#define DRIVER_NAME   TEXT("USBPRN.DLL")
#define DEVICE_PREFIX TEXT("LPT")

#define CLASS_NAME_SZ    TEXT("Printer_Class")
#define CLIENT_REGKEY_SZ TEXT("Drivers\\USB\\ClientDrivers\\Printer_Class")

#define MAX_LPT_NAME_INDEX  10
#define PORTNAME_LEN        32

//
// USB Printer Interface Descriptor
//
#define USB_PRN_INTERFACE_CLASS        0x07
#define USB_PRN_INTERFACE_SUBCLASS     0x01
#define USB_PRN_INTERFACE_PROTOCOL_UNI 0x01
#define USB_PRN_INTERFACE_PROTOCOL_BI  0x02

//
// Default Timeout values
//
#define GET_PORT_STATUS_TIMEOUT  2000
#define GET_DEVICE_ID_TIMEOUT    2000
#define SOFT_RESET_TIMEOUT       2000

#define READ_TIMEOUT_INTERVAL    250
#define READ_TIMEOUT_MULTIPLIER  10
#define READ_TIMEOUT_CONSTANT       100
#define WRITE_TIMEOUT_MULTIPLIER    50
#define WRITE_TIMEOUT_CONSTANT  1000

//
// registry strings
//
#define GET_PORT_STATUS_TIMEOUT_SZ  TEXT("PortStatusTimeout")
#define GET_DEVICE_ID_TIMEOUT_SZ    TEXT("DeviceIdTimeout")
#define SOFT_RESET_TIMEOUT_SZ       TEXT("SoftResetTimeout")

#define READ_TIMEOUT_MULTIPLIER_SZ  TEXT("ReadTimeoutMultiplier")
#define READ_TIMEOUT_CONSTANT_SZ    TEXT("ReadTimeoutConstant")
#define WRITE_TIMEOUT_MULTIPLIER_SZ TEXT("WriteTimeoutMultiplier")
#define WRITE_TIMEOUT_CONSTANT_SZ   TEXT("WriteTimeoutConstant")



// defined in device class spec
#define USBPRN_STATUS_SELECT      0x10
#define USBPRN_STATUS_NOTERROR    0x08
#define USBPRN_STATUS_PAPEREMPTY  0x20
// additional err
#define USBPRN_STATUS_TIMEOUT     0x100
#define USBPRN_STATUS_BUSY        0x200

#define USB_PRN_SIG 0x50425355 // "USBP" tag

#define MANUAL_RESET_EVENT TRUE
#define AUTO_RESET_EVENT   FALSE

#pragma warning(push)
#pragma warning(disable:4214)
typedef struct _FLAGS {
   UCHAR    Open           : 1; // bits 0
   UCHAR    UnloadPending  : 1; // bits 1
   UCHAR    Reserved       : 6; // bits 2-7
} FLAGS, *PFLAGS;
#pragma warning(pop)


typedef struct _USBTIMEOUTS {

    DWORD PortStatusTimeout;

    DWORD DeviceIdTimeout;

    DWORD SoftResetTimeout;

} USBTIMEOUTS, *PUSBTIMEOUTS;


//
// Our notion of a Pipe
//
typedef struct _PIPE {
    //
    // USB Pipe handle received from the stack
    //
    USB_PIPE hPipe;

    //
    // Endpoint's Address
    //
    UCHAR    bIndex;

    //
    // Endpoint's wMaxPacketSize
    //
    USHORT wMaxPacketSize;

    //
    // Completion Event
    //
    HANDLE   hEvent;

} PIPE, *PPIPE;


typedef struct _USBPRN_CONTEXT {
   //
   // We use a Signature (defined above) since we must read
   // our context pointer out of the registry
   //
   ULONG Sig;

   //
   // sync object for this instance
   //
   CRITICAL_SECTION Lock;

   //
   // path to HKLM\Drivers\Active\xx
   //
   LPTSTR  ActivePath;

   //
   // Name under HKLM\Printers\Ports\PortX
   //
   TCHAR   PortName[PORTNAME_LEN];

   //
   // Handle for Stream interface.
   //
   HANDLE   hStreamDevice;

   //
   // USB handle to the device
   //
    HANDLE   hUsbDevice;

   //
   // USBD Function table
   //
   LPCUSB_FUNCS   UsbFuncs;

   //
   // Fields from USB_INTERFACE that we need
   //
   UCHAR bInterfaceNumber;
   UCHAR bAlternateSetting;

   USHORT ConfigIndex;

   //
   // Bulk OUT Pipe
   //
   PIPE BulkOut;

   //
   // Bulk IN Pipe
   //
   PIPE BulkIn;

    //
    // completion signal used for endpoint 0
    //
    HANDLE hEP0Event;


   //
   // Signals the device has been closed.
   //
   HANDLE hCloseEvent;

   //
   // FLAGS
   //
   FLAGS Flags;

   //
   // Comm Timeouts
   //
   COMMTIMEOUTS Timeouts;

   //
   // USB Timeouts
   //
   USBTIMEOUTS UsbTimeouts;

} USBPRN_CONTEXT, *PUSBPRN_CONTEXT;


//
// Is the pointer a valid PUSBPRN_CONTEXT
//
#define VALID_CONTEXT( p ) \
   ( p && USB_PRN_SIG == p->Sig )


//
// Can the device accept any I/O requests
//
#define ACCEPT_IO( p ) \
   ( VALID_CONTEXT( p ) && \
     p->Flags.Open && \
    !p->Flags.UnloadPending )


//
// USB_DRIVER_SETTINGS
//
#define DRIVER_SETTINGS \
            sizeof(USB_DRIVER_SETTINGS),  \
            USB_NO_INFO,   \
            USB_NO_INFO,   \
            USB_NO_INFO,   \
            USB_NO_INFO,   \
            USB_NO_INFO,   \
            USB_NO_INFO,   \
            USB_PRN_INTERFACE_CLASS,      \
            USB_PRN_INTERFACE_SUBCLASS,   \
            USB_NO_INFO


//
// USBPRN.C
//
LPCUSB_INTERFACE
ParseUsbDescriptors(
   USB_HANDLE       hUsbDevice,
   LPCUSB_FUNCS     UsbFuncs,
   LPCUSB_INTERFACE CurInterface,
   LPUSHORT         ConfigIndex
   );

BOOL
SetUsbInterface(
   PUSBPRN_CONTEXT  pUsbPrn,
   LPCUSB_INTERFACE UsbInterface
   );

DWORD
GetDeviceId(
   PUSBPRN_CONTEXT pUsbPrn,
   __out_bcount(dwLen) PBYTE           pBuf,
   DWORD           dwLen
   );

USHORT
GetPortStatus(
   PUSBPRN_CONTEXT pUsbPrn
   );

BOOL
SoftReset(
   PUSBPRN_CONTEXT pUsbPrn
   );

DWORD
IoErrorHandler(
   PUSBPRN_CONTEXT pUsbPrn,
   USB_PIPE        hPipe,
   UCHAR           bIndex,
   USB_ERROR       dwUsbError
   );


//
// DEBUG
//
#if DEBUG

#define ZONE_ERR            DEBUGZONE(0)
#define ZONE_WARN           DEBUGZONE(1)
#define ZONE_INIT           DEBUGZONE(2)
#define ZONE_TRACE          DEBUGZONE(3)

#define ZONE_LPT_INIT       DEBUGZONE(4)
#define ZONE_LPT_READ       DEBUGZONE(5)
#define ZONE_LPT_WRITE      DEBUGZONE(6)
#define ZONE_LPT_IOCTL      DEBUGZONE(7)

#define ZONE_USB_PARSE      DEBUGZONE(8)
#define ZONE_USB_INIT       DEBUGZONE(9)
#define ZONE_USB_CONTROL    DEBUGZONE(10)
#define ZONE_USB_BULK       DEBUGZONE(11)

#define ZONE_USBCLIENT      DEBUGZONE(15)

//
// these should be removed in the code if you can 'g' past these successfully
//
#define TEST_TRAP() { \
   NKDbgPrintfW( TEXT("%s: Code Coverage Trap in: %s, Line: %d\n"), DRIVER_NAME, TEXT(__FILE__), __LINE__); \
   DebugBreak();  \
}

#define DUMP_USB_DEVICE_DESCRIPTOR( d ) { \
   DEBUGMSG( ZONE_USB_PARSE, (TEXT("USB_DEVICE_DESCRIPTOR:\n"))); \
   DEBUGMSG( ZONE_USB_PARSE, (TEXT("----------------------\n"))); \
   DEBUGMSG( ZONE_USB_PARSE, (TEXT("bLength: 0x%x\n"), d.bLength ));   \
   DEBUGMSG( ZONE_USB_PARSE, (TEXT("bDescriptorType: 0x%x\n"), d.bDescriptorType ));  \
   DEBUGMSG( ZONE_USB_PARSE, (TEXT("bcdUSB: 0x%x\n"), d.bcdUSB ));  \
   DEBUGMSG( ZONE_USB_PARSE, (TEXT("bDeviceClass: 0x%x\n"), d.bDeviceClass ));  \
   DEBUGMSG( ZONE_USB_PARSE, (TEXT("bDeviceSubClass: 0x%x\n"), d.bDeviceSubClass ));  \
   DEBUGMSG( ZONE_USB_PARSE, (TEXT("bDeviceProtocol: 0x%x\n"), d.bDeviceProtocol ));  \
   DEBUGMSG( ZONE_USB_PARSE, (TEXT("bMaxPacketSize0: 0x%x\n"), d.bMaxPacketSize0 ));  \
   DEBUGMSG( ZONE_USB_PARSE, (TEXT("idVendor: 0x%x\n"), d.idVendor )); \
   DEBUGMSG( ZONE_USB_PARSE, (TEXT("idProduct: 0x%x\n"), d.idProduct ));  \
   DEBUGMSG( ZONE_USB_PARSE, (TEXT("bcdDevice: 0x%x\n"), d.bcdDevice ));  \
   DEBUGMSG( ZONE_USB_PARSE, (TEXT("iManufacturer: 0x%x\n"), d.iManufacturer ));   \
   DEBUGMSG( ZONE_USB_PARSE, (TEXT("iProduct: 0x%x\n"), d.iProduct )); \
   DEBUGMSG( ZONE_USB_PARSE, (TEXT("iSerialNumber: 0x%x\n"), d.iSerialNumber ));   \
   DEBUGMSG( ZONE_USB_PARSE, (TEXT("bNumConfigurations: 0x%x\n"), d.bNumConfigurations ));  \
   DEBUGMSG( ZONE_USB_PARSE, (TEXT("\n")));  \
}

#define DUMP_USB_CONFIGURATION_DESCRIPTOR( c ) { \
   DEBUGMSG( ZONE_USB_PARSE, (TEXT("USB_CONFIGURATION_DESCRIPTOR:\n"))); \
   DEBUGMSG( ZONE_USB_PARSE, (TEXT("-----------------------------\n"))); \
   DEBUGMSG( ZONE_USB_PARSE, (TEXT("bLength: 0x%x\n"), c.bLength )); \
   DEBUGMSG( ZONE_USB_PARSE, (TEXT("bDescriptorType: 0x%x\n"), c.bDescriptorType )); \
   DEBUGMSG( ZONE_USB_PARSE, (TEXT("wTotalLength: 0x%x\n"), c.wTotalLength )); \
   DEBUGMSG( ZONE_USB_PARSE, (TEXT("bNumInterfaces: 0x%x\n"), c.bNumInterfaces )); \
   DEBUGMSG( ZONE_USB_PARSE, (TEXT("bConfigurationValue: 0x%x\n"), c.bConfigurationValue )); \
   DEBUGMSG( ZONE_USB_PARSE, (TEXT("iConfiguration: 0x%x\n"), c.iConfiguration )); \
   DEBUGMSG( ZONE_USB_PARSE, (TEXT("bmAttributes: 0x%x\n"), c.bmAttributes )); \
   DEBUGMSG( ZONE_USB_PARSE, (TEXT("MaxPower: 0x%x\n"), c.MaxPower )); \
   DEBUGMSG( ZONE_USB_PARSE, (TEXT("\n"))); \
}

#define DUMP_USB_INTERFACE_DESCRIPTOR( i, _index ) { \
   DEBUGMSG( ZONE_USB_PARSE, (TEXT("USB_INTERFACE_DESCRIPTOR[%d]:\n"), _index )); \
   DEBUGMSG( ZONE_USB_PARSE, (TEXT("-------------------------\n"))); \
   DEBUGMSG( ZONE_USB_PARSE, (TEXT("bLength: 0x%x\n"), i.bLength )); \
   DEBUGMSG( ZONE_USB_PARSE, (TEXT("bDescriptorType: 0x%x\n"), i.bDescriptorType )); \
   DEBUGMSG( ZONE_USB_PARSE, (TEXT("bInterfaceNumber: 0x%x\n"), i.bInterfaceNumber )); \
   DEBUGMSG( ZONE_USB_PARSE, (TEXT("bAlternateSetting: 0x%x\n"), i.bAlternateSetting )); \
   DEBUGMSG( ZONE_USB_PARSE, (TEXT("bNumEndpoints: 0x%x\n"), i.bNumEndpoints )); \
   DEBUGMSG( ZONE_USB_PARSE, (TEXT("bInterfaceClass: 0x%x\n"), i.bInterfaceClass )); \
   DEBUGMSG( ZONE_USB_PARSE, (TEXT("bInterfaceSubClass: 0x%x\n"), i.bInterfaceSubClass )); \
   DEBUGMSG( ZONE_USB_PARSE, (TEXT("bInterfaceProtocol: 0x%x\n"), i.bInterfaceProtocol )); \
   DEBUGMSG( ZONE_USB_PARSE, (TEXT("iInterface: 0x%x\n"), i.iInterface )); \
   DEBUGMSG( ZONE_USB_PARSE, (TEXT("\n"))); \
}

#define DUMP_USB_ENDPOINT_DESCRIPTOR( e ) { \
   DEBUGMSG( ZONE_USB_PARSE, (TEXT("USB_ENDPOINT_DESCRIPTOR:\n"))); \
   DEBUGMSG( ZONE_USB_PARSE, (TEXT("-----------------------------\n"))); \
   DEBUGMSG( ZONE_USB_PARSE, (TEXT("bLength: 0x%x\n"), e.bLength )); \
   DEBUGMSG( ZONE_USB_PARSE, (TEXT("bDescriptorType: 0x%x\n"), e.bDescriptorType )); \
   DEBUGMSG( ZONE_USB_PARSE, (TEXT("bEndpointAddress: 0x%x\n"), e.bEndpointAddress )); \
   DEBUGMSG( ZONE_USB_PARSE, (TEXT("bmAttributes: 0x%x\n"), e.bmAttributes )); \
   DEBUGMSG( ZONE_USB_PARSE, (TEXT("wMaxPacketSize: 0x%x\n"), e.wMaxPacketSize )); \
   DEBUGMSG( ZONE_USB_PARSE, (TEXT("bInterval: 0x%x\n"), e.bInterval ));\
   DEBUGMSG( ZONE_USB_PARSE, (TEXT("\n"))); \
}

#else
#define TEST_TRAP()
#define DUMP_USB_DEVICE_DESCRIPTOR( p )
#define DUMP_USB_CONFIGURATION_DESCRIPTOR( c )
#define DUMP_USB_INTERFACE_DESCRIPTOR( i, _index )
#define DUMP_USB_ENDPOINT_DESCRIPTOR( e )
#endif // DEBUG


#endif // _USBPRN_

// EOF