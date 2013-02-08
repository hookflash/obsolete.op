//
//  ViewController.m
//  hfmediatest_ios
//
//  Created by Vladimir Morosev on 9/12/12.
//
//

#import "ViewController.h"
#import "MediaEngineDelegateWrapper.h"
//#include <hookflash/IClient.h>

@implementation ViewController

-(IBAction)startTest1
{
  //NSString* documents = [NSHomeDirectory() stringByAppendingString:@"/Documents/"];
  //const char* buffer = [documents UTF8String];
  //const char* receiverIPAddress = [receiverIPAddressTextField.text UTF8String];
  //const char* receiverIPAddress = "127.0.0.1";
  
//  IClient::setup();
  
  mediaEngineDelegatePtr = MediaEngineDelegateWrapper::create();
  
  //    IClient::setLogLevel(IClient::Log::Trace);
  //    IClient::setLogLevel("hookflash_services", IClient::Log::Debug);      // recommend Debug
  //    IClient::setLogLevel("hookflash_media", IClient::Log::Debug);         // recommend Debug
  //    IClient::installTelnetLogger(59999, 60, true);
  
  hookflash::core::internal::IMediaEngineForTestApplication::setup(mediaEngineDelegatePtr);
  
  hookflash::core::internal::IMediaEngineForTestApplication& mediaEngine =
    hookflash::core::internal::IMediaEngineForTestApplication::singleton()->forTestApplication();
  
  mediaEngine.setCaptureRenderView(_imgView1);
  mediaEngine.setChannelRenderView(_imgView2);
  
  mediaEngine.setEcEnabled(true);
  mediaEngine.setAgcEnabled(true);
  mediaEngine.setNsEnabled(false);
  mediaEngine.setMuteEnabled(false);
  mediaEngine.setLoudspeakerEnabled(false);
  
  mediaEngine.setReceiverAddress("127.0.0.1");
  
  mediaEngine.startVoice();
  mediaEngine.startVideo();
}

-(IBAction)startTest2
{
  hookflash::core::internal::IMediaEngineForTestApplication& mediaEngine =
    hookflash::core::internal::IMediaEngineForTestApplication::singleton()->forTestApplication();
  
  //    mediaEngine->setVideoOrientation();
  
  mediaEngine.stopVoice();
  mediaEngine.stopVideo();
}

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
  self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
  if (self)
    self.title = @"Video Test";
  
  return self;
}

- (void)viewDidLoad
{
  [super viewDidLoad];
  [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(orientationChanged) name:@"orientationChanged" object:nil];
  [[UIApplication sharedApplication] setIdleTimerDisabled: YES];
}

- (void)viewDidUnload
{
  [super viewDidUnload];
  [[NSNotificationCenter defaultCenter] removeObserver:self name:@"orientationChanged" object:nil];
}

- (void)orientationChanged
{
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
