//
//  AppViewController.h
//  hfcoretest
//
//  Created by Vladimir Morosev on 9/10/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "FakeGUIViewController.h"



@interface AppViewController : UIViewController 
{
  IBOutlet UIButton *startButton;
  IBOutlet FakeGUIViewController * fakeGUIViewController;
}


- (IBAction) startApp: (id) sender;
	
@end
