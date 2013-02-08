/*
 
 Copyright (c) 2013, SMB Phone Inc.
 All rights reserved.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
 
 1. Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 
 The views and conclusions contained in the software and documentation are those
 of the authors and should not be interpreted as representing official policies,
 either expressed or implied, of the FreeBSD Project.
 
 */

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
