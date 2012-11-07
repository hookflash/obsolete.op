//
//  hookflashAppDelegate.h
//  hookflash
//
//  Created by hookflash on 11-08-29.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import <UIKit/UIKit.h>

@class hookflashViewController;

@interface hookflashAppDelegate : NSObject <UIApplicationDelegate> {

}

@property (nonatomic, retain) IBOutlet UIWindow *window;

@property (nonatomic, retain) IBOutlet hookflashViewController *viewController;

@end
