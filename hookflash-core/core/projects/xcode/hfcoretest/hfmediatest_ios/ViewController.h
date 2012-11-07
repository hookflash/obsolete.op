//
//  ViewController.h
//  hfmediatest_ios
//
//  Created by Vladimir Morosev on 9/12/12.
//
//

#import <UIKit/UIKit.h>
#include "iphone_render_view.h"
#include <hookflash/hookflashTypes.h>
#include <hookflash/internal/hookflash_MediaEngine.h>

@interface ViewController : UIViewController <VideoRendererDelegate> 
{
    IBOutlet UIButton* _btnStartTest1;
    IBOutlet UIButton* _btnStartTest2;
    IBOutlet UIImageView* _imgView1;
    IBOutlet UIImageView* _imgView2;
  
    IPhoneRenderView* _renderView1;
    IPhoneRenderView* _renderView2;
  
    IBOutlet UITextField* receiverIPAddressTextField;
  
    hookflash::IMediaEngineDelegatePtr mediaEngineDelegatePtr;
}

-(IBAction)startTest1;
-(IBAction)startTest2;

@end
