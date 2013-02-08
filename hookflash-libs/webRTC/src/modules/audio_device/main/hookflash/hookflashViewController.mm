//
//  hookflashViewController.m
//  hookflash
//
//  Created by hookflash on 11-08-29.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import "hookflashViewController.h"

#include "audio_device_utility.h"

using namespace webrtc;

char homeDirectory[256];

int hookflash_test_api(const char* fileName);
int hookflash_test_func(const char* fileName, int testID);

@implementation hookflashViewController


#pragma mark - Start Test(s)
-(IBAction)startTestAPI
{
    NSString* str = [NSHomeDirectory() stringByAppendingString:@"/Documents/audio_device_hookflash_test_api.txt"];
   
    const char* buffer = [str UTF8String];

    hookflash_test_api(buffer);
}

-(IBAction)startTest1
{
    const char* home = [NSHomeDirectory() UTF8String];
    
    strcpy(homeDirectory, home);
    
    NSString* str = [NSHomeDirectory() stringByAppendingString:@"/Documents/audio_device_hookflash_test_func_0.txt"];
    
    const char* buffer = [str UTF8String];
    
    hookflash_test_func(buffer, 0);
}

-(IBAction)startTest2
{
    const char* home = [NSHomeDirectory() UTF8String];
    
    strcpy(homeDirectory, home);
    
    NSString* str = [NSHomeDirectory() stringByAppendingString:@"/Documents/audio_device_hookflash_test_func_1.txt"];
    
    const char* buffer = [str UTF8String];
    
    hookflash_test_func(buffer, 1);
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


// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad
{
    [super viewDidLoad];
}


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
