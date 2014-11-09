/**
 *  @file RDRscMgrInterface.m
 *  @brief RFIDReader
 *  @author Surendra
 *  @date 08/03/14
 */

/*
 * Copyright (c) 2014 Trimble, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#import "RDRscMgrInterface.h"

#include <sys/time.h>
#include <unistd.h>

static RDRscMgrInterface *_sharedInterface = nil;

@implementation RDRscMgrInterface

+ (RDRscMgrInterface*)sharedInterface
{
    static dispatch_once_t pred = 0;
    dispatch_once(&pred, ^{
        _sharedInterface = [[self alloc] init];
    });
    return _sharedInterface;
}

- (id)init
{
    self = [super init];
    
    if (self)
    {
        _bufferedData = [NSMutableData data];
        _lastReadIndex = 0;
        
        _rscMgr = [[RscMgr alloc] init];
        
        [_rscMgr setBaud:9600];
        [_rscMgr setDataSize:kDataSize8];
        [_rscMgr setParity:kParityNone];
        [_rscMgr setStopBits:kStopBits1];
        
        _rscMgr.delegate = self;
    }
    
    return self;
}


- (void)startDeviceTest
{

}

- (void) resetCounters
{
    txCount = rxCount = errCount = 0;
    seqNum = 0;
}

#pragma mark - RscMgrDelegate method

- (void) cableConnected:(NSString *)protocol
{
    DLog(@"Cable Connected: %@", protocol);
    
    serialPortConfig portCfg;
	[_rscMgr getPortConfig:&portCfg];
    
    portCfg.txAckSetting = 1;
    portCfg.rxForwardingTimeout = 5;
    
    [_rscMgr setPortConfig:&portCfg requestStatus: NO];
    
    [_rscMgr open];
    
    [_rscMgr setBaud:9600];
    [_rscMgr setDataSize:kDataSize8];
    [_rscMgr setParity:kParityNone];
    [_rscMgr setStopBits:kStopBits1];
    
    _cableState = kCableConnected;
    
    if (_delegate && [_delegate respondsToSelector:@selector(rscMgrCableConnected)])
    {
        [_delegate rscMgrCableConnected];
    }
    
}

- (void) cableDisconnected
{
    DLog(@"Cable disconnected");
    
    _cableState = kCableNotConnected;
    
    if (_delegate && [_delegate respondsToSelector:@selector(rscMgrCableDisconnected)])
    {
        [_delegate rscMgrCableDisconnected];
    }
    
    [self resetCounters];
}

- (void) portStatusChanged
{
    DLog(@"portStatusChanged");
}

- (void) readBytesAvailable:(UInt32)length
{
    DLog(@"readBytesAvailable: %d", (unsigned int)length);
    int len;
    DLog(@"before read _bufferedData length: %d", [_bufferedData length]);
    len = [_rscMgr read:rxBuffer length:length];
    [_bufferedData appendBytes:rxBuffer length:len];

    DLog(@"after read _bufferedData length: %d", [_bufferedData length]);

    NSMutableString *stringBuffer = [NSMutableString stringWithCapacity:length*2];
    const unsigned char *dataBuffer = [_bufferedData bytes];
    dataBuffer = [_bufferedData bytes];
    for (int i=0; i < [_bufferedData length]; ++i)
    {
        [stringBuffer appendFormat:@"%x ", dataBuffer[i]];
    }

    DLog(@"Data read: %@", stringBuffer);
}

// called when a response is received to a getPortConfig call
- (void) didReceivePortConfig
{
    ULog(@"didReceivePortConfig");
    
}

// GPS Cable only - called with result when loop test completes.
- (void) didGpsLoopTest:(BOOL)pass
{
    ULog(@"didGpsLoopTest");
    
}

- (BOOL) rscMessageReceived:(UInt8 *)msg TotalLength:(int)len
{
    DLog(@"rscMessageRecieved:TotalLength:");
    return FALSE;
}

#pragma mark - C_API

TMR_Status RDRscMgrOpen (void *self)
{
    DLog(@"RDRscMgrOpen");
    
    RscMgr *rscMgr = [[RDRscMgrInterface sharedInterface] rscMgr];
    
    serialPortConfig portCfg;
	[rscMgr getPortConfig:&portCfg];
    portCfg.txAckSetting = 1;
    portCfg.rxForwardingTimeout = 5;
    [rscMgr setPortConfig:&portCfg requestStatus: NO];
    
    serialPortControl portCtl;
    portCtl.rxFlush = 1;
    portCtl.txFlush = 1;
	[rscMgr setPortControl:&portCtl requestStatus:NO];

    
    [rscMgr open];
    
    return TMR_SUCCESS;
}

TMR_Status RDRscMgrSendBytes (TMR_SR_SerialTransport *this, uint32_t length,
                              uint8_t* message, const uint32_t timeoutMs)
{
    NSMutableString *str = [NSMutableString string];
    int i;
    for (i = 0; i < length; i++)
    {
        [str appendFormat:@"%02x ", message[i]];
    }
    
    int lengthOfBufferedData = [[RDRscMgrInterface sharedInterface].bufferedData length];

    if (lengthOfBufferedData)
    {
        DLog(@"Data already exists in bufferedData with size %d, deleting it", lengthOfBufferedData);
        [RDRscMgrInterface sharedInterface].bufferedData = nil;
        [RDRscMgrInterface sharedInterface].bufferedData = [NSMutableData data];
    }
    DLog(@"RDRscMgrSendBytes messageLenth: %d %@", length, str);
    i = [[[RDRscMgrInterface sharedInterface] rscMgr] write:message Length:length];
    return TMR_SUCCESS;
}


TMR_Status RDRscMgrReceiveBytes (TMR_SR_SerialTransport *this, uint32_t length,
                                 uint32_t* messageLength, uint8_t* message, const uint32_t timeoutMs)
{
    DLog(@"RDRscMgrReceiveBytes: %d messageLength: %d, timeout: %d", length, *messageLength, timeoutMs);
    
    TMR_Status status = TMR_ERROR_TIMEOUT;
    
    struct timeval currentTime, initialTime;
    
    if (*messageLength == -1)
    {
        *messageLength = 0;
    }
    
    gettimeofday(&initialTime, NULL);
    gettimeofday(&currentTime, NULL);

    double currentMillis, initialMillis;
    currentMillis = initialMillis = (initialTime.tv_sec * 1000) + (initialTime.tv_usec / 1000);
    
    while((currentMillis - initialMillis) < timeoutMs*20)
    {
        int lengthOfBufferedData = [[RDRscMgrInterface sharedInterface].bufferedData length];

        if ((lengthOfBufferedData >= (length + (*messageLength))) && lengthOfBufferedData > 0)
        {
            NSMutableString *stringBuffer = [NSMutableString stringWithCapacity:length*2];
            const unsigned char *dataBuffer = [[RDRscMgrInterface sharedInterface].bufferedData bytes];
            NSInteger i;
            for (i=0; i < [[RDRscMgrInterface sharedInterface].bufferedData length]; ++i)
            {
                [stringBuffer appendFormat:@"%x ", dataBuffer[i]];
            }
            DLog(@"Data in buffer while writing: %@", stringBuffer);

            NSRange rangeToWrite = NSMakeRange(*messageLength, length);

            memcpy(message, [[[RDRscMgrInterface sharedInterface].bufferedData subdataWithRange:rangeToWrite] bytes], length);

            *messageLength += length;
            message += length;

            status = TMR_SUCCESS;
            break;
        }
        else
        {
            gettimeofday(&currentTime, NULL);

            currentMillis = (currentTime.tv_sec * 1000) + (currentTime.tv_usec / 1000);
            usleep(10 * 1000);
        }
    }

    if (status == TMR_ERROR_TIMEOUT)
    {
        DLog(@"RDRscMgrReceiveBytes TIMEOUT");
    }

    return status;
}



TMR_Status setRDRscMgrBaudRate (TMR_SR_SerialTransport *this, uint32_t rate)
{
    DLog(@"setRDRscMgrBaudRate:%d", rate);
    
    [[[RDRscMgrInterface sharedInterface] rscMgr] setBaud:rate];
    
    return TMR_SUCCESS;
}



TMR_Status RDRscMgrshutdown (TMR_SR_SerialTransport *this)
{
    DLog(@"RDRscMgrReceiveBytes");
    
    return TMR_SUCCESS;
}



TMR_Status RDRscMgrFlush (TMR_SR_SerialTransport *this)
{
    DLog(@"RDRscMgrFlush");
    RscMgr *rscMgr = [[RDRscMgrInterface sharedInterface] rscMgr];
    
    [RDRscMgrInterface sharedInterface].lastReadIndex = 0;
    
    serialPortControl portCtl;
    portCtl.rxFlush = 1;
    portCtl.txFlush = 1;
    
	[rscMgr setPortControl:&portCtl requestStatus:NO];
    
    int lengthOfBufferedData = [[RDRscMgrInterface sharedInterface].bufferedData length];
    
    if (lengthOfBufferedData)
    {
        DLog(@"Data already exists in bufferedData with size %d, deleting it", lengthOfBufferedData);
        [RDRscMgrInterface sharedInterface].bufferedData = nil;
        [RDRscMgrInterface sharedInterface].bufferedData = [NSMutableData data];
    }
    
    return TMR_SUCCESS;
}

@end
