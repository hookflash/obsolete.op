//
//  MediaEngineDelegateWrapper.h
//  Hookflash
//
//  Created by Vladimir Morosev on 9/27/12.
//
//

#import <Foundation/Foundation.h>
#include <openpeer/core/IMediaEngineObsolete.h>

using namespace openpeer::core;

class MediaEngineDelegateWrapper : public IMediaEngineDelegateObsolete
{
public:
  typedef boost::shared_ptr<MediaEngineDelegateWrapper> MediaEngineDelegateWrapperPtr;
  
  static MediaEngineDelegateWrapperPtr create();
  
  virtual void onMediaEngineAudioRouteChanged(IMediaEngineObsolete::OutputAudioRoutes audioRoute);
  virtual void onMediaEngineFaceDetected();
  virtual void onMediaEngineVideoCaptureRecordStopped();
  
};

typedef boost::shared_ptr<MediaEngineDelegateWrapper> MediaEngineDelegateWrapperPtr;
