//
//  ViewController.h
//  hfmediatest_ios
//
//  Created by Vladimir Morosev on 9/12/12.
//
//

#import <UIKit/UIKit.h>
#include <hookflash/hookflashTypes.h>
#include <hookflash/internal/hookflash_MediaEngine.h>

@interface ViewController : UIViewController
{
    IBOutlet UIButton* _btnStartTest1;
    IBOutlet UIButton* _btnStartTest2;
    IBOutlet UIButton* _btnStartTest3;
    IBOutlet UIButton* _btnStartTest4;
    IBOutlet UIImageView* _imgView1;
    IBOutlet UIImageView* _imgView2;
  
    IBOutlet UITextField* receiverIPAddressTextField;
  
    hookflash::IMediaEngineDelegatePtr mediaEngineDelegatePtr;
}

-(IBAction)startTest1;
-(IBAction)startTest2;
-(IBAction)startTest3;
-(IBAction)startTest4;

@end
