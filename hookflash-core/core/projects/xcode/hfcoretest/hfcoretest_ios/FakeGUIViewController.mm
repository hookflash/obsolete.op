//
//  FakeGUIViewController.mm
//  hfcoretest
//
//  Created by Vladimir Morosev on 9/10/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import "FakeGUIViewController.h"

#include <hookflash/hookflashTypes.h>
#include <hookflash/IClient.h>

using namespace hookflash;

IClientLogDelegatePtr createWrapper (id<ClientLogDelegateObjC> del);

@implementation FakeGUIViewController

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
    }
    return self;
}

-(id)init
{
  self = [super init];
  if (self) {
    
  }
  
  return self;
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

- (void)viewDidLoad
{
    [super viewDidLoad];
    // Do any additional setup after loading the view from its nib.
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
    return (interfaceOrientation == UIInterfaceOrientationPortrait);
}

- (void)OnLog:(NSObject*)object
{
  NSLog(@"1");
}

- (void)OnNewSubsystem:(NSObject*)object
{
  NSLog(@"2");
}

@end



struct ClientLogDelegateWrapper : public IClientLogDelegate
{
  typedef boost::shared_ptr<ClientLogDelegateWrapper> ClientLogDelegateWrapperPtr;
  
  id<ClientLogDelegateObjC> delegate;
  
  static ClientLogDelegateWrapperPtr create(){return ClientLogDelegateWrapperPtr(new ClientLogDelegateWrapper());}
  
  virtual void onNewSubsystem(
                              zsLib::PTRNUMBER subsystemID,
                              const char *subsystemName
                              )
  {
    NSString * string = @"string 1";
    [delegate OnLog:string];
  }
  
  virtual void onLog(
                     zsLib::PTRNUMBER subsystemID,
                     const char *subsystemName,
                     IClient::Log::Severity inSeverity,
                     IClient::Log::Level inLevel,
                     const char *inMessage,
                     const char *inFunction,
                     const char *inFilePath,
                     zsLib::ULONG inLineNumber
                     )
  {
    NSString * string = @"string 2";
    [delegate OnNewSubsystem:string];
  }
  
};
typedef boost::shared_ptr<ClientLogDelegateWrapper> ClientLogDelegateWrapperPtr;

IClientLogDelegatePtr createWrapper (id<ClientLogDelegateObjC> del) 
{
  ClientLogDelegateWrapperPtr ret = ClientLogDelegateWrapper::create();
  ret->delegate = del;
  return ret;
}
