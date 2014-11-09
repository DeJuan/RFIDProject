/**
 *  @file RDRscMgrInterface.h
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

#import <Foundation/Foundation.h>
#import "MercuryAPI.h"

#import "RscMgr.h"

#define BUFFER_LEN 1024

#define TEST_DATA_LEN 256

typedef enum CableConnectState
{
	kCableNotConnected,
	kCableConnected,
	kCableRequiresPasscode
    
} CableConnectState;

typedef enum
{
	kBaudIndex = 0,
	kDataBitsIndex = 1,
	kParityIndex = 2,
	kStopBitsIndex = 3
	
} PortConfigSettingType;

typedef enum
{
	kStatRx = 0,
	kStatTx = 1,
    kStatErr = 2
} StatType;

@protocol RDRscMgrInterfaceDelegate <NSObject>

- (void)rscMgrCableConnected;
- (void)rscMgrCableDisconnected;

@end

@interface RDRscMgrInterface : NSObject <RscMgrDelegate>
{
    UInt8 rxBuffer[BUFFER_LEN];
	UInt8 txBuffer[BUFFER_LEN];

    int rxCount;
    int txCount;
    int errCount;
    
    UInt8 seqNum;
}

+ (RDRscMgrInterface*)sharedInterface;

@property (nonatomic, strong) RscMgr *rscMgr;
@property (nonatomic, assign) CableConnectState cableState;
@property (nonatomic, strong) NSMutableData *bufferedData;
@property (nonatomic, assign) int lastReadIndex;
@property (nonatomic, assign) id <RDRscMgrInterfaceDelegate>delegate;

- (void)startDeviceTest;

@end
