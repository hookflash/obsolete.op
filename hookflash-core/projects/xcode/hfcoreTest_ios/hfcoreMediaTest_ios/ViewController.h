//
//  ViewController.h
//  hfmediatest_ios
//
//  Created by Vladimir Morosev on 9/12/12.
//
//

#import <UIKit/UIKit.h>
#include <hookflash/core/internal/core_MediaEngine.h>

@interface ViewController : UIViewController
{
    IBOutlet UIButton* _btnStartTest1;
    IBOutlet UIButton* _btnStartTest2;
    IBOutlet UIImageView* _imgView1;
    IBOutlet UIImageView* _imgView2;
  
    IBOutlet UITextField* receiverIPAddressTextField;
  
    hookflash::core::IMediaEngineDelegatePtr mediaEngineDelegatePtr;
}

-(IBAction)startTest1;
-(IBAction)startTest2;

@end
