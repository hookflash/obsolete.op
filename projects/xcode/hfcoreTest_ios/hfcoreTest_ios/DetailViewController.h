//
//  DetailViewController.h
//  hfcoreTest_ios
//
//  Created by Robin Raymond on 2/12/13.
//  Copyright (c) 2013 SMB Phone Inc. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface DetailViewController : UIViewController <UISplitViewControllerDelegate>

@property (strong, nonatomic) id detailItem;

@property (weak, nonatomic) IBOutlet UILabel *detailDescriptionLabel;
@end
