/*
 *  Created by Robin Raymond.
 *  Copyright 2009-2013. Robin Raymond. All rights reserved.
 *
 * This file is part of zsLib.
 *
 * zsLib is free software; you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License (LGPL) as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * zsLib is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with zsLib; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 *
 */

#import "zsLibTest_iosAppDelegate_iPad.h"

#import "zsLib/Stringize.h"
#import "zsLib/types.h"

#import "boost_replacement.h"

@implementation zsLibTest_iosAppDelegate_iPad

- (IBAction) btnTest_action: (id) sender
{
  zsLib::String s = zsLib::Stringize<int>(24);

  BoostReplacement::output();
}

- (void)dealloc
{
	[super dealloc];
}

@end
