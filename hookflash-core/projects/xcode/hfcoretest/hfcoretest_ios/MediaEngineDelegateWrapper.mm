//
//  MediaEngineDelegateWrapper.cpp
//  Hookflash
//
//  Created by Vladimir Morosev on 9/27/12.
//
//

#include "MediaEngineDelegateWrapper.h"


MediaEngineDelegateWrapperPtr MediaEngineDelegateWrapper::create()
{
  return MediaEngineDelegateWrapperPtr (new MediaEngineDelegateWrapper());
}

void MediaEngineDelegateWrapper::onAudioRouteChanged(IMediaEngine::OutputAudioRoutes audioRoute)
{
  switch (audioRoute)
  {
    case hookflash::IMediaEngine::OutputAudioRoute_Headphone:
      printf("Audio route changed -- Headphone");
      break;
    case hookflash::IMediaEngine::OutputAudioRoute_BuiltInReceiver:
      printf("Audio route changed -- Built in speaker");
      break;
    case hookflash::IMediaEngine::OutputAudioRoute_BuiltInSpeaker:
      printf("Audio route changed -- Built in receiver");
      break;
    default:
      break;
  }
}

