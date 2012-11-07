//
//  hookflashAppDelegate.h
//  hookflash
//
//  Created by Vladimir Morosev on 11-09-26.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import <UIKit/UIKit.h>

@class hookflashViewController;

@interface hookflashAppDelegate : NSObject <UIApplicationDelegate> {

}

@property (nonatomic, retain) IBOutlet UIWindow *window;

@property (nonatomic, retain) IBOutlet hookflashViewController *viewController;

@end
