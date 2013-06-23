//
//  MasterViewController.h
//  hfcoreTest_ios
//
//  Created by Robin Raymond on 2/12/13.
//  Copyright (c) 2013 SMB Phone Inc. All rights reserved.
//

#import <UIKit/UIKit.h>

@class DetailViewController;

@interface MasterViewController : UITableViewController

@property (strong, nonatomic) DetailViewController *detailViewController;

@end
