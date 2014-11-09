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
    usbhid.h

Abstract:
    USB Client Driver for Human Interface Device (HID) Class.

Functions:

Notes:

--*/

#ifndef _USBHID_H_
#define _USBHID_H_

#include <windows.h>
#include <usbdi.h>
#include <hid.h>

#include "usbclient.h"


#define UnusedParameter(x) UNREFERENCED_PARAMETER(x)
#pragma deprecated("UnusedParameter")

#define DRIVER_NAME             _T("USBHID.DLL")
#define DEVICE_PREFIX           _T("HID")

#define CLASS_NAME_SZ           _T("Hid_Class")
#define CLIENT_REGKEY_SZ        _T("Drivers\\USB\\ClientDrivers\\Hid\\") CLASS_NAME_SZ
#define HID_REGKEY_SZ           _T("Drivers\\USB\\ClientDrivers\\Hid\\Instance")

#define QUEUED_TRANSFERS_SZ         _T("QueuedTransferCount")
#define MIN_QUEUED_TRANSFERS        1
#define MAX_QUEUED_TRANSFERS        MAXIMUM_WAIT_OBJECTS
#define DEFAULT_QUEUED_TRANSFERS    2


#define USB_HID_SIG 'HBSU' // "USBH" tag

#define MANUAL_RESET_EVENT TRUE
#define AUTO_RESET_EVENT   FALSE


// Class-specific definitions (from the HID specification).
// Must use IssueVendorTransfer to make these requests.
#define USB_REQUEST_HID_GET_REPORT      0x01
#define USB_REQUEST_HID_GET_IDLE        0x02
#define USB_REQUEST_HID_GET_PROTOCOL    0x03
#define USB_REQUEST_HID_SET_REPORT      0x09
#define USB_REQUEST_HID_SET_IDLE        0x0A
#define USB_REQUEST_HID_SET_PROTOCOL    0x0B

#define HID_DESCRIPTOR_TYPE             0x21
#define HID_REPORT_DESCRIPTOR_TYPE      0x22

#define HID_INTERFACE_NO_SUBCLASS       0x00
#define HID_INTERFACE_BOOT_SUBCLASS     0x01

#define HID_BOOT_PROTOCOL               0x00
#define HID_REPORT_PROTOCOL             0x01


#pragma pack (1)
typedef struct _HID_DESCRIPTOR
{
    UCHAR   bLength;
    UCHAR   bDescriptorType;
    USHORT  bcdHID;
    UCHAR   bCountryCode;
    UCHAR   bNumDescriptors;
    UCHAR   bClassDescriptorType;
    USHORT  wDescriptorLength;
} HID_DESCRIPTOR, *PHID_DESCRIPTOR;
#pragma pack ()


typedef struct _FLAGS {
    UCHAR    Open           : 1; // bits 0
    UCHAR    UnloadPending  : 1; // bits 1
    UCHAR    MddInitialized : 1; // bits 2
    UCHAR    RemoteWakeup   : 1; // bits 3
    UCHAR    Reserved       : 5; // bits 4-7
} FLAGS, *PFLAGS;


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
    UCHAR bIndex;

    //
    // Endpoint's wMaxPacketSize
    //
    USHORT wMaxPacketSize;

    //
    // Completion Event
    //
    HANDLE hEvent;

} PIPE, *PPIPE;


typedef struct _USBHID_CONTEXT {
    //
    // We use a Signature (defined above) for validation
    //
    ULONG Sig;

    //
    // sync object for this instance
    //
    CRITICAL_SECTION csLock;

#ifdef DEBUG
    // The current depth of csLock
    INT cInCS;
#endif

    //
    // USB handle to the device
    //
    USB_HANDLE hUsbDevice;

    //
    // The device handle for this instance's unnamed stream interface
    //
    HANDLE hHidDriver;

    //
    // USBD Function table
    //
    LPCUSB_FUNCS pUsbFuncs;

    //
    // Fields from USB_INTERFACE that we need
    //
    LPCUSB_INTERFACE pUsbInterface;

    //
    // Interrupt In Pipe
    //
    PIPE InterruptIn;

    //
    // completion signal used for endpoint 0 (control)
    //
    HANDLE hEP0Event;

    //
    // Signals the device has been closed.
    //
    HANDLE hThread;
#ifdef DEBUG
    BOOL fhThreadInited;
#endif

    //
    // FLAGS
    //
    FLAGS Flags;

    // Regarding fSendToInterface.  The original HID spec said that the Hid
    // descriptor would come after the interface and endpoint descriptors.
    // It also said that class specific commands should be sent to the endpoint.
    // The next spec said that the HID descriptor would come after the interface
    // descriptor (not at the end) and that commands should be sent to the
    // interface, not to the endpoint.  So, I'm assuming that if I find the
    // Hid descriptor after the interface, the device is following the new spec
    // and I should send commands to the interface.  Otherwise, I'll send them
    // to the endpoint, as stated in the old spec.
    BOOL fSendToInterface;

    //
    // Parameter provided by the MDD.
    //
    PVOID pvNotifyParameter;

    //
    // The length of the largest interrupt input report that we can receive.
    //
    DWORD cbMaxReport;

    //
    // Keep a reference of USBHID.dll
    //
    HMODULE hModSelf;
    
    //
    // Reference count of the USBHID_CONTEXT
    //
    LONG dwRefCnt;

} USBHID_CONTEXT, *PUSBHID_CONTEXT;


//
// Is the pointer a valid PUSBHID_CONTEXT
//
#define VALID_CONTEXT( p ) \
   ( p && (USB_HID_SIG == p->Sig) )


// The size in bytes of the global array of devices
#define USB_HID_ARRAY_BYTE_SIZE(count) (sizeof(PUSBHID_CONTEXT) * count)


//
// USB_DRIVER_SETTINGS
//
// As specified in the HID spec, HID class devices are identified in 
// the interface descriptor, so we only need to specify the 
// dwInterfaceClass field.
//
#define DRIVER_SETTINGS \
            sizeof(USB_DRIVER_SETTINGS),  \
            USB_NO_INFO,   \
            USB_NO_INFO,   \
            USB_NO_INFO,   \
            USB_NO_INFO,   \
            USB_NO_INFO,   \
            USB_NO_INFO,   \
            USB_DEVICE_CLASS_HUMAN_INTERFACE,      \
            USB_NO_INFO,   \
            USB_NO_INFO


// Hid functions
LPCUSB_INTERFACE
ParseUsbDescriptors(
    LPCUSB_INTERFACE pCurrInterface
    );

PUSBHID_CONTEXT
CreateUsbHidDevice(
    USB_HANDLE hUsbDevice,
    PCUSB_FUNCS pUsbFuncs,
    PCUSB_INTERFACE pUsbInterface
    );

void
RemoveDeviceContext(
   PUSBHID_CONTEXT pUsbHid
   );

BOOL
CreateInterruptThread(
    PUSBHID_CONTEXT pUsbHid
    );

void
DetermineDestination(
    PUSBHID_CONTEXT pUsbHid,
    BYTE *pbmRequestType,
    USHORT *pwIndex
    );


#ifdef DEBUG


// Pre-defined usbclient.lib debug zone.
#define ZONE_USB_CLIENT             DEBUGZONE(15)


//
// ***** Debug utility functions *****
// 

BOOL
IsUsbHidContextLocked(
    const USBHID_CONTEXT *pUsbHid
    );

void
LockUsbHidContext(
    USBHID_CONTEXT *pUsbHid
    );

void
UnlockUsbHidContext(
    USBHID_CONTEXT *pUsbHid
    );

void
ValidateUsbHidContext(
    PUSBHID_CONTEXT pUsbHid 
    );


// Data dumping macros

#define DUMP_USB_DEVICE_DESCRIPTOR( d ) { \
   DEBUGMSG( ZONE_PARSE, (_T("USB_DEVICE_DESCRIPTOR:\n"))); \
   DEBUGMSG( ZONE_PARSE, (_T("----------------------\n"))); \
   DEBUGMSG( ZONE_PARSE, (_T("bLength: 0x%x\n"), d.bLength ));   \
   DEBUGMSG( ZONE_PARSE, (_T("bDescriptorType: 0x%x\n"), d.bDescriptorType ));  \
   DEBUGMSG( ZONE_PARSE, (_T("bcdUSB: 0x%x\n"), d.bcdUSB ));  \
   DEBUGMSG( ZONE_PARSE, (_T("bDeviceClass: 0x%x\n"), d.bDeviceClass ));  \
   DEBUGMSG( ZONE_PARSE, (_T("bDeviceSubClass: 0x%x\n"), d.bDeviceSubClass ));  \
   DEBUGMSG( ZONE_PARSE, (_T("bDeviceProtocol: 0x%x\n"), d.bDeviceProtocol ));  \
   DEBUGMSG( ZONE_PARSE, (_T("bMaxPacketSize0: 0x%x\n"), d.bMaxPacketSize0 ));  \
   DEBUGMSG( ZONE_PARSE, (_T("idVendor: 0x%x\n"), d.idVendor )); \
   DEBUGMSG( ZONE_PARSE, (_T("idProduct: 0x%x\n"), d.idProduct ));  \
   DEBUGMSG( ZONE_PARSE, (_T("bcdDevice: 0x%x\n"), d.bcdDevice ));  \
   DEBUGMSG( ZONE_PARSE, (_T("iManufacturer: 0x%x\n"), d.iManufacturer ));   \
   DEBUGMSG( ZONE_PARSE, (_T("iProduct: 0x%x\n"), d.iProduct )); \
   DEBUGMSG( ZONE_PARSE, (_T("iSerialNumber: 0x%x\n"), d.iSerialNumber ));   \
   DEBUGMSG( ZONE_PARSE, (_T("bNumConfigurations: 0x%x\n"), d.bNumConfigurations ));  \
   DEBUGMSG( ZONE_PARSE, (_T("\n")));  \
}

#define DUMP_USB_CONFIGURATION_DESCRIPTOR( c ) { \
   DEBUGMSG( ZONE_PARSE, (_T("USB_CONFIGURATION_DESCRIPTOR:\n"))); \
   DEBUGMSG( ZONE_PARSE, (_T("-----------------------------\n"))); \
   DEBUGMSG( ZONE_PARSE, (_T("bLength: 0x%x\n"), c.bLength )); \
   DEBUGMSG( ZONE_PARSE, (_T("bDescriptorType: 0x%x\n"), c.bDescriptorType )); \
   DEBUGMSG( ZONE_PARSE, (_T("wTotalLength: 0x%x\n"), c.wTotalLength )); \
   DEBUGMSG( ZONE_PARSE, (_T("bNumInterfaces: 0x%x\n"), c.bNumInterfaces )); \
   DEBUGMSG( ZONE_PARSE, (_T("bConfigurationValue: 0x%x\n"), c.bConfigurationValue )); \
   DEBUGMSG( ZONE_PARSE, (_T("iConfiguration: 0x%x\n"), c.iConfiguration )); \
   DEBUGMSG( ZONE_PARSE, (_T("bmAttributes: 0x%x\n"), c.bmAttributes )); \
   DEBUGMSG( ZONE_PARSE, (_T("MaxPower: 0x%x\n"), c.MaxPower )); \
   DEBUGMSG( ZONE_PARSE, (_T("\n"))); \
}

#define DUMP_USB_INTERFACE_DESCRIPTOR( i, _index ) { \
   DEBUGMSG( ZONE_PARSE, (_T("USB_INTERFACE_DESCRIPTOR[%d]:\n"), _index )); \
   DEBUGMSG( ZONE_PARSE, (_T("-------------------------\n"))); \
   DEBUGMSG( ZONE_PARSE, (_T("bLength: 0x%x\n"), i.bLength )); \
   DEBUGMSG( ZONE_PARSE, (_T("bDescriptorType: 0x%x\n"), i.bDescriptorType )); \
   DEBUGMSG( ZONE_PARSE, (_T("bInterfaceNumber: 0x%x\n"), i.bInterfaceNumber )); \
   DEBUGMSG( ZONE_PARSE, (_T("bAlternateSetting: 0x%x\n"), i.bAlternateSetting )); \
   DEBUGMSG( ZONE_PARSE, (_T("bNumEndpoints: 0x%x\n"), i.bNumEndpoints )); \
   DEBUGMSG( ZONE_PARSE, (_T("bInterfaceClass: 0x%x\n"), i.bInterfaceClass )); \
   DEBUGMSG( ZONE_PARSE, (_T("bInterfaceSubClass: 0x%x\n"), i.bInterfaceSubClass )); \
   DEBUGMSG( ZONE_PARSE, (_T("bInterfaceProtocol: 0x%x\n"), i.bInterfaceProtocol )); \
   DEBUGMSG( ZONE_PARSE, (_T("iInterface: 0x%x\n"), i.iInterface )); \
   DEBUGMSG( ZONE_PARSE, (_T("\n"))); \
}

#define DUMP_USB_ENDPOINT_DESCRIPTOR( e ) { \
   DEBUGMSG( ZONE_PARSE, (_T("USB_ENDPOINT_DESCRIPTOR:\n"))); \
   DEBUGMSG( ZONE_PARSE, (_T("-----------------------------\n"))); \
   DEBUGMSG( ZONE_PARSE, (_T("bLength: 0x%x\n"), e.bLength )); \
   DEBUGMSG( ZONE_PARSE, (_T("bDescriptorType: 0x%x\n"), e.bDescriptorType )); \
   DEBUGMSG( ZONE_PARSE, (_T("bEndpointAddress: 0x%x\n"), e.bEndpointAddress )); \
   DEBUGMSG( ZONE_PARSE, (_T("bmAttributes: 0x%x\n"), e.bmAttributes )); \
   DEBUGMSG( ZONE_PARSE, (_T("wMaxPacketSize: 0x%x\n"), e.wMaxPacketSize )); \
   DEBUGMSG( ZONE_PARSE, (_T("bInterval: 0x%x\n"), e.bInterval ));\
   DEBUGMSG( ZONE_PARSE, (_T("\n"))); \
}

#define DUMP_USB_HID_DESCRIPTOR( h ) { \
   DEBUGMSG( ZONE_PARSE, (_T("USB_HID_DESCRIPTOR:\n"))); \
   DEBUGMSG( ZONE_PARSE, (_T("-----------------------------\n"))); \
   DEBUGMSG( ZONE_PARSE, (_T("bLength: 0x%x\n"), h.bLength )); \
   DEBUGMSG( ZONE_PARSE, (_T("bDescriptorType: 0x%x\n"), h.bDescriptorType )); \
   DEBUGMSG( ZONE_PARSE, (_T("bcdHID: 0x%x\n"), h.bcdHID )); \
   DEBUGMSG( ZONE_PARSE, (_T("bCountryCode: 0x%x\n"), h.bCountryCode )); \
   DEBUGMSG( ZONE_PARSE, (_T("bNumDescriptors: 0x%x\n"), h.bNumDescriptors )); \
   DEBUGMSG( ZONE_PARSE, (_T("bClassDescriptorType: 0x%x\n"), h.bClassDescriptorType )); \
   DEBUGMSG( ZONE_PARSE, (_T("wDescriptorLength: 0x%x\n"), h.wDescriptorLength )); \
   DEBUGMSG( ZONE_PARSE, (_T("\n"))); \
}


#else

#define IsUsbHidContextLocked(ptr)
#define LockUsbHidContext(x) EnterCriticalSection(&x->csLock)
#define UnlockUsbHidContext(x) LeaveCriticalSection(&x->csLock)
#define ValidateUsbHidContext(ptr)

#define DUMP_USB_DEVICE_DESCRIPTOR( p )
#define DUMP_USB_CONFIGURATION_DESCRIPTOR( c )
#define DUMP_USB_INTERFACE_DESCRIPTOR( i, index )
#define DUMP_USB_ENDPOINT_DESCRIPTOR( e )
#define DUMP_USB_HID_DESCRIPTOR( h )

#endif // DEBUG


#endif // _USBHID_H_

