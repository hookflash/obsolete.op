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

  BlackberryWindowWrapper::BlackberryWindowWrapper(const char* parentWindowId, const char* groupId, int width, int height) :
    _width(width),
    _height(height) {

    _parentWindowId = new char[strlen(parentWindowId)+1];
    strcpy(_parentWindowId, parentWindowId);
    _groupId = new char[strlen(groupId)+1];
    strcpy(_groupId, groupId);
  }

  BlackberryWindowWrapper::~BlackberryWindowWrapper() {
    delete[] _parentWindowId;
    delete[] _groupId;
  }

}

