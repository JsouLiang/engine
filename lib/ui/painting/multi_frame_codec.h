// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_LIB_UI_PAINTING_MUTLI_FRAME_CODEC_H_
#define FLUTTER_LIB_UI_PAINTING_MUTLI_FRAME_CODEC_H_

#include "flutter/fml/macros.h"
#include "flutter/lib/ui/painting/codec.h"
#include "third_party/skia/src/codec/SkCodecImageGenerator.h"

namespace flutter {

class MultiFrameCodec : public Codec {
 public:
  // BD MOD: START
  // MultiFrameCodec(std::shared_ptr<SkCodecImageGenerator> generator);
  MultiFrameCodec(std::shared_ptr<SkCodecImageGenerator> generator, std::string key);

  MultiFrameCodec(Codec* codec);
  // END

  ~MultiFrameCodec() override;

  // |Codec|
  int frameCount() const override;

  // |Codec|
  int repetitionCount() const override;

  CodecType getClassType() const override;

  // |Codec|
  Dart_Handle getNextFrame(Dart_Handle args) override;

  // BD ADD
  int getNextFrameWithCount(Dart_Handle args, int requireIndex);

 private:
  // Captures the state shared between the IO and UI task runners.
  //
  // The state is initialized on the UI task runner when the Dart object is
  // created. Decoding occurs on the IO task runner. Since it is possible for
  // the UI object to be collected independently of the IO task runner work,
  // it is not safe for this state to live directly on the MultiFrameCodec.
  // Instead, the MultiFrameCodec creates this object when it is constructed,
  // shares it with the IO task runner's decoding work, and sets the live_
  // member to false when it is destructed.
  struct State {
    // BD MOD
    // State(std::shared_ptr<SkCodecImageGenerator> generator);
    State(std::shared_ptr<SkCodecImageGenerator> generator,std::string key);

    const std::shared_ptr<SkCodecImageGenerator> generator_;
    const int frameCount_;
    const int repetitionCount_;
    // BD ADD: START
    const std::string key_;
    std::vector<DartPersistentValue> pending_callbacks_;
    bool inProgress = false;
    // END

    // The non-const members and functions below here are only read or written
    // to on the IO thread. They are not safe to access or write on the UI
    // thread.
    int nextFrameIndex_;
    // The last decoded frame that's required to decode any subsequent frames.
    std::unique_ptr<SkBitmap> lastRequiredFrame_;

    // The index of the last decoded required frame.
    int lastRequiredFrameIndex_ = -1;

    // BD MOD:
    // sk_sp<SkImage> GetNextFrameImage(fml::WeakPtr<GrDirectContext> resourceContext);
    sk_sp<SkImage> GetNextFrameImage(fml::WeakPtr<IOManager> io_manager);

    void GetNextFrameAndInvokeCallback(
        std::unique_ptr<DartPersistentValue> callback,
        fml::RefPtr<fml::TaskRunner> ui_task_runner,
        // BD MOD: START
        // fml::WeakPtr<GrDirectContext> resourceContext,
        // fml::RefPtr<flutter::SkiaUnrefQueue> unref_queue,
        fml::WeakPtr<IOManager> io_manager,
        // END
        size_t trace_id);
  };

  // Shared across the UI and IO task runners.
  std::shared_ptr<State> state_;

  // BD ADD: START
  int requireIndex_ = 0;
  int currentIndex_ = 0;
  void realGetFrame(Dart_Handle args);
  // END

  FML_FRIEND_MAKE_REF_COUNTED(MultiFrameCodec);
  FML_FRIEND_REF_COUNTED_THREAD_SAFE(MultiFrameCodec);
};

}  // namespace flutter

#endif  // FLUTTER_LIB_UI_PAINTING_MUTLI_FRAME_CODEC_H_
