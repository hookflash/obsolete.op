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

void MediaEngineDelegateWrapper::onMediaEngineAudioRouteChanged(IMediaEngine::OutputAudioRoutes audioRoute)
{
  switch (audioRoute)
  {
    case hookflash::IMediaEngine::OutputAudioRoute_Headphone:
      printf("Audio route changed -- Headphone\n");
      break;
    case hookflash::IMediaEngine::OutputAudioRoute_BuiltInReceiver:
      printf("Audio route changed -- Built in speaker\n");
      break;
    case hookflash::IMediaEngine::OutputAudioRoute_BuiltInSpeaker:
      printf("Audio route changed -- Built in receiver\n");
      break;
    default:
      break;
  }
}


void MediaEngineDelegateWrapper::onMediaEngineFaceDetected()
{
    printf("******************* FACE DETECTED *******************\n");
}

