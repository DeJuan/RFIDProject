/**
 *  @file RootController.h
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

#import <UIKit/UIKit.h>
#import "LogController.h"
#import "RscMgr.h"
#import "tm_reader.h"


@interface RDViewController : UIViewController <UICollectionViewDataSource, UICollectionViewDelegate,UITextFieldDelegate>
{
    TMR_Reader r, *rp;
    TMR_Status ret;
    
    NSThread *commThread;
    
    NSMutableArray *readDataArray;
}

@property (strong, nonatomic) IBOutlet UITextField *readTimeOut;

@property (strong, nonatomic) IBOutlet UILabel *readtimelabel;


@property (nonatomic, strong) IBOutlet UICollectionView *collectionView;
@property (nonatomic, strong) IBOutlet UILabel *totalTagsCount;
@property (nonatomic, strong) IBOutlet UILabel *uniqueTagsCount;

@property (strong, nonatomic) IBOutlet UITextField *readtext;
@property (strong, nonatomic) IBOutlet UILabel *exceptionlabel;
@property (nonatomic, strong) IBOutlet UIButton *connectBtn;
@property (nonatomic, strong) IBOutlet UIButton *readBtn;

@property (nonatomic, strong) IBOutlet UIToolbar *bottomToolBar;

@property (nonatomic, strong) LogController *logWindow;

@property (nonatomic, strong) IBOutlet UIImageView *connectedIndicator;
@property (strong, nonatomic) IBOutlet UIButton *clearBtn;

- (IBAction)log:(id)sender;
- (IBAction)connectBtnTouched:(id)sender;
- (IBAction)readBtnTouched:(id)sender;
- (IBAction)clearBtnTouched:(id)sender;
- (IBAction)disconnectBtnTouched:(id)sender;

@end
