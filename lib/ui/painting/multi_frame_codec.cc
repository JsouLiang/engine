// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/lib/ui/painting/multi_frame_codec.h"

#include "flutter/fml/make_copyable.h"
#include "flutter/lib/ui/painting/image.h"
#include "third_party/dart/runtime/include/dart_api.h"
#include "third_party/skia/include/core/SkPixelRef.h"
#include "third_party/tonic/logging/dart_invoke.h"

// BD ADD:
#include "flutter/bdflutter/lib/ui/performance/performance.h"

namespace flutter {

// BD MOD: START
//MultiFrameCodec::MultiFrameCodec(
//    std::shared_ptr<SkCodecImageGenerator> generator)
//    : state_(new State(std::move(generator))) {}
MultiFrameCodec::MultiFrameCodec(
    std::shared_ptr<SkCodecImageGenerator> generator, std::string key)
    : state_(new State(std::move(generator), key)) {
    codec_recorder.insert(std::make_pair(state_->key_, this));
}
// END

/**
 * BD ADD
 * @param codec
 */
MultiFrameCodec::MultiFrameCodec(Codec* codec) {
    real_codec = codec;
    real_codec->RetainDartWrappableReference();
}

// BD MOD: START
// MultiFrameCodec::~MultiFrameCodec() = default;
MultiFrameCodec::~MultiFrameCodec() {
    if (real_codec == nullptr) {
        codec_recorder.erase(state_->key_);
    } else {
        real_codec->ReleaseDartWrappableReference();
    }
}
// END

MultiFrameCodec::State::State(std::shared_ptr<SkCodecImageGenerator> generator, std::string key)
    : generator_(std::move(generator)),
      repetitionCount_(generator_->getRepetitionCount()),
      frameCount_(generator_->getFrameCount()),
      key_(std::move(key)),
      nextFrameIndex_(0) {}

static void InvokeNextFrameCallback(
    fml::RefPtr<CanvasImage> image,
    int duration,
    std::unique_ptr<DartPersistentValue> callback,
    size_t trace_id) {
  std::shared_ptr<tonic::DartState> dart_state = callback->dart_state().lock();
  if (!dart_state) {
    FML_DLOG(ERROR) << "Could not acquire Dart state while attempting to fire "
                       "next frame callback.";
    return;
  }
  tonic::DartState::Scope scope(dart_state);
  tonic::DartInvoke(callback->value(),
                    {tonic::ToDart(image), tonic::ToDart(duration)});
}

// Copied the source bitmap to the destination. If this cannot occur due to
// running out of memory or the image info not being compatible, returns false.
static bool CopyToBitmap(SkBitmap* dst,
                         SkColorType dstColorType,
                         const SkBitmap& src) {
  SkPixmap srcPM;
  if (!src.peekPixels(&srcPM)) {
    return false;
  }

  SkBitmap tmpDst;
  SkImageInfo dstInfo = srcPM.info().makeColorType(dstColorType);
  if (!tmpDst.setInfo(dstInfo)) {
    return false;
  }

  if (!tmpDst.tryAllocPixels()) {
    return false;
  }

  SkPixmap dstPM;
  if (!tmpDst.peekPixels(&dstPM)) {
    return false;
  }

  if (!srcPM.readPixels(dstPM)) {
    return false;
  }

  dst->swap(tmpDst);
  return true;
}

sk_sp<SkImage> MultiFrameCodec::State::GetNextFrameImage(
  // BD MOD:
  // fml::WeakPtr<GrDirectContext> resourceContext
  fml::WeakPtr<IOManager> io_manager) {
  SkBitmap bitmap = SkBitmap();
  SkImageInfo info = generator_->getInfo().makeColorType(kN32_SkColorType);
  if (info.alphaType() == kUnpremul_SkAlphaType) {
    SkImageInfo updated = info.makeAlphaType(kPremul_SkAlphaType);
    info = updated;
  }
  bitmap.allocPixels(info);

  SkCodec::Options options;
  options.fFrameIndex = nextFrameIndex_;
  SkCodec::FrameInfo frameInfo{0};
  generator_->getFrameInfo(nextFrameIndex_, &frameInfo);
  const int requiredFrameIndex = frameInfo.fRequiredFrame;
  if (requiredFrameIndex != SkCodec::kNoFrame) {
    if (lastRequiredFrame_ == nullptr) {
      FML_LOG(ERROR) << "Frame " << nextFrameIndex_ << " depends on frame "
                     << requiredFrameIndex
                     << " and no required frames are cached.";
      return nullptr;
    } else if (lastRequiredFrameIndex_ != requiredFrameIndex) {
      FML_DLOG(INFO) << "Required frame " << requiredFrameIndex
                     << " is not cached. Using " << lastRequiredFrameIndex_
                     << " instead";
    }

    if (lastRequiredFrame_->getPixels() &&
        CopyToBitmap(&bitmap, lastRequiredFrame_->colorType(),
                     *lastRequiredFrame_)) {
      options.fPriorFrame = requiredFrameIndex;
    }
  }

  if (!generator_->getPixels(info, bitmap.getPixels(), bitmap.rowBytes(),
                             &options)) {
    FML_LOG(ERROR) << "Could not getPixels for frame " << nextFrameIndex_;
    return nullptr;
  }

  // Hold onto this if we need it to decode future frames.
  if (frameInfo.fDisposalMethod == SkCodecAnimation::DisposalMethod::kKeep) {
    lastRequiredFrame_ = std::make_unique<SkBitmap>(bitmap);
    lastRequiredFrameIndex_ = nextFrameIndex_;
  }

  // BD MOD: START
  //  if (resourceContext) {
  //    SkPixmap pixmap(bitmap.info(), bitmap.pixelRef()->pixels(),
  //                    bitmap.pixelRef()->rowBytes());
  //    return SkImage::MakeCrossContextFromPixmap(resourceContext.get(), pixmap,
  //                                               // BD MOD:
  //                                               // true
  //                                               !Performance::GetInstance()->IsDisableMips()
  //                                               );
  //  } else {
  //    // Defer decoding until time of draw later on the GPU thread. Can happen
  //    // when GL operations are currently forbidden such as in the background
  //    // on iOS.
  //    return SkImage::MakeFromBitmap(bitmap);
  //  }
  sk_sp<SkImage> result;
  io_manager->GetIsGpuDisabledSyncSwitch()->Execute(
      fml::SyncSwitch::Handlers()
          .SetIfTrue([&result, &bitmap]{
            // Defer decoding until time of draw later on the GPU thread. Can happen
            // when GL operations are currently forbidden such as in the background
            // on iOS.
            result = SkImage::MakeFromBitmap(bitmap);
          })
          .SetIfFalse([&result, &bitmap,
                          resourceContext = io_manager->GetResourceContext()]{
            SkPixmap pixmap(bitmap.info(), bitmap.pixelRef()->pixels(),
                            bitmap.pixelRef()->rowBytes());
            result = SkImage::MakeCrossContextFromPixmap(
                resourceContext.get(),  // context
                pixmap,         // pixmap
                // BD MOD:
                // true,           // buildMips,
                !Performance::GetInstance()->IsDisableMips()
            );
          })
  );
  return result;
  // END
}

void MultiFrameCodec::State::GetNextFrameAndInvokeCallback(
    std::unique_ptr<DartPersistentValue> callback,
    fml::RefPtr<fml::TaskRunner> ui_task_runner,
    // BD MOD: START
    // fml::WeakPtr<GrDirectContext> resourceContext,
    // fml::RefPtr<flutter::SkiaUnrefQueue> unref_queue,
    fml::WeakPtr<IOManager> io_manager,
    // END
    size_t trace_id) {
  fml::RefPtr<CanvasImage> image = nullptr;
  int duration = 0;
  // BD MOD: START
  // sk_sp<SkImage> skImage = GetNextFrameImage(resourceContext);
  sk_sp<SkImage> skImage = GetNextFrameImage(io_manager);
  fml::RefPtr<flutter::SkiaUnrefQueue> unref_queue = io_manager->GetSkiaUnrefQueue();
  // END
  if (skImage) {
    // BD MOD
    // image = CanvasImage::Create();
    image = CanvasImage::Create(key_);
    // BD ADD:
    image->setMips(!Performance::GetInstance()->IsDisableMips());
    image->set_image({skImage, std::move(unref_queue)});
    SkCodec::FrameInfo skFrameInfo{0};
    generator_->getFrameInfo(nextFrameIndex_, &skFrameInfo);
    duration = skFrameInfo.fDuration;
    image->duration = duration;
  }

  // BD ADD: Protect invalid image buffer.
  if (frameCount_ > 0) {
    nextFrameIndex_ = (nextFrameIndex_ + 1) % frameCount_;
  }

  ui_task_runner->PostTask(fml::MakeCopyable([callback = std::move(callback),
                                              image = std::move(image),
                                              duration, trace_id, this]() mutable {
    // BD ADD:START
    inProgress = false;
    if (!pending_callbacks_.empty()) {
        for (const DartPersistentValue &callback_item : pending_callbacks_) {
            auto dart_state = callback_item.dart_state().lock();
            if (!dart_state) {
                continue;
            }
            tonic::DartState::Scope scope(dart_state);
            tonic::DartInvoke(callback_item.value(),
                    {tonic::ToDart(CanvasImage::Create(image.get())), tonic::ToDart(duration)});
        }
        pending_callbacks_.clear();
    }
    // END
    InvokeNextFrameCallback(std::move(image), duration, std::move(callback),
                            trace_id);
  }));
}

/**
 * BD ADD
 * @param args
 * @param requireIndex
 * @return
 */
int MultiFrameCodec::getNextFrameWithCount(Dart_Handle args, int requireIndex) {
    if (requireIndex > currentIndex_) {
        realGetFrame(args);
    } else {
        auto it_find = CanvasImage::image_recorder.find(state_->key_);
        if (it_find != CanvasImage::image_recorder.end()) {
            tonic::DartInvoke(args,
                              {tonic::ToDart(CanvasImage::Create(it_find->second)),
                               tonic::ToDart(it_find->second->duration)});
        } else {
            if (state_->inProgress) {
                auto dart_state = UIDartState::Current();
                state_->pending_callbacks_.emplace_back(dart_state, args);
            } else {
                realGetFrame(args);
            }
        }
    }
    return currentIndex_;
}

Dart_Handle MultiFrameCodec::getNextFrame(Dart_Handle callback_handle) {
  if (!Dart_IsClosure(callback_handle)) {
    return tonic::ToDart("Callback must be a function");
  }

  requireIndex_++;
  if (real_codec != nullptr) {
      requireIndex_ = ((MultiFrameCodec *) real_codec)->getNextFrameWithCount(callback_handle, requireIndex_);
  } else {
     requireIndex_ = getNextFrameWithCount(callback_handle, requireIndex_);
  }
  return Dart_Null();
}

void MultiFrameCodec::realGetFrame(Dart_Handle callback_handle) {
    static size_t trace_counter = 1;
    const size_t trace_id = trace_counter++;
    currentIndex_++;
    state_->inProgress = true;
    auto* dart_state = UIDartState::Current();

    const auto& task_runners = dart_state->GetTaskRunners();

  task_runners.GetIOTaskRunner()->PostTask(fml::MakeCopyable(
      [callback = std::make_unique<DartPersistentValue>(
           tonic::DartState::Current(), callback_handle),
       weak_state = std::weak_ptr<MultiFrameCodec::State>(state_), trace_id,
       ui_task_runner = task_runners.GetUITaskRunner(),
       io_manager = dart_state->GetIOManager()]() mutable {

        auto state = weak_state.lock();
        if (!state) {
          ui_task_runner->PostTask(fml::MakeCopyable(
              [callback = std::move(callback)]() { callback->Clear(); }));
          return;
        }
        while (io_manager.get() == nullptr && (!state->pending_callbacks_.empty())) {
            auto otherCallback = state->pending_callbacks_.begin();
            auto otherState = (*otherCallback).dart_state().lock();
            if (otherState) {
                io_manager = (static_cast<UIDartState *>(otherState.get()))->GetIOManager();
            } else {
                ui_task_runner->PostTask(fml::MakeCopyable(
                        [otherCallback]() { otherCallback->Clear(); }));
            }
        }
        if (io_manager.get() == nullptr) {
            ui_task_runner->PostTask(fml::MakeCopyable(
                    [callback = std::move(callback)]() { callback->Clear(); }));
            state->inProgress = false;
            FML_LOG(ERROR) << " No io_manager, Engine maybe destroyed";
            return;
        }
        state->GetNextFrameAndInvokeCallback(
            std::move(callback), std::move(ui_task_runner),
            // BD MOD:
            // io_manager->GetResourceContext(), io_manager->GetSkiaUnrefQueue(),
            io_manager,
            trace_id);
      }));
}

int MultiFrameCodec::frameCount() const {
    // BD ADD: START
    if (real_codec!= nullptr){
        return real_codec->frameCount();
    }
    // END
  return state_->frameCount_;
}

int MultiFrameCodec::repetitionCount() const {
    // BD ADD:START
    if (real_codec!=nullptr){
        return real_codec->repetitionCount();
    }
    // END
  return state_->repetitionCount_;
}

/**
 * BD ADD
 * @return
 */
CodecType MultiFrameCodec::getClassType() const {
    return CodecType::MultiFrame;
}

}  // namespace flutter
