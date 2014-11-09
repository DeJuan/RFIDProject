/**
 *  @file LogController.m
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

#import "LogController.h"

@implementation LogController

@synthesize logWindow;

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self)
    {
        NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
        NSString *log = [[paths objectAtIndex:0] stringByAppendingPathComponent: @"ns.log"];	
        NSFileHandle *fh = [NSFileHandle fileHandleForReadingAtPath:log];
        
        //Read the existing logs, I opted not to do this.
        
        //Seek to end of file so that logs from previous application launch are not visible
        [fh seekToEndOfFile];
        
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(getData:)
                                                     name:@"NSFileHandleReadCompletionNotification"
                                                   object:fh];
        [fh readInBackgroundAndNotify];
        firstOpen = YES;
    }
    return self;
}

- (void)dealloc
{
    [logWindow release];
    [super dealloc];
}

- (void)didReceiveMemoryWarning
{
    logWindow.text = @"";
    
    [super didReceiveMemoryWarning];
}

#pragma mark - View lifecycle

- (void)viewDidLoad
{
    [super viewDidLoad];
}

- (void)viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *log = [[paths objectAtIndex:0] stringByAppendingPathComponent: @"ns.log"];
    if ( firstOpen )
    {
        NSError *error;
        NSString* content = [NSString stringWithContentsOfFile:log encoding:NSUTF8StringEncoding error:&error];
        logWindow.editable = TRUE;
		logWindow.text = [logWindow.text stringByAppendingString: content];
		logWindow.editable = FALSE;
        firstOpen = NO;
    } 
}

- (void)viewDidUnload
{
    [self setLogWindow:nil];
    [super viewDidUnload];
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    
	return YES;
}

#pragma mark - NSLog Redirection Methods

- (void) getData: (NSNotification *)aNotification
{
    NSData *data = [[aNotification userInfo] objectForKey:NSFileHandleNotificationDataItem];
    // If the length of the data is zero
    if ([data length])
    {
        // Send the data on to the controller
        NSString *aString = [[[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding] autorelease];
		
		logWindow.editable = TRUE;
		logWindow.text = [logWindow.text stringByAppendingString: aString];
		logWindow.editable = FALSE;
        
		//Scroll to the last line
		[self setWindowScrollToVisible];
        
		// we need to schedule the file handle go read more data in the background again.
		[[aNotification object] readInBackgroundAndNotify];
    }
	else
	{
		//I use a delay to minimize CPU usage when the file has not changed.
		[self performSelector:@selector(refreshLog:) withObject:aNotification afterDelay:1.0];
	}
}
- (void) refreshLog: (NSNotification *)aNotification
{
	[[aNotification object] readInBackgroundAndNotify];
}

-(void)setWindowScrollToVisible
{
	NSRange txtOutputRange;
	txtOutputRange.location = [[logWindow text] length];
	txtOutputRange.length = 0;
    logWindow.editable = TRUE;
	[logWindow scrollRangeToVisible:txtOutputRange];
	[logWindow setSelectedRange:txtOutputRange];
    logWindow.editable = FALSE;
}

#pragma mark - UIBarButtonItem Callbacks

- (IBAction)clear:(id)sender
{
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *log = [[paths objectAtIndex:0] stringByAppendingPathComponent: @"ns.log"];

    [[NSData data] writeToFile:log options:NSDataWritingAtomic error:NULL];
    
    logWindow.text = @"";
}

-(IBAction)done:(id)sender
{
    [self dismissViewControllerAnimated:YES completion:nil];
    
}

@end

