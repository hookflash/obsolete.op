/*
 * bb_window_wrapper.h
 *
 *  Created on: Apr 10, 2013
 *      Author: lawrence.gunn
 */

#include <screen/screen.h>

namespace webrtc {

#ifndef BB_WINDOW_WRAPPER_H_
#define BB_WINDOW_WRAPPER_H_

class VideoRenderBlackBerry;

class BlackberryWindowWrapper {
public:
  BlackberryWindowWrapper(const char* parentWindowId, const char* groupId, int width, int height);
  virtual ~BlackberryWindowWrapper();

  const char* GetParentWindowId() { return _parentWindowId; }
  const char* GetGroupId() { return _groupId; }
  int GetWidth() { return _width; }
  int GetHeight() { return _height; }

private:
  char* _parentWindowId;
  char* _groupId;
  int _width;
  int _height;
};

}

#endif // BB_WINDOW_WRAPPER_H_
