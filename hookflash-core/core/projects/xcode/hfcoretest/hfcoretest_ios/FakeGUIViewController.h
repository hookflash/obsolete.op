//
//  FakeGUIViewController.h
//  hfcoretest
//
//  Created by Vladimir Morosev on 9/10/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#pragma once

#import <UIKit/UIKit.h>


@protocol ClientLogDelegateObjC <NSObject>

- (void) OnNewSubsystem:(NSObject*) object;
- (void) OnLog:(NSObject*) object;

@end



@interface FakeGUIViewController : UIViewController <ClientLogDelegateObjC>
{
    
}

@end
