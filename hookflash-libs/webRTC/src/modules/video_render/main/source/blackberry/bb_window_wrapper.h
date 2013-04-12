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
  BlackberryWindowWrapper(screen_window_t window, screen_context_t context, const char* groupId);
  virtual ~BlackberryWindowWrapper();

  void SetRenderer(VideoRenderBlackBerry* renderer);

  void Render();

  screen_window_t GetWindow() { return _ptrWindow; }
  screen_context_t GetContext() { return _ptrContext; }
  const char* GetGroupId() { return _groupId; }

private:
  screen_window_t _ptrWindow;
  screen_context_t _ptrContext;
  VideoRenderBlackBerry* _renderer;
  char* _groupId;
};

}

#endif // BB_WINDOW_WRAPPER_H_
