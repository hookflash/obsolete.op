//
//  hookflashViewController.h
//  hookflash
//
//  Created by hookflash on 11-08-29.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface hookflashViewController : UIViewController 
{

    IBOutlet UIButton *btnStartTestApi;
    IBOutlet UIButton *btnStartTest1;
    IBOutlet UIButton *btnStartTest2;
}

-(IBAction)startTestAPI;
-(IBAction)startTest1;
-(IBAction)startTest2;

@end
