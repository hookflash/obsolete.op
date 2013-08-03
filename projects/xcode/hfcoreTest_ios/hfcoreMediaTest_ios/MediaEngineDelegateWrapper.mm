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

void MediaEngineDelegateWrapper::onMediaEngineAudioRouteChanged(IMediaEngineObsolete::OutputAudioRoutes audioRoute)
{
  switch (audioRoute)
  {
    case openpeer::core::IMediaEngineObsolete::OutputAudioRoute_Headphone:
      printf("Audio route changed -- Headphone\n");
      break;
    case openpeer::core::IMediaEngineObsolete::OutputAudioRoute_BuiltInReceiver:
      printf("Audio route changed -- Built in speaker\n");
      break;
    case openpeer::core::IMediaEngineObsolete::OutputAudioRoute_BuiltInSpeaker:
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

void MediaEngineDelegateWrapper::onMediaEngineVideoCaptureRecordStopped()
{
  printf("******************* VIDEO RECORDING STOPPED *******************\n");
}
