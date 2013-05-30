/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "webrtc/modules/audio_coding/neteq4/tools/input_audio_file.h"

namespace webrtc {
namespace test {

bool InputAudioFile::Read(std::size_t samples, int16_t* destination) {
  if (!fp_) {
    return false;
  }
  std::size_t bytes_read = std::fread(destination, sizeof(int16_t), samples, fp_);
  if (bytes_read < samples) {
    // Rewind and read the missing sampels.
    std::rewind(fp_);
    std::size_t missing_samples = samples - bytes_read;
    if (std::fread(destination, sizeof(int16_t), missing_samples, fp_) <
        missing_samples) {
      // Could not read enough even after rewinding the file.
      return false;
    }
  }
  return true;
}

void InputAudioFile::DuplicateInterleaved(const int16_t* source, std::size_t samples,
                                          std::size_t channels,
                                          int16_t* destination) {
  for (std::size_t i = 0; i < samples; ++i) {
    for (std::size_t j = 0; j < channels; ++j) {
      destination[i * channels + j] = source[i];
    }
  }
}

}  // namespace test
}  // namespace webrtc
