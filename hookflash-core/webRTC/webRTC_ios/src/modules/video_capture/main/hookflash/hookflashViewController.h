#import <UIKit/UIKit.h>
//#import "iphone_render_view.h"

@interface hookflashViewController : UIViewController //<VideoRendererDelegate>
{
    IBOutlet UIButton* _btnStartTest;
    IBOutlet UIImageView* _imgView;
    
    IPhoneRenderView* _renderView;
}

-(IBAction)startTest;

@end
