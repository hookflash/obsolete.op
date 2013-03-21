//
//  ViewController.m
//  hfmediatest_ios
//
//  Created by Vladimir Morosev on 9/12/12.
//
//

#import "ViewController.h"
#import "MediaEngineDelegateWrapper.h"
#include <hookflash/IClient.h>

@implementation ViewController

-(IBAction)startTest1
{
    hookflash::internal::IMediaEngineForTestApplicationPtr mediaEngine =
        hookflash::internal::IMediaEngineForTestApplication::singleton();
  
    mediaEngine->setCaptureRenderView(_imgView1);
    mediaEngine->setChannelRenderView(_imgView2);
    
    NSString* documents = [NSHomeDirectory() stringByAppendingString:@"/Documents/test.mp4"];
    
    const char* buffer = [documents UTF8String];

    mediaEngine->startVideoCapture();
    mediaEngine->startRecordVideoCapture(buffer);
}

-(IBAction)startTest2
{
    hookflash::internal::IMediaEngineForTestApplicationPtr mediaEngine =
        hookflash::internal::IMediaEngineForTestApplication::singleton();
  
    mediaEngine->setReceiverAddress("127.0.0.1");
  
    mediaEngine->startVoice();
    mediaEngine->startVideoChannel();
}

-(IBAction)startTest3
{
    hookflash::internal::IMediaEngineForTestApplicationPtr mediaEngine =
        hookflash::internal::IMediaEngineForTestApplication::singleton();
    
    mediaEngine->stopVoice();
    mediaEngine->stopVideoChannel();
}

-(IBAction)startTest4
{
    hookflash::internal::IMediaEngineForTestApplicationPtr mediaEngine =
        hookflash::internal::IMediaEngineForTestApplication::singleton();
  
    mediaEngine->stopRecordVideoCapture();
    mediaEngine->stopVideoCapture();
}

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self)
        self.title = @"Video Test";
  
    //NSString* documents = [NSHomeDirectory() stringByAppendingString:@"/Documents/"];
    //const char* buffer = [documents UTF8String];
    //const char* receiverIPAddress = [receiverIPAddressTextField.text UTF8String];
    //const char* receiverIPAddress = "127.0.0.1";
    
    IClient::setup();
    
    mediaEngineDelegatePtr = MediaEngineDelegateWrapper::create();
    
//    IClient::setLogLevel(IClient::Log::Trace);
//    IClient::setLogLevel("hookflash_services", IClient::Log::Debug);      // recommend Debug
//    IClient::setLogLevel("hookflash_media", IClient::Log::Debug);         // recommend Debug
//    IClient::installTelnetLogger(59999, 60, true);
  
    hookflash::internal::IMediaEngineForTestApplication::setup(mediaEngineDelegatePtr);
    
    hookflash::internal::IMediaEngineForTestApplicationPtr mediaEngine =
      hookflash::internal::IMediaEngineForTestApplication::singleton();
    
    mediaEngine->setEcEnabled(true);
    mediaEngine->setAgcEnabled(true);
    mediaEngine->setNsEnabled(false);
    mediaEngine->setMuteEnabled(false);
    mediaEngine->setLoudspeakerEnabled(false);
    mediaEngine->setContinuousVideoCapture(true);
    mediaEngine->setDefaultVideoOrientation(hookflash::IMediaEngine::VideoOrientation_Portrait);
    mediaEngine->setRecordVideoOrientation(hookflash::IMediaEngine::VideoOrientation_LandscapeRight);
    mediaEngine->setFaceDetection(false);
  
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(orientationChanged) name:@"orientationChanged" object:nil];
    [[UIApplication sharedApplication] setIdleTimerDisabled: YES];
  
    [_imgView1 addObserver:self forKeyPath:@"image"
                   options:(NSKeyValueObservingOptionNew | NSKeyValueObservingOptionOld)
                   context:NULL];
    [_imgView2 addObserver:self forKeyPath:@"image"
                   options:(NSKeyValueObservingOptionNew | NSKeyValueObservingOptionOld)
                   context:NULL];
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change
                       context:(void *)context
{
    if (object == _imgView1 && [keyPath isEqualToString:@"image"])
    {
        UIImage* image = [change objectForKey:NSKeyValueChangeNewKey];
        [_imgView1 setFrame:CGRectMake(_imgView1.frame.origin.x, _imgView1.frame.origin.y, image.size.width, image.size.height)];
    }
    else if (object == _imgView2 && [keyPath isEqualToString:@"image"])
    {
        UIImage* image = [change objectForKey:NSKeyValueChangeNewKey];
        [_imgView2 setFrame:CGRectMake(_imgView2.frame.origin.x, _imgView2.frame.origin.y, image.size.width, image.size.height)];
    }
}

- (void)viewDidUnload
{
    [super viewDidUnload];
    [[NSNotificationCenter defaultCenter] removeObserver:self name:@"orientationChanged" object:nil];
    [_imgView1 removeObserver:self forKeyPath:@"image"];
    [_imgView2 removeObserver:self forKeyPath:@"image"];
}

- (void)orientationChanged
{
    hookflash::internal::IMediaEngineForTestApplicationPtr mediaEngine =
        hookflash::internal::IMediaEngineForTestApplication::singleton();
  
    mediaEngine->setVideoOrientation();
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
