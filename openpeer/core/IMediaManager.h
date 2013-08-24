/*
 
 Copyright (c) 2013, SMB Phone Inc.
 All rights reserved.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
 
 1. Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 
 The views and conclusions contained in the software and documentation are those
 of the authors and should not be interpreted as representing official policies,
 either expressed or implied, of the FreeBSD Project.
 
 */

#pragma once

#include <openpeer/core/types.h>

namespace openpeer
{
  namespace core
  {
    interaction IMediaManager
    {
      enum VideoOrientations
      {
        VideoOrientation_LandscapeLeft,
        VideoOrientation_PortraitUpsideDown,
        VideoOrientation_LandscapeRight,
        VideoOrientation_Portrait
      };
      
      static const char *toString(VideoOrientations orientation);
      
      enum OutputAudioRoutes
      {
        OutputAudioRoute_Headphone,
        OutputAudioRoute_BuiltInReceiver,
        OutputAudioRoute_BuiltInSpeaker
      };
      
      static const char *toString(OutputAudioRoutes route);
      
      static IMediaManagerPtr singleton();
      
      virtual void setDefaultVideoOrientation(VideoOrientations orientation) = 0;
      virtual VideoOrientations getDefaultVideoOrientation() = 0;
      virtual void setRecordVideoOrientation(VideoOrientations orientation) = 0;
      virtual VideoOrientations getRecordVideoOrientation() = 0;
      virtual void setVideoOrientation() = 0;
      
      virtual void setMuteEnabled(bool enabled) = 0;
      virtual bool getMuteEnabled() = 0;
      virtual void setLoudspeakerEnabled(bool enabled) = 0;
      virtual bool getLoudspeakerEnabled() = 0;
      virtual OutputAudioRoutes getOutputAudioRoute() = 0;
    };
    
    interaction IMediaManagerDelegate
    {
      typedef IMediaManager::OutputAudioRoutes OutputAudioRoutes;
      
      virtual void onMediaManagerAudioRouteChanged(OutputAudioRoutes audioRoute) = 0;
    };
  }
}

ZS_DECLARE_PROXY_BEGIN(openpeer::core::IMediaManagerDelegate)
ZS_DECLARE_PROXY_TYPEDEF(openpeer::core::IMediaManager::OutputAudioRoutes, OutputAudioRoutes)
ZS_DECLARE_PROXY_METHOD_1(onMediaManagerAudioRouteChanged, OutputAudioRoutes)
ZS_DECLARE_PROXY_END()
