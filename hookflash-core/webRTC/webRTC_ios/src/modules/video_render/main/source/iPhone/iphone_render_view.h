/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

//
//  cocoa_render_view.h 
//

#ifndef WEBRTC_MODULES_VIDEO_RENDER_MAIN_SOURCE_IPHONE_RENDER_VIEW_H_
#define WEBRTC_MODULES_VIDEO_RENDER_MAIN_SOURCE_IPHONE_RENDER_VIEW_H_

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

@protocol VideoRendererDelegate <NSObject>

- (void) SendLastFrameToMainView:(UIImage*) image :(id) sender;
- (void) SendLastFrameToSubView:(UIImage*) image :(id) sender;

@end


@interface IPhoneRenderView : NSObject{
@public
    id<VideoRendererDelegate> delegate;
@private
   
    int viewId;
    size_t frameWidth;
    size_t frameHeight;
    size_t frameBytesPerRow;
}

@property (assign) id<VideoRendererDelegate> delegate;
@property (assign) int viewId;
@property (assign) size_t frameWidth;
@property (assign) size_t frameHeight;
@property (assign) size_t frameBytesPerRow;
@end


#endif  // WEBRTC_MODULES_VIDEO_RENDER_MAIN_SOURCE_IPHONE_RENDER_VIEW_H_
