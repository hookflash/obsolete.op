//
//  hfStackTest_iosAppDelegate_iPad.m
//  hfStackTest_ios
//
//  Created by hookflash on 11-11-18.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import "hfStackTest_iosAppDelegate_iPad.h"

#import <hookflash/stack/hookflashTypes.h>
#import <hookflash/stack/message/hookflashTypes.h>
#import "zsLib/Stringize.h"
#import "zsLib/XML.h"
#import "zsLib/zsTypes.h"
#import <hookflash/services/IDNS.h>
#import <hookflash/stack/message/Message.h>
#import <hookflash/stack/IHelper.h>

using namespace zsLib::XML;
using zsLib::XML::Document;
using namespace hookflash::stack;

@implementation hfStackTest_iosAppDelegate_iPad

- (IBAction) btnTest_action: (id) sender
{
    zsLib::String s = zsLib::Stringize<int>(24);
    hookflash::services::IDNSQueryPtr query;
    query = hookflash::services::IDNS::lookupA(hookflash::services::IDNSDelegatePtr(), "www.hookflash.com");

    zsLib::String result = ""
    "<result xmlns=\"http://www.hookflash.com/openpeer/1.0/message\" id=\"abc123\" method=\"message-expire\" epoch=\"438483284\">"
    "</result>";

    zsLib::XML::DocumentPtr xmlDoc = Document::create();
    xmlDoc->parse(result);

    //const char * test = hookflash::stack::IHelper::randomString(7);

    //hookflash::stack::message::MessagePtr msg_p2fME_res = hookflash::stack::message::Message::create(xmlDoc);
}

- (void)dealloc
{
	[super dealloc];
}

@end
