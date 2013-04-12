/*
 * bb_window_wrapper.cpp
 *
 *  Created on: Apr 10, 2013
 *      Author: lawrence.gunn
 */

#include "bb_window_wrapper.h"
#include "video_render_bb_impl.h"
#include <string.h>

namespace webrtc {

BlackberryWindowWrapper::BlackberryWindowWrapper(
  screen_window_t window,
  screen_context_t context,
  const char* groupId) :
  _ptrWindow(window),
  _ptrContext(context),
  _renderer(NULL) {
  _groupId = new char[strlen(groupId)+1];
  strcpy(_groupId, groupId);
}

BlackberryWindowWrapper::~BlackberryWindowWrapper() {
  delete[] _groupId;
}

void BlackberryWindowWrapper::Render() {
  if(_renderer) {
    _renderer->OnBBRenderEvent();
  }
}

}

