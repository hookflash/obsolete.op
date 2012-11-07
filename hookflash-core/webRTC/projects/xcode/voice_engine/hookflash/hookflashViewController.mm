//
//  hookflashViewController.m
//  hookflash
//
//  Created by Vladimir Morosev on 11-10-08.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import "hookflashViewController.h"

namespace voetest{
int hookflash_test(const char* directory, int testID, const char* sndIpAddress, int sndPort, int rcvPort);
}

@implementation hookflashViewController

#pragma mark - Start Test(s)
-(IBAction)startTest
{
    NSString* documents = [NSHomeDirectory() stringByAppendingString:@"/Documents"];
    
    const char* buffer = [documents UTF8String];
    const char* sendIPAddress = [sendIPAddressTextField.text UTF8String];
    int sendPort = [sendPortTextField.text intValue];
    int receivePort = [receivedPortTextField.text intValue];

    voetest::hookflash_test(buffer, 0, sendIPAddress, sendPort, receivePort);
}

- (void)dealloc
{
    [super dealloc];
}

- (void)didReceiveMemoryWarning
{
    // Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
    
    // Release any cached data, images, etc that aren't in use.
}

#pragma mark - View lifecycle

/*
// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad
{
    [super viewDidLoad];
}
*/

- (void)viewDidUnload
{
    [super viewDidUnload];
    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    // Return YES for supported orientations
    return YES;
}

@end
