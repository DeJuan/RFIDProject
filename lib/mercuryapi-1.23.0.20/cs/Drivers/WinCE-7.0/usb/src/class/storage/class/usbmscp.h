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

    usbmscp.h (private)

Abstract:

    USB Mass Storage Class
        Bulk-Only Transport 1.0
        Control/Bulk/Interrupt (CBI) Transport 1.0

Functions:

Notes: 

--*/

#if !defined( _USBMSCP_ )
#define _USBMSCP_

//*****************************************************************************
// I N C L U D E S
//*****************************************************************************

#include <windows.h>
#include <usbdi.h>

#include "usbmsc.h"
#include "usbclient.h"

//*****************************************************************************
// D E F I N E S
//*****************************************************************************

#define DRIVER_NAME_SZ   TEXT("USBMSC.DLL")
#define CLASS_NAME_SZ    TEXT("Mass_Storage_Class")
#define CLIENT_REGKEY_SZ TEXT("Drivers\\USB\\ClientDrivers\\Mass_Storage_Class")

#define USBMSC_SIG 'UMSC'

#define MAX_USB_BULK_LENGTH 0x2000L // 8k

// default timeout values
#define RESET_TIMEOUT           4000
#define COMMAND_BLOCK_TIMEOUT   4000
#define COMMAND_STATUS_TIMEOUT  4000

#define RESET_TIMEOUT_SZ            TEXT("ResetTimeout")
#define COMMAND_BLOCK_TIMEOUT_SZ    TEXT("CommandBlockTimeout")
#define COMMAND_STATUS_TIMEOUT_SZ   TEXT("CommandStatusTimeout")

// TBD: may want this in the registry
#define MAX_INT_RETRIES 3


//
// Is the context pointer valid
//
#define VALID_CONTEXT( p ) \
   ( (p != NULL) && USBMSC_SIG == p->Sig )

//
// Can the device accept any I/O requests
//
#define ACCEPT_IO( p ) \
   ( VALID_CONTEXT( p ) && \
     p->Flags.AcceptIo && \
    !p->Flags.DeviceRemoved )

//
// global USB_DRIVER_SETTINGS
//
#define USBMSC_DRIVER_SETTINGS \
            sizeof(USB_DRIVER_SETTINGS),  \
            USB_NO_INFO,   \
            USB_NO_INFO,   \
            USB_NO_INFO,   \
            USB_NO_INFO,   \
            USB_NO_INFO,   \
            USB_NO_INFO,   \
            USBMSC_INTERFACE_CLASS, \
            USB_NO_INFO,    \
            USB_NO_INFO


#ifdef GET_DT

#define INIT_DT  DWORD _dwStartTime=0, _dwStopTime=0, _dt;

#define START_DT _dwStartTime = GetTickCount();

#define STOP_DT(_uStr, _WarnThreshold, _HaltThreshold) \
     _dwStopTime = GetTickCount(); \
     _dt = _dwStopTime - _dwStartTime; \
     if (_dt > _WarnThreshold ) { \
        DEBUGMSG( ZONE_TIME, (TEXT("%s dT:%d\n"), _uStr, _dt )); \
        if (_dt > _HaltThreshold ) { \
            ASSERT(0); \
        } \
     }

#else

#define INIT_DT
#define START_DT
#define STOP_DT(x,y,z)

#endif // GET_DT



//*****************************************************************************
// T Y P E D E F S
//*****************************************************************************
extern LONG g_NumDevices;

#pragma warning(push)
#pragma warning(disable:4214) // nonstandard extension used : bit field types other than int
//
// Device Flags
//
typedef struct _DEVICE_FLAGS {
    //
    // The device is initialized and ready to accept I/O
    //
    UCHAR   AcceptIo : 1;

    //
    // The device has been removed
    //
    UCHAR   DeviceRemoved : 1;

    //
    // The device claims CBIT protocol,
    // but does not use the Interrupt pipe
    //
    UCHAR   IgnoreInterrupt : 1;

} DEVICE_FLAGS, *PDEVICE_FLAGS;
#pragma warning(pop)

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

    //
    // Async Content
    PVOID   pAsyncContent;    

} PIPE, *PPIPE;


//
// Reg configurable Timeouts
//
typedef struct _TIMEOUTS {

    ULONG Reset;

    ULONG CommandBlock;

    ULONG CommandStatus;

} TIMEOUTS, *PTIMEOUTS;


typedef struct _USBMSC_DEVICE {
    //
    // device context Signature
    //
    ULONG Sig;

    //
    // global sync object for this instance
    //
    CRITICAL_SECTION Lock;

    //
    // Handle to the Disk driver instance
    //
    HINSTANCE hDiskDriver;

    //
    // USB handle to our device
    //
    HANDLE   hUsbDevice;
    ULONG    Index;


    //
    // USBD Function table
    //
    LPCUSB_FUNCS   UsbFuncs;

    //
    // Fields from USB_INTERFACE that we need
    //
    LPCUSB_INTERFACE pUsbInterface;

    //
    // USB Configutation Index
    //
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
    // Interrupt Pipe
    //
    PIPE  Interrupt;
    UCHAR InterruptErrors;

    //
    // completion signal used for endpoint 0
    //
    HANDLE hEP0Event;

    //
    // Disk driver's Detach callback & context
    //
    PUSBDISK_DETACH DiskDetach;
    // Multi-Lun Support.
    DWORD dwMaxLun;
    PVOID DiskContext[MAX_LUN];
    //
    // FLAGS
    //
    DEVICE_FLAGS Flags;

    TIMEOUTS Timeouts;

    DWORD dwCurTag;

} USBMSC_DEVICE, *PUSBMSC_DEVICE;


//*****************************************************************************
//
// D E B U G
//
//*****************************************************************************
#if DEBUG
#define ZONEID_ERR          0
#define ZONEID_WARN         1
#define ZONEID_INIT         2
#define ZONEID_TRACE        3
#define ZONEID_USB_PARSE    4
#define ZONEID_USB_INIT     5
#define ZONEID_USB_CONTROL  6
#define ZONEID_USB_BULK     7
#define ZONEID_USB_INT      8
#define ZONEID_BOT          9
#define ZONEID_CBI          10
#define ZONEID_CBIT         11
#define ZONEID_TIME         12

#define ZONE_ERR            DEBUGZONE(ZONEID_ERR)
#define ZONE_WARN           DEBUGZONE(ZONEID_WARN)
#define ZONE_INIT           DEBUGZONE(ZONEID_INIT)
#define ZONE_TRACE          DEBUGZONE(ZONEID_TRACE)

#define ZONE_USB_PARSE      DEBUGZONE(ZONEID_USB_PARSE)
#define ZONE_USB_INIT       DEBUGZONE(ZONEID_USB_INIT)
#define ZONE_USB_CONTROL    DEBUGZONE(ZONEID_USB_CONTROL)
#define ZONE_USB_BULK       DEBUGZONE(ZONEID_USB_BULK)

#define ZONE_USB_INT        DEBUGZONE(ZONEID_USB_INT)
#define ZONE_BOT            DEBUGZONE(ZONEID_BOT)
#define ZONE_CBI            DEBUGZONE(ZONEID_CBI)
#define ZONE_CBIT           DEBUGZONE(ZONEID_CBIT)

#define ZONE_TIME           DEBUGZONE(ZONEID_TIME)

#define ZONEMASK_ERR            ( 1 << ZONEID_ERR )
#define ZONEMASK_WARN           ( 1 << ZONEID_WARN )
#define ZONEMASK_INIT           ( 1 << ZONEID_INIT )
#define ZONEMASK_TRACE          ( 1 << ZONEID_TRACE )
#define ZONEMASK_USB_PARSE      ( 1 << ZONEID_USB_PARSE )
#define ZONEMASK_USB_INIT       ( 1 << ZONEID_USB_INIT )
#define ZONEMASK_USB_CONTROL    ( 1 << ZONEID_USB_CONTROL )
#define ZONEMASK_USB_BULK       ( 1 << ZONEID_USB_BULK )
#define ZONEMASK_USB_INT        ( 1 << ZONEID_USB_INT )
#define ZONEMASK_BOT            ( 1 << ZONEID_BOT )
#define ZONEMASK_CBI            ( 1 << ZONEID_CBI )
#define ZONEMASK_CBIT           ( 1 << ZONEID_CBIT )
#define ZONEMASK_TIME           ( 1 << ZONEID_TIME )


//
// these should be removed in the code if you can 'g' past these successfully
//
#define TEST_TRAP() { \
   NKDbgPrintfW( TEXT("%s: Code Coverage Trap in: %s, Line: %d\n"), DRIVER_NAME_SZ, TEXT(__FILE__), __LINE__); \
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


#endif // _USBMSCP_

// EOF
