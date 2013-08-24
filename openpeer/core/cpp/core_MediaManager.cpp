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

#include <openpeer/core/internal/core_MediaManager.h>
#include <openpeer/core/internal/core_Stack.h>

namespace openpeer { namespace core { ZS_DECLARE_SUBSYSTEM(openpeer_webrtc) } }

namespace openpeer
{
  namespace core
  {
    using zsLib::Stringize;
    
    namespace internal
    {
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark MediaManager
      #pragma mark
      
      //-----------------------------------------------------------------------
      MediaManager::MediaManager(
                                 IMessageQueuePtr queue,
                                 IMediaManagerDelegatePtr delegate
                                 ) :
        MessageQueueAssociator(queue),
        mError(0),
        mID(zsLib::createPUID()),
        mDelegate(IMediaManagerDelegateProxy::createWeak(delegate))      {
      }

      //-----------------------------------------------------------------------
      MediaManager::~MediaManager()
      {
      }
      
      //---------------------------------------------------------------------
      //---------------------------------------------------------------------
      //---------------------------------------------------------------------
      //---------------------------------------------------------------------
      #pragma mark
      #pragma mark MediaManager => IMediaManager
      #pragma mark
      
      //-----------------------------------------------------------------------
      MediaManagerPtr MediaManager::create(IMediaManagerDelegatePtr delegate)
      {
        MediaManagerPtr pThis(new MediaManager(IStackForInternal::queueCore(), delegate));
        pThis->mThisWeak = pThis;
        pThis->init();
        return pThis;
      }
      
      //-----------------------------------------------------------------------
      MediaManagerPtr MediaManager::singleton(IMediaManagerDelegatePtr delegate)
      {
        static MediaManagerPtr engine = create(delegate);
        return engine;
      }
      
      //-------------------------------------------------------------------------
      void MediaManager::setDefaultVideoOrientation(VideoOrientations orientation)
      {
        AutoRecursiveLock lock(mLock);
        
        ZS_LOG_DEBUG(log("set default video orientation - ") + IMediaManager::toString(orientation))
        
//        mDefaultVideoOrientation = orientation;
      }
      
      //-------------------------------------------------------------------------
      MediaManager::VideoOrientations MediaManager::getDefaultVideoOrientation()
      {
        AutoRecursiveLock lock(mLock);
        
        ZS_LOG_DEBUG(log("get default video orientation"))
        
//        return mDefaultVideoOrientation;
      }
      
      //-------------------------------------------------------------------------
      void MediaManager::setRecordVideoOrientation(VideoOrientations orientation)
      {
        AutoRecursiveLock lock(mLock);
        
        ZS_LOG_DEBUG(log("set record video orientation - ") + IMediaManager::toString(orientation))
        
//        mRecordVideoOrientation = orientation;
      }
      
      //-------------------------------------------------------------------------
      MediaManager::VideoOrientations MediaManager::getRecordVideoOrientation()
      {
        AutoRecursiveLock lock(mLock);
        
        ZS_LOG_DEBUG(log("get record video orientation"))
        
//        return mRecordVideoOrientation;
      }
      
      //-----------------------------------------------------------------------
      void MediaManager::setVideoOrientation()
      {
        AutoRecursiveLock lock(mLock);
        
        ZS_LOG_DEBUG(log("set video orientation and codec parameters"))
/*
        if (mVideoChannel == OPENPEER_MEDIA_ENGINE_INVALID_CHANNEL) {
          mError = setVideoCaptureRotation();
        } else {
          mError = setVideoCodecParameters();
        }
*/
      }
      
      //-----------------------------------------------------------------------
      void MediaManager::setMuteEnabled(bool enabled)
      {
        AutoRecursiveLock lock(mLock);
        
        ZS_LOG_DEBUG(log("set microphone mute enabled - value: ") + (enabled ? "true" : "false"))
/*
        mError = mVoiceVolumeControl->SetInputMute(-1, enabled);
        if (mError != 0) {
          ZS_LOG_ERROR(Detail, log("failed to set microphone mute (error: ") + Stringize<INT>(mVoiceBase->LastError()).string() + ")")
          return;
        }
*/
      }
      
      //-----------------------------------------------------------------------
      bool MediaManager::getMuteEnabled()
      {
        AutoRecursiveLock lock(mLock);
        
        ZS_LOG_DEBUG(log("get microphone mute enabled"))
        
        bool enabled;
/*
        mError = mVoiceVolumeControl->GetInputMute(-1, enabled);
        if (mError != 0) {
          ZS_LOG_ERROR(Detail, log("failed to set microphone mute (error: ") + Stringize<INT>(mVoiceBase->LastError()).string() + ")")
          return false;
        }
*/        
        return enabled;
      }
      
      //-----------------------------------------------------------------------
      void MediaManager::setLoudspeakerEnabled(bool enabled)
      {
        AutoRecursiveLock lock(mLock);
        
        ZS_LOG_DEBUG(log("set loudspeaker enabled - value: ") + (enabled ? "true" : "false"))
/*
        mError = mVoiceHardware->SetLoudspeakerStatus(enabled);
        if (mError != 0) {
          ZS_LOG_ERROR(Detail, log("failed to set loudspeaker (error: ") + Stringize<INT>(mVoiceBase->LastError()).string() + ")")
          return;
        }
*/
      }
      
      //-----------------------------------------------------------------------
      bool MediaManager::getLoudspeakerEnabled()
      {
        AutoRecursiveLock lock(mLock);
        
        ZS_LOG_DEBUG(log("get loudspeaker enabled"))
        
        bool enabled;
/*
        mError = mVoiceHardware->GetLoudspeakerStatus(enabled);
        if (mError != 0) {
          ZS_LOG_ERROR(Detail, log("failed to get loudspeaker (error: ") + Stringize<INT>(mVoiceBase->LastError()).string() + ")")
          return false;
        }
*/        
        return enabled;
      }
      
      //-----------------------------------------------------------------------
      IMediaManager::OutputAudioRoutes MediaManager::getOutputAudioRoute()
      {
        AutoRecursiveLock lock(mLock);
        
        ZS_LOG_DEBUG(log("get output audio route"))
/*
        OutputAudioRoute route;
        mError = mVoiceHardware->GetOutputAudioRoute(route);
        if (mError != 0) {
          ZS_LOG_ERROR(Detail, log("failed to get output audio route (error: ") + Stringize<INT>(mVoiceBase->LastError()).string() + ")")
          return OutputAudioRoute_BuiltInSpeaker;
        }
        
        switch (route) {
          case webrtc::kOutputAudioRouteHeadphone:
            return OutputAudioRoute_Headphone;
          case webrtc::kOutputAudioRouteBuiltInReceiver:
            return OutputAudioRoute_BuiltInReceiver;
          case webrtc::kOutputAudioRouteBuiltInSpeaker:
            return OutputAudioRoute_BuiltInSpeaker;
          default:
            return OutputAudioRoute_BuiltInSpeaker;
        }
*/
      }
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IMediaManagerForStack
      #pragma mark
      
      //-----------------------------------------------------------------------
      void IMediaManagerForStack::setup()
      {
        MediaManager::setup();
      }
      
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IMediaManagerForCall
      #pragma mark
      
      //-----------------------------------------------------------------------
      MediaManagerPtr IMediaManagerForCall::singleton()
      {
        return MediaManager::singleton();
      }
      
      //-----------------------------------------------------------------------
      MediaSessionList IMediaManagerForCall::getReceiveMediaSessions()
      {
        
      }
      
      //-----------------------------------------------------------------------
      MediaSessionList IMediaManagerForCall::getSendMediaSessions()
      {
        
      }
      
      //-----------------------------------------------------------------------
      void IMediaManagerForCall::addMediaSession(IMediaSessionPtr session, bool mergeAudioStreams)
      {
        
      }
      
      //-----------------------------------------------------------------------
      void IMediaManagerForCall::removeMediaSession(IMediaSessionPtr session)
      {
        
      }
      
      //-----------------------------------------------------------------------
      IMediaStreamForCallTransport::MediaConstraintList IMediaManagerForCall::getVideoConstraints(ILocalSendVideoStreamForCall::CameraTypes cameraType)
      {
        
      }
      
      //-----------------------------------------------------------------------
      IMediaStreamForCallTransport::MediaConstraintList IMediaManagerForCall::getAudioConstraints()
      {
        
      }
      
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark IMediaManagerForCallTransport
      #pragma mark
      
      //-----------------------------------------------------------------------
      MediaManagerPtr IMediaManagerForCallTransport::singleton()
      {
        return MediaManager::singleton();
      }

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark MediaManager => (internal)
      #pragma mark
      
      //-----------------------------------------------------------------------
      String MediaManager::log(const char *message) const
      {
        return String("MediaManager [") + Stringize<typeof(mID)>(mID).string() + "] " + message;
      }
    }
    
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    #pragma mark
    #pragma mark IMediaManager
    #pragma mark
    
    //-----------------------------------------------------------------------
    IMediaManagerPtr IMediaManager::singleton()
    {
      return internal::MediaManager::singleton();
    }
    
    //---------------------------------------------------------------------------
    const char *IMediaManager::toString(VideoOrientations orientation)
    {
      switch (orientation) {
        case VideoOrientation_LandscapeLeft:        return "Landscape left";
        case VideoOrientation_PortraitUpsideDown:   return "Portrait upside down";
        case VideoOrientation_LandscapeRight:       return "Landscape right";
        case VideoOrientation_Portrait:             return "Portrait";
      }
      return "UNDEFINED";
    }
    
    //-------------------------------------------------------------------------
    const char *IMediaManager::toString(OutputAudioRoutes route)
    {
      switch (route) {
        case OutputAudioRoute_Headphone:        return "Headphone";
        case OutputAudioRoute_BuiltInReceiver:  return "Built in receiver";
        case OutputAudioRoute_BuiltInSpeaker:   return "Built in speaker";
      }
      return "UNDEFINED";
    }
  }
}
