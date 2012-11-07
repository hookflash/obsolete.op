//
//  hookflashViewController.h
//  hookflash
//
//  Created by Vladimir Morosev on 11-10-08.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface hookflashViewController : UIViewController {
    
    IBOutlet UIButton *btnStartTest;
  
    IBOutlet UITextField *sendIPAddressTextField;
    IBOutlet UITextField *sendPortTextField;
    IBOutlet UITextField *receivedPortTextField;

}

-(IBAction)startTest;

@end
