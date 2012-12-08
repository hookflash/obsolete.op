//
//  hookflashViewController.h
//  hookflash
//
//  Created by Vladimir Morosev on 11-09-26.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface hookflashViewController : UIViewController
{
    IBOutlet UIButton* _btnStartTest1;
    IBOutlet UIButton* _btnStartTest2;
    IBOutlet UIImageView* _imgView1;
    IBOutlet UIImageView* _imgView2;
  
    IBOutlet UITextField *sendIPAddressTextField;
    IBOutlet UITextField *sendPortTextField;
    IBOutlet UITextField *receivedPortTextField;
    IBOutlet UITextField *cpuLoadTextField;
    IBOutlet UITextField *sendCodecKeyFrameTextField;
    IBOutlet UITextField *sendCodecDeltaFrameTextField;
    IBOutlet UITextField *receiveCodecKeyFrameTextField;
    IBOutlet UITextField *receiveCodecDeltaFrameTextField;
    IBOutlet UITextField *sendFractionLostTextField;
    IBOutlet UITextField *sendCumulativeLostTextField;
    IBOutlet UITextField *sendJitterTextField;
    IBOutlet UITextField *sendRttTextField;
    IBOutlet UITextField *receiveFractionLostTextField;
    IBOutlet UITextField *receiveCumulativeLostTextField;
    IBOutlet UITextField *receiveJitterTextField;
    IBOutlet UITextField *receiveRttTextField;
    IBOutlet UITextField *sentPacketTextField;
    IBOutlet UITextField *receivedPacketTextField;
    
    IPhoneRenderView* _renderView1;
    IPhoneRenderView* _renderView2;
  
    NSString* _machineName;
    NSInteger _captureIndex;

    NSTimer *timer;
}

@property (nonatomic, retain) IBOutlet UITextField *capturedFrameTextField;
@property (nonatomic, retain) IBOutlet UITextField *outgoingCodecFramerateTextField;
@property (nonatomic, retain) IBOutlet UITextField *outgoingCodecBitrateTextField;
@property (nonatomic, retain) IBOutlet UITextField *incomingCodecFramerateTextField;
@property (nonatomic, retain) IBOutlet UITextField *incomingCodecBitrateTextField;

-(IBAction)startTest1;
-(IBAction)startTest2;

@end
