//
//  MediaEngineDelegateWrapper.h
//  Hookflash
//
//  Created by Vladimir Morosev on 9/27/12.
//
//

#import <Foundation/Foundation.h>
#include <hookflash/hookflashTypes.h>
#include <hookflash/IMediaEngine.h>

using namespace hookflash;

class MediaEngineDelegateWrapper : public IMediaEngineDelegate
{
public:
  typedef boost::shared_ptr<MediaEngineDelegateWrapper> MediaEngineDelegateWrapperPtr;
  
  static MediaEngineDelegateWrapperPtr create();
  
  virtual void onMediaEngineAudioRouteChanged(IMediaEngine::OutputAudioRoutes audioRoute);
  virtual void onMediaEngineFaceDetected();
  
};

typedef boost::shared_ptr<MediaEngineDelegateWrapper> MediaEngineDelegateWrapperPtr;
