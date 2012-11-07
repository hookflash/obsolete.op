//
//  zsLibTest_iosAppDelegate_iPad.m
//  zsLibTest_ios
//
//  Created by Vladimir Morosev on 11/18/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import "zsLibTest_iosAppDelegate_iPad.h"

#import "zsLib/Stringize.h"
#import "zsLib/zsTypes.h"

@implementation zsLibTest_iosAppDelegate_iPad

- (IBAction) btnTest_action: (id) sender
{
  zsLib::String s = zsLib::Stringize<int>(24);
}

- (void)dealloc
{
	[super dealloc];
}

@end
