//
//  RDRscMgrCInterface.h
//  RFIDDemo
//
//  Created by qvantel on 08/03/14.
//  Copyright (c) 2014 qvantel. All rights reserved.
//

#ifndef __RDRSCMGR_C_INTERFACE_H__
#define __RDRSCMGR_C_INTERFACE_H__

#include "tmr_status.h"
#include "tmr_serial_transport.h"

// This is the C "trampoline" function that will be used
// to invoke a specific Objective-C method FROM C++
TMR_Status RDRscMgrOpen (void *self);


TMR_Status RDRscMgrSendBytes (TMR_SR_SerialTransport *this, uint32_t length,
                              uint8_t* message, const uint32_t timeoutMs);


TMR_Status RDRscMgrReceiveBytes (TMR_SR_SerialTransport *this, uint32_t length,
                                 uint32_t* messageLength, uint8_t* message, const uint32_t timeoutMs);



TMR_Status setRDRscMgrBaudRate (TMR_SR_SerialTransport *this, uint32_t rate);



TMR_Status RDRscMgrshutdown (TMR_SR_SerialTransport *this);



TMR_Status RDRscMgrFlush (TMR_SR_SerialTransport *this);

#endif
