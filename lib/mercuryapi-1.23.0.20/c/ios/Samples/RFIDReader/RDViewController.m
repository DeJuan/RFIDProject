/**
 *  @file RootController.m
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


#import "RDViewController.h"
#import "RDReadValuesCollectonViewCell.h"
#import "RFIDReadingDataModal.h"
#import "RDRscMgrInterface.h"
#import <QuartzCore/QuartzCore.h>

#define kCellID @"readValuesCollectionViewCell"
#define CONNECT_TITLE @"Connect"
#define DISCONNECT_TITLE @"Disconnect"

@interface RDViewController () <RDRscMgrInterfaceDelegate>

@end;

@implementation RDViewController

@synthesize readTimeOut;

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
}

#pragma mark - View lifecycle

- (void)viewDidLoad
{
    [super viewDidLoad];
    readTimeOut.delegate = self;
    self.bottomToolBar.hidden = TRUE;
    readDataArray = [NSMutableArray array];
    self.readBtn.enabled = FALSE;
    self.clearBtn.enabled = FALSE;
    self.exceptionlabel.hidden = TRUE;
    [self.readTimeOut setText:@"500"];
    
    [[RDRscMgrInterface sharedInterface] setDelegate:self];
    
    if ([[RDRscMgrInterface sharedInterface] cableState] == kCableNotConnected)
    {
        [self disconnectImageIndication];
        self.connectBtn.titleLabel.textColor = [UIColor lightGrayColor];
    }
    else
    {
        [self connectImageIndication];
        self.exceptionlabel.hidden = TRUE;
    }
    
#if DEBUG
    self.bottomToolBar.hidden = FALSE;
    self.logWindow = [[LogController alloc] initWithNibName:@"LogController" bundle:nil];
#endif
}

- (void)disconnectImageIndication
{
    UIImage *img = [UIImage imageNamed:@"RedLight.png"];
    self.connectedIndicator.image = img;
    self.connectBtn.enabled = FALSE;
    self.exceptionlabel.hidden = TRUE;
    [self.readtext setText:@""];
    [self clearBtnTouched:nil];
    self.readtimelabel.hidden = TRUE;
    self.readTimeOut.hidden = TRUE;
}

- (void)connectImageIndication
{
    UIImage *img = [UIImage imageNamed:@"OrangeLight.png"];
    self.connectedIndicator.image = img;
    self.connectBtn.enabled = TRUE;
    NSArray *accessories = [[EAAccessoryManager sharedAccessoryManager] connectedAccessories];
    for (EAAccessory *accessory in accessories)
    {
        [self.readtext setText:[NSString stringWithFormat:@"%@ %@",accessory.manufacturer,accessory.modelNumber]];
        NSLog(@"accessory name %@ Accessory MManufacturer %@",accessory.manufacturer,accessory.modelNumber);
    }
    self.readtimelabel.hidden = TRUE;
    self.readTimeOut.hidden = TRUE;
}

- (void)rscMgrCableConnected
{
    [self connectImageIndication];
    self.connectBtn.selected = FALSE;
}

- (void)rscMgrCableDisconnected
{
    [self disconnectImageIndication];
    self.connectBtn.selected = FALSE;
    self.readBtn.enabled = FALSE;
    self.connectBtn.titleLabel.textColor = [UIColor lightGrayColor];
}


- (void)viewDidUnload
{
    [self setConnectedIndicator:nil];
    [super viewDidUnload];
    self.exceptionlabel.hidden = TRUE;
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    return UIInterfaceOrientationIsPortrait(interfaceOrientation);
}

#pragma mark - UICollectionViewDataSource

- (NSInteger)collectionView:(UICollectionView *)collectionView numberOfItemsInSection:(NSInteger)section
{
    return [readDataArray count];
}

// The cell that is returned must be retrieved from a call to -dequeueReusableCellWithReuseIdentifier:forIndexPath:
- (UICollectionViewCell *)collectionView:(UICollectionView *)collectionView cellForItemAtIndexPath:(NSIndexPath *)indexPath
{
    RDReadValuesCollectonViewCell *cell = [collectionView dequeueReusableCellWithReuseIdentifier:kCellID forIndexPath:indexPath];
    
    cell.autoresizingMask = UIViewAutoresizingFlexibleWidth;
    
    
    RFIDReadingDataModal *readingData = [readDataArray objectAtIndex:indexPath.row];
    
    CGSize size=[readingData.epic sizeWithFont:[UIFont systemFontOfSize:18] constrainedToSize:CGSizeMake(508, MAXFLOAT)];
    float height=size.height;
    
    if (indexPath.item == readDataArray.count)
    {
        height = size.height + 1;
    }
    
    cell.epicLabel.frame = CGRectMake(2, 0, 506, height);
    cell.snoLabel.frame = CGRectMake(0, 0, 99, height);
    cell.countLabel.frame = CGRectMake(608, 0, 100, height);
    cell.backgroundView.frame = CGRectMake(99, 0, 508, height);
    
    cell.snoLabel.text = readingData.sno;
    cell.epicLabel.text = readingData.epic;
    cell.countLabel.text = [readingData.count stringValue];
    cell.epicLabel.numberOfLines = 2;
    return cell;
}

- (CGSize)collectionView:(UICollectionView *)collectionView layout:(UICollectionViewLayout*)collectionViewLayout sizeForItemAtIndexPath:(NSIndexPath *)indexPath
{
    RFIDReadingDataModal *model=[readDataArray objectAtIndex: indexPath.item];
    CGSize size=[model.epic sizeWithFont:[UIFont systemFontOfSize:18] constrainedToSize:CGSizeMake(508, MAXFLOAT)];
    NSLog(@" Height of the cell if string extends %f",size.height);
    
    return CGSizeMake(728, size.height + 1);
}

- (IBAction)connectBtnTouched:(id)sender
{
    DLog(@"connectBtnTouched");
    
    self.connectBtn.userInteractionEnabled = FALSE;
    self.connectBtn.titleLabel.textColor = [UIColor lightGrayColor];
    
    UIActivityIndicatorView *spinner = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleGray];
    spinner.center = self.collectionView.center;
    [self.view addSubview:spinner];
    [spinner startAnimating];
    
    self.exceptionlabel.hidden = TRUE;
    
    if (self.connectBtn.selected)
    {
        [self disconnectBtnTouched:nil];
        self.connectBtn.selected = FALSE;
        self.exceptionlabel.hidden = TRUE;
        self.connectBtn.userInteractionEnabled = TRUE;
        self.connectBtn.titleLabel.textColor = [UIColor colorWithRed:(0/255.0) green:(105/255.0) blue:(229/255.0) alpha:1];
        [spinner stopAnimating];
        self.readtimelabel.hidden = TRUE;
        self.readTimeOut.hidden = TRUE;
    }
    else
    {
        dispatch_async( dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
            
            if ([[RDRscMgrInterface sharedInterface] cableState] == kCableNotConnected)
            {
                ULog(@"Please connect Redpark cable");
                return;
            }
            
            self.exceptionlabel.hidden = TRUE;
            rp = &r;
            NSArray *accessories = [[EAAccessoryManager sharedAccessoryManager] connectedAccessories];
            for (EAAccessory *accessory in accessories)
            {
                NSString *name = accessory.manufacturer;
                NSString *model = accessory.modelNumber;
                
                const char *deviceURI = [[NSString stringWithFormat:@"tmr:///%@/%@",name,model] UTF8String];
                NSLog(@"my device uri %s",deviceURI);
                ret = TMR_create(rp, deviceURI);
            }
            
            DLog(@"TMR_create Status:%d", ret);
            if (TMR_SUCCESS == ret)
            {
                ret = TMR_connect(rp);
                DLog(@"TMR_connect Status:%d", ret);
                @autoreleasepool
                {
                    @try
                    {
                        if (TMR_SUCCESS == ret)
                        {
                            dispatch_async( dispatch_get_main_queue(), ^{
                                // Add code here to update the UI/send notifications based on the results of the background processing
                                UIImage *img = [UIImage imageNamed:@"GreenLight.png"];
                                self.connectedIndicator.image = img;
                                self.connectBtn.selected = TRUE;
                                self.readBtn.enabled = TRUE;
                                self.connectBtn.userInteractionEnabled = TRUE;
                                self.readtimelabel.hidden = FALSE;
                                self.readTimeOut.hidden = FALSE;
                                [spinner stopAnimating];
                            });
                        }
                        else
                        {
                            @throw [NSException alloc];
                        }
                    }
                    @catch (NSException *exception)
                    {
                        
                        dispatch_async( dispatch_get_main_queue(), ^{
                            self.exceptionlabel.hidden = FALSE;
                            [spinner stopAnimating];
                            self.connectBtn.userInteractionEnabled = TRUE;
                            self.clearBtn.enabled = TRUE;
                            self.exceptionlabel.text = @"* Couldn't open device";
                            NSLog(@"* Couldn't open device");
                        });
                    }
                }
            }
            else if (TMR_ERROR_TIMEOUT == ret)
            {
                dispatch_async( dispatch_get_main_queue(), ^{
                    self.exceptionlabel.hidden = FALSE;
                    self.clearBtn.enabled = TRUE;
                    self.exceptionlabel.text = @"* Timeout Exception";
                    NSLog(@"* Timeout Exception");
                });
            }
        });
    }
}

- (IBAction)readBtnTouched:(id)sender
{
    UIActivityIndicatorView *spinner = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleGray];
    spinner.center = self.collectionView.center;
    [self.view addSubview:spinner];
    [spinner startAnimating];
    self.connectBtn.userInteractionEnabled = FALSE;
    self.connectBtn.titleLabel.textColor = [UIColor lightGrayColor];
    
    if (self.readTimeOut.text.length == 0)
    {
        dispatch_async( dispatch_get_main_queue(), ^{
            [spinner stopAnimating];
            [self readTimeExceptionRised];
            self.exceptionlabel.text = @"read timeout can't be empty";
        });
    }
    else if ([self.readTimeOut.text isEqualToString:@"0"])
    {
        dispatch_async( dispatch_get_main_queue(), ^{
            [spinner stopAnimating];
            [self readTimeExceptionRised];
            self.exceptionlabel.text = @"read timeout should be greater than zero";
        });
    }
    else
    {
        if ([readTimeOut.text intValue] > 65535) {
            dispatch_async( dispatch_get_main_queue(), ^{
                [spinner stopAnimating];
                [self readTimeExceptionRised];
                self.exceptionlabel.text = @"read timeout must be less than 65535";
            });
        }
        else
        {
            self.readBtn.enabled = FALSE;
            self.clearBtn.enabled = FALSE;
            self.exceptionlabel.hidden = TRUE;
            [self clearBtnTouched:nil];
            
            dispatch_async( dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
                TMR_Region region;
                region = TMR_REGION_NONE;
                
                ret = TMR_paramGet(rp, TMR_PARAM_REGION_ID, &region);
                DLog(@"region TMR_paramGet %d", region);
                
                if (TMR_REGION_NONE == region)
                {
                    TMR_RegionList regions;
                    TMR_Region _regionStore[32];
                    regions.list = _regionStore;
                    regions.max = sizeof(_regionStore)/sizeof(_regionStore[0]);
                    regions.len = 0;
                    
                    ret = TMR_paramGet(rp, TMR_PARAM_REGION_SUPPORTEDREGIONS, &regions);
                    
                    if (regions.len < 1)
                    {
                        DLog(@"Reader does not support any region");
                    }
                    
                    region = regions.list[0];
                    ret = TMR_paramSet(rp, TMR_PARAM_REGION_ID, &region);
                    DLog(@"Setting region %d", region);
                }
                
                int snoCounter = 0;
                ret = TMR_read(rp,readTimeOut.text.intValue, NULL);
                if (TMR_ERROR_TIMEOUT == ret)
                {
                    dispatch_async( dispatch_get_main_queue(), ^{
                        self.exceptionlabel.hidden = FALSE;
                        self.readBtn.enabled = FALSE;
                        self.clearBtn.enabled = TRUE;
                        self.connectBtn.selected = FALSE;
                        UIImage *img = [UIImage imageNamed:@"OrangeLight.png"];
                        self.connectedIndicator.image = img;
                        self.exceptionlabel.text = @"* Operation timeout";
                        NSLog(@"* Operation timeout");
                        [spinner stopAnimating];
                        self.connectBtn.userInteractionEnabled = TRUE;
                        self.connectBtn.titleLabel.textColor = [UIColor colorWithRed:(0/255.0) green:(105/255.0) blue:(229/255.0) alpha:1];
                    });
                }
                else
                {
                    
                    if (TMR_ERROR_CRC_ERROR == ret)
                    {
                        dispatch_async( dispatch_get_main_queue(), ^{
                            [self ExceptionRised];
                            self.exceptionlabel.text = @"* CRC Error";
                            NSLog(@"* CRC Error");
                            [spinner stopAnimating];
                        });
                    }
                    if((TMR_ERROR_TIMEOUT == ret))
                    {
                        dispatch_async( dispatch_get_main_queue(), ^{
                            [self ExceptionRised];
                            self.connectBtn.selected = FALSE;
                            UIImage *img = [UIImage imageNamed:@"OrangeLight.png"];
                            self.connectedIndicator.image = img;
                            self.exceptionlabel.text = @"* Operation timeout";
                            NSLog(@"* Operation timeout");
                            [spinner stopAnimating];
                        });
                    }
                    
                    DLog(@"TMR_read Status:%d", ret);
                    
                    if(TMR_ERROR_NO_ANTENNA == ret)
                    {
                        dispatch_async( dispatch_get_main_queue(), ^{
                            [self ExceptionRised];
                            self.exceptionlabel.text = @"* Antenna not Connected";
                            NSLog(@"* Antenna not Connected");
                            [spinner stopAnimating];
                        });
                    }
                    else
                    {
                        readDataArray = [NSMutableArray array];
                        while (TMR_SUCCESS == TMR_hasMoreTags(rp))
                        {
                            TMR_TagReadData trd;
                            char epcStr[128];
                            ret = TMR_getNextTag(rp, &trd);
                            if((TMR_SUCCESS != ret) && (TMR_ERROR_CRC_ERROR == ret))
                            {
                                dispatch_async( dispatch_get_main_queue(), ^{
                                    [self ExceptionRised];
                                    self.exceptionlabel.text = @"* CRC Error";
                                    NSLog(@"* CRC Error");
                                    [spinner stopAnimating];
                                });
                            }
                            DLog(@"TMR_getNextTag Status: %d", ret);
                            TMR_bytesToHex(trd.tag.epc, trd.tag.epcByteCount, epcStr);
                            RFIDReadingDataModal *readData1 = [[RFIDReadingDataModal alloc] init];
                            readData1.sno = [NSString stringWithFormat:@"%d", ++snoCounter];
                            readData1.epic = [NSString stringWithFormat:@"%s", epcStr];
                            readData1.count = [NSNumber numberWithInt:trd.readCount];
                            [readDataArray addObject:readData1];
                            
                            DLog(@"EPC:%s ant:%d, count:%d\n", epcStr, trd.antenna, trd.readCount);
                        }
                        if ((TMR_ERROR_NO_TAGS != ret) && (TMR_ERROR_CRC_ERROR == ret))
                        {
                            dispatch_async( dispatch_get_main_queue(), ^{
                                [self ExceptionRised];
                                self.exceptionlabel.text = @"* CRC Error";
                                NSLog(@"* CRC Error");
                                [spinner stopAnimating];
                            });
                        }
                        
                        dispatch_async( dispatch_get_main_queue(), ^{
                            // Add code here to update the UI/send notifications based on the results of the background processing
                            [self refreshView];
                            self.connectBtn.userInteractionEnabled = TRUE;
                            self.connectBtn.titleLabel.textColor = [UIColor colorWithRed:(0/255.0) green:(105/255.0) blue:(229/255.0) alpha:1];
                            [spinner stopAnimating];
                            self.readBtn.enabled = TRUE;
                            self.clearBtn.enabled = TRUE;
                        });
                    }
                }
            });
        }
    }
}

-(void)refreshView
{
    [self.collectionView reloadData];
    
    self.exceptionlabel.hidden = TRUE;
    NSArray *tagsCountArray = [readDataArray valueForKeyPath:@"count"];
    NSNumber *sum = [tagsCountArray valueForKeyPath:@"@sum.self"];
    
    self.uniqueTagsCount.text = [NSString stringWithFormat:@"%d", [readDataArray count]];
    self.totalTagsCount.text = [sum stringValue];
}

- (IBAction)clearBtnTouched:(id)sender
{
    self.uniqueTagsCount.text = @"0";
    self.totalTagsCount.text = @"0";
    readDataArray = [NSMutableArray array];
    [self.collectionView reloadData];
    self.exceptionlabel.hidden = TRUE;
    self.clearBtn.enabled = FALSE;
}

- (IBAction)disconnectBtnTouched:(id)sender
{
    [self clearBtnTouched:nil];
    UIImage *img = [UIImage imageNamed:@"OrangeLight.png"];
    self.connectedIndicator.image = img;
    self.readBtn.enabled = FALSE;
    self.clearBtn.enabled = FALSE;
}


- (BOOL)textField:(UITextField *)textField shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString *)string
{
    // verify the text field you wanna validate
    if (textField.keyboardType == UIKeyboardTypeNumberPad)
    {
        if ([string rangeOfCharacterFromSet:[[NSCharacterSet decimalDigitCharacterSet] invertedSet]].location != NSNotFound)
        {
            return NO;
        }
    }
    if (textField == readTimeOut)
    {
        NSLog(@"read %d",[readTimeOut.text intValue]);
        // in case you need to limit the max number of characters
        if ([readTimeOut.text stringByReplacingCharactersInRange:range withString:string].length > 5)
        {
            return NO;
        }
    }
    return YES;
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
    [textField resignFirstResponder];
    return YES;
}

- (void)readTimeExceptionRised
{
    [self clearBtnTouched:nil];
    self.clearBtn.enabled = TRUE;
    self.connectBtn.userInteractionEnabled = TRUE;
    self.connectBtn.titleLabel.textColor = [UIColor colorWithRed:(0/255.0) green:(105/255.0) blue:(229/255.0) alpha:1];
    self.exceptionlabel.hidden = FALSE;
}

- (void)ExceptionRised
{
    self.exceptionlabel.hidden = FALSE;
    self.readBtn.enabled = TRUE;
    self.clearBtn.enabled = TRUE;
    self.connectBtn.userInteractionEnabled = TRUE;
    self.connectBtn.titleLabel.textColor = [UIColor colorWithRed:(0/255.0) green:(105/255.0) blue:(229/255.0) alpha:1];
}

#pragma mark - UIToolbarButton Callbacks

- (IBAction)log:(id)sender
{
    [self presentViewController:self.logWindow animated:YES completion:nil];
}

@end
