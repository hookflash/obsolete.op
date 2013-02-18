//
//  ViewController.m
//  hfmediatest_ios
//
//  Created by Vladimir Morosev on 9/12/12.
//
//

#import "ViewController.h"
#import "MediaEngineDelegateWrapper.h"
#include <hookflash/core/internal/core_MediaEngine.h>
#include <hookflash/core/test/TestMediaEngine.h>

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

  hookflash::core::test::TestMediaEngineFactoryPtr overrideFactory(new hookflash::core::test::TestMediaEngineFactory);
  
  hookflash::core::internal::Factory::override(overrideFactory);

  hookflash::core::internal::IMediaEngineForStack::setup(mediaEngineDelegatePtr);
  
  hookflash::core::internal::MediaEnginePtr mediaEngineInternal = hookflash::core::internal::IMediaEngineForCallTransport::singleton();
  hookflash::core::test::TestMediaEnginePtr testMediaEngineInternal = boost::dynamic_pointer_cast<hookflash::core::test::TestMediaEngine>(mediaEngineInternal);
  hookflash::core::IMediaEnginePtr mediaEngine = hookflash::core::IMediaEngine::singleton();
  
  mediaEngine->setCaptureRenderView((__bridge void*)_imgView1);
  mediaEngine->setChannelRenderView((__bridge void*)_imgView2);
  
  mediaEngine->setEcEnabled(true);
  mediaEngine->setAgcEnabled(true);
  mediaEngine->setNsEnabled(false);
  mediaEngine->setMuteEnabled(false);
  mediaEngine->setLoudspeakerEnabled(false);
  
  testMediaEngineInternal->setReceiverAddress("127.0.0.1");
  
  mediaEngineInternal->forCallTransport().startVoice();
  mediaEngineInternal->forCallTransport().startVideo();
}

-(IBAction)startTest2
{
  hookflash::core::internal::MediaEnginePtr mediaEngineInternal = hookflash::core::internal::IMediaEngineForCallTransport::singleton();
  hookflash::core::IMediaEnginePtr mediaEngine = hookflash::core::IMediaEngine::singleton();
  
  mediaEngine->setVideoOrientation();
  
  mediaEngineInternal->forCallTransport().stopVoice();
  mediaEngineInternal->forCallTransport().stopVideo();
}

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
  self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
  if (self)
    self.title = @"Media Engine Test";
  
  return self;
}

- (void)viewDidLoad
{
  [super viewDidLoad];
  [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(orientationChanged)
                                               name:@"orientationChanged" object:nil];
  [[UIApplication sharedApplication] setIdleTimerDisabled: YES];
  
  [_imgView1 addObserver:self forKeyPath:@"image"
                 options:(NSKeyValueObservingOptionNew | NSKeyValueObservingOptionOld)
                 context:NULL];
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change
                       context:(void *)context
{
  if (object == _imgView1 && [keyPath isEqualToString:@"image"])
  {
    UIImage* image = [change objectForKey:NSKeyValueChangeNewKey];
  }
}

- (void)viewDidUnload
{
  [super viewDidUnload];
  [[NSNotificationCenter defaultCenter] removeObserver:self name:@"orientationChanged" object:nil];
  [_imgView1 removeObserver:self forKeyPath:@"image"];
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
