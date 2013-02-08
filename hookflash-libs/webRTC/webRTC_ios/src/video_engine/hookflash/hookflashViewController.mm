//
//  hookflashViewController.m
//  hookflash
//
//  Created by Vladimir Morosev on 11-09-26.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import "hookflashViewController.h"
#import <UIKit/UIKit.h>

#include <sys/sysctl.h>

hookflashViewController* hookflash_view_controller;

int hookflash_test(const char* directory, int testID, int cameraIndex, 
                   const char* sendIpAddress, int sendPort, int receivePort,
                   IPhoneRenderView* view1, IPhoneRenderView* view2);
int hookflash_set_codec_size(int width, int height);
int hookflash_set_capture_rotation();
int hookflash_set_event_callback(void (*eventCallback)(const char*, void*));
int hookflash_get_average_system_cpu(int& systemCPU);
int hookflash_get_send_codec_statistics(unsigned int& keyFrames, unsigned int& deltaFrames);
int hookflash_get_receive_codec_statistics(unsigned int& keyFrames, unsigned int& deltaFrames);
int hookflash_get_received_rtcp_statistics(unsigned short& fractionLost,
                                           unsigned int& cumulativeLost, unsigned int& extendedMax,
                                           unsigned int& jitter, int& rttMs);
int hookflash_get_sent_rtcp_statistics(unsigned short& fractionLost,
                                       unsigned int& cumulativeLost,
                                       unsigned int& extendedMax,
                                       unsigned int& jitter,
                                       int& rttMs);
int hookflash_get_rtp_statistics(unsigned int& bytesSent,
                                 unsigned int& packetsSent,
                                 unsigned int& bytesReceived,
                                 unsigned int& packetsReceived);

void hookflash_event_callback(const char* eventType, void* data)
{
    [[NSAutoreleasePool alloc] init];
    
    NSString* eventTypeString = [[NSString stringWithFormat:@"%s", eventType] autorelease];
    if ([eventTypeString compare:@"CapturedFrameRate"] == NSOrderedSame)
    {
        [[hookflash_view_controller capturedFrameTextField] performSelectorOnMainThread:@selector(setText:)
                                  withObject:[NSString stringWithFormat:@"%d", *((char*)data)] waitUntilDone:NO];
    }
    else if ([eventTypeString compare:@"OutgoingRate"] == NSOrderedSame)
    {
      [[hookflash_view_controller outgoingCodecFramerateTextField] performSelectorOnMainThread:@selector(setText:)
                                  withObject:[NSString stringWithFormat:@"%d", ((unsigned int*)data)[0]] waitUntilDone:NO];
      [[hookflash_view_controller outgoingCodecBitrateTextField] performSelectorOnMainThread:@selector(setText:)
                                  withObject:[NSString stringWithFormat:@"%d", ((unsigned int*)data)[1]] waitUntilDone:NO];
    }
    else if ([eventTypeString compare:@"IncomingRate"] == NSOrderedSame)
    {
      [[hookflash_view_controller incomingCodecFramerateTextField] performSelectorOnMainThread:@selector(setText:)
                                  withObject:[NSString stringWithFormat:@"%d", ((unsigned int*)data)[0]] waitUntilDone:NO];
      [[hookflash_view_controller incomingCodecBitrateTextField] performSelectorOnMainThread:@selector(setText:)
                                  withObject:[NSString stringWithFormat:@"%d", ((unsigned int*)data)[1]] waitUntilDone:NO];
    }
}

@implementation hookflashViewController

@synthesize capturedFrameTextField=_capturedFrameTextField;
@synthesize outgoingCodecFramerateTextField=_outgoingCodecFramerateTextField;
@synthesize outgoingCodecBitrateTextField=_outgoingCodecBitrateTextField;
@synthesize incomingCodecFramerateTextField=_incomingCodecFramerateTextField;
@synthesize incomingCodecBitrateTextField=_incomingCodecBitrateTextField;

- (void) SendLastFrameToMainView:(UIImage*) image :(id) sender
{
    IPhoneRenderView* renderView = (IPhoneRenderView*) sender;
    if (renderView == _renderView1)
    {
        [_imgView1 setFrame:CGRectMake(_imgView1.frame.origin.x, _imgView1.frame.origin.y, image.size.width, image.size.height)];
        [_imgView1 performSelectorOnMainThread:@selector(setImage:)
                                    withObject:image waitUntilDone:NO];
    }
    else if (renderView == _renderView2)
    {
        [_imgView2 setFrame:CGRectMake(_imgView2.frame.origin.x, _imgView2.frame.origin.y, image.size.width, image.size.height)];
        [_imgView2 performSelectorOnMainThread:@selector(setImage:)
                                    withObject:image waitUntilDone:NO];
    }
}

- (void) SendLastFrameToSubView:(UIImage*) image :(id) sender
{
    IPhoneRenderView* renderView = (IPhoneRenderView*) sender;
    if (renderView == _renderView1)
    {
        [_imgView1 performSelectorOnMainThread:@selector(setImage:)
                                   withObject:image waitUntilDone:NO];
    }
    else if (renderView == _renderView2)
    {
        [_imgView2 performSelectorOnMainThread:@selector(setImage:)
                                    withObject:image waitUntilDone:NO];
    }
}

#pragma mark - Start Test(s)
-(IBAction)startTest1
{    
    NSString* documents = [NSHomeDirectory() stringByAppendingString:@"/Documents/"];
    
    const char* buffer = [documents UTF8String];
    const char* sendIPAddress = [sendIPAddressTextField.text UTF8String];
    int sendPort = [sendPortTextField.text intValue];
    int receivePort = [receivedPortTextField.text intValue];
//    _captureIndex = 0; // back camera
    _captureIndex = 1; // front camera

    hookflash_test(buffer, 0, _captureIndex, sendIPAddress, sendPort, receivePort, _renderView1, _renderView2);
}

-(IBAction)startTest2
{    
  hookflash_set_capture_rotation();
  
  hookflash_set_event_callback(hookflash_event_callback);
  
  timer = [NSTimer scheduledTimerWithTimeInterval:0.5
                                           target:self
                                         selector:@selector(refreshTransportStatistics)
                                         userInfo:nil
                                          repeats:YES];

/*
    NSString* documents = [NSHomeDirectory() stringByAppendingString:@"/Documents/"];
  
    hookflash_set_capture_rotation();

    const char* buffer = [documents UTF8String];
    const char* sendIPAddress = [sendIPAddressTextField.text UTF8String];
    int sendPort = [sendPortTextField.text intValue];
    int receivePort = [receivedPortTextField.text intValue];
    
    hookflash_test(buffer, 0, sendIPAddress, sendPort, receivePort, _renderView1, _renderView2, 320, 180);
*/
}

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
        _renderView1 = [[IPhoneRenderView alloc] init];
        _renderView1.viewId = 0;
        _renderView1.delegate = self;
        _renderView2 = [[IPhoneRenderView alloc] init];
        _renderView2.viewId = 0;
        _renderView2.delegate = self;
        self.title = @"Video Test";
      
        hookflash_view_controller = self;
    }

    int name[] = {CTL_HW, HW_MACHINE};
    size_t size;
    sysctl(name, 2, NULL, &size, NULL, 0);
    char* machine = (char*)malloc(size);
    sysctl(name, 2, machine, &size, NULL, 0);
    _machineName = [[NSString alloc] initWithCString:machine encoding: NSUTF8StringEncoding];
    _captureIndex = -1;
    free(machine);

    return self;
}

- (void) refreshTransportStatistics
{
    int systemCPU;
    unsigned int keyFrames;
    unsigned int deltaFrames;
    unsigned short fractionLost;
    unsigned int cumulativeLost; 
    unsigned int extendedMax;
    unsigned int jitter; 
    int rttMs;
    unsigned int bytesSent;
    unsigned int packetsSent;
    unsigned int bytesReceived;
    unsigned int packetsReceived;
  
    hookflash_get_average_system_cpu(systemCPU);
    [cpuLoadTextField setText:[NSString stringWithFormat:@"%d", systemCPU]];
  
    hookflash_get_send_codec_statistics(keyFrames, deltaFrames);
    [sendCodecKeyFrameTextField setText:[NSString stringWithFormat:@"%d", keyFrames]];
    [sendCodecDeltaFrameTextField setText:[NSString stringWithFormat:@"%d", deltaFrames]];
  
    hookflash_get_receive_codec_statistics(keyFrames, deltaFrames);
    [receiveCodecKeyFrameTextField setText:[NSString stringWithFormat:@"%d", keyFrames]];
    [receiveCodecDeltaFrameTextField setText:[NSString stringWithFormat:@"%d", deltaFrames]];
  
    hookflash_get_received_rtcp_statistics(fractionLost, cumulativeLost, extendedMax,
                                           jitter, rttMs);
    [receiveFractionLostTextField setText:[NSString stringWithFormat:@"%d", fractionLost]];
    [receiveCumulativeLostTextField setText:[NSString stringWithFormat:@"%d", cumulativeLost]];
    [receiveJitterTextField setText:[NSString stringWithFormat:@"%d", jitter]];
    [receiveRttTextField setText:[NSString stringWithFormat:@"%d", rttMs]];
  
    hookflash_get_sent_rtcp_statistics(fractionLost, cumulativeLost, extendedMax,
                                       jitter, rttMs);
    [sendFractionLostTextField setText:[NSString stringWithFormat:@"%d", fractionLost]];
    [sendCumulativeLostTextField setText:[NSString stringWithFormat:@"%d", cumulativeLost]];
    [sendJitterTextField setText:[NSString stringWithFormat:@"%d", jitter]];
    [sendRttTextField setText:[NSString stringWithFormat:@"%d", rttMs]];
  
    hookflash_get_rtp_statistics(bytesSent, packetsSent, bytesReceived, packetsReceived);
    [sentPacketTextField setText:[NSString stringWithFormat:@"%d", packetsSent]];
    [receivedPacketTextField setText:[NSString stringWithFormat:@"%d", packetsReceived]];
}

- (void)dealloc
{
    [timer invalidate];
    timer = nil;
  
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
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(orientationChanged) name:@"orientationChanged" object:nil];
    [[UIApplication sharedApplication] setIdleTimerDisabled: YES];
}

- (void)viewDidUnload
{
    [super viewDidUnload];
    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
    [[NSNotificationCenter defaultCenter] removeObserver:self name:@"orientationChanged" object:nil];
}

- (void)orientationChanged
{
  UIDeviceOrientation uido = [[UIDevice currentDevice] orientation];
  switch (uido)
  {
    case UIDeviceOrientationLandscapeLeft:
    case UIDeviceOrientationLandscapeRight:
    case UIDeviceOrientationUnknown:
    case UIDeviceOrientationFaceUp:
    case UIDeviceOrientationFaceDown:
        if (_captureIndex == 0) {
            if ([_machineName hasPrefix:@"iPhone"])
                hookflash_set_codec_size(160, 90);
            else if ([_machineName hasPrefix:@"iPod"])
                hookflash_set_codec_size(160, 90);
            else if ([_machineName hasPrefix:@"iPad2"])
                hookflash_set_codec_size(320, 180);
            else if ([_machineName hasPrefix:@"iPad3"])
                hookflash_set_codec_size(480, 270);
        } else if (_captureIndex == 1) {
            if ([_machineName hasPrefix:@"iPhone"])
                hookflash_set_codec_size(160, 120);
            else if ([_machineName hasPrefix:@"iPod"])
                hookflash_set_codec_size(160, 120);
            else if ([_machineName hasPrefix:@"iPad"])
                hookflash_set_codec_size(320, 240);
        }
        break;
    case UIDeviceOrientationPortraitUpsideDown:
    case UIDeviceOrientationPortrait:
        if (_captureIndex == 0) {
            if ([_machineName hasPrefix:@"iPhone"])
                hookflash_set_codec_size(90, 160);
            else if ([_machineName hasPrefix:@"iPod"])
                hookflash_set_codec_size(90, 160);
            else if ([_machineName hasPrefix:@"iPad2"])
                hookflash_set_codec_size(180, 320);
            else if ([_machineName hasPrefix:@"iPad3"])
              hookflash_set_codec_size(270, 480);
        } else if (_captureIndex == 1) {
            if ([_machineName hasPrefix:@"iPhone"])
                hookflash_set_codec_size(120, 160);
            else if ([_machineName hasPrefix:@"iPod"])
                hookflash_set_codec_size(120, 160);
            else if ([_machineName hasPrefix:@"iPad"])
                hookflash_set_codec_size(240, 320);
        }
        break;
  }
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    return YES;
}

-(void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration
{
  [[NSNotificationCenter defaultCenter] postNotificationName:@"orientationChanged" object:nil];
}

@end
