// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "flutter/shell/platform/darwin/ios/ios_external_view_embedder.h"

namespace flutter {

IOSExternalViewEmbedder::IOSExternalViewEmbedder(
    const std::shared_ptr<FlutterPlatformViewsController>& platform_views_controller,
    std::shared_ptr<IOSContext> context)
    : platform_views_controller_(platform_views_controller), ios_context_(context) {
  FML_CHECK(ios_context_);
}

IOSExternalViewEmbedder::~IOSExternalViewEmbedder() = default;

// |ExternalViewEmbedder|
SkCanvas* IOSExternalViewEmbedder::GetRootCanvas() {
  // On iOS, the root surface is created from the on-screen render target. Only the surfaces for the
  // various overlays are controlled by this class.
  return nullptr;
}

// |ExternalViewEmbedder|
void IOSExternalViewEmbedder::CancelFrame() {
  TRACE_EVENT0("flutter", "IOSExternalViewEmbedder::CancelFrame");
  FML_CHECK(platform_views_controller_);
  platform_views_controller_->CancelFrame();
  // BD ADD
  // Committing the current transaction as |BeginFrame| will create a nested
  // CATransaction otherwise.
  [CATransaction commit];
}

// |ExternalViewEmbedder|
void IOSExternalViewEmbedder::BeginFrame(
    SkISize frame_size,
    GrDirectContext* context,
    double device_pixel_ratio,
    fml::RefPtr<fml::RasterThreadMerger> raster_thread_merger) {
  TRACE_EVENT0("flutter", "IOSExternalViewEmbedder::BeginFrame");
  FML_CHECK(platform_views_controller_);
  platform_views_controller_->BeginFrame(frame_size);
  // BD ADD
  [CATransaction begin];
}

// |ExternalViewEmbedder|
void IOSExternalViewEmbedder::PrerollCompositeEmbeddedView(
    int view_id,
    std::unique_ptr<EmbeddedViewParams> params) {
  TRACE_EVENT0("flutter", "IOSExternalViewEmbedder::PrerollCompositeEmbeddedView");
  FML_CHECK(platform_views_controller_);
  platform_views_controller_->PrerollCompositeEmbeddedView(view_id, std::move(params));
}

// |ExternalViewEmbedder|
PostPrerollResult IOSExternalViewEmbedder::PostPrerollAction(
    fml::RefPtr<fml::RasterThreadMerger> raster_thread_merger) {
  TRACE_EVENT0("flutter", "IOSExternalViewEmbedder::PostPrerollAction");
  FML_CHECK(platform_views_controller_);
  PostPrerollResult result = platform_views_controller_->PostPrerollAction(raster_thread_merger);
  // BD ADD : START
  if (result == PostPrerollResult::kSkipAndRetryFrame) {
    // Commit the current transaction if the frame is dropped.
    [CATransaction commit];
  }
  // END
  return result;
}

// |ExternalViewEmbedder|
std::vector<SkCanvas*> IOSExternalViewEmbedder::GetCurrentCanvases() {
  FML_CHECK(platform_views_controller_);
  return platform_views_controller_->GetCurrentCanvases();
}

// |ExternalViewEmbedder|
SkCanvas* IOSExternalViewEmbedder::CompositeEmbeddedView(int view_id) {
  TRACE_EVENT0("flutter", "IOSExternalViewEmbedder::CompositeEmbeddedView");
  FML_CHECK(platform_views_controller_);
  return platform_views_controller_->CompositeEmbeddedView(view_id);
}

// |ExternalViewEmbedder|
void IOSExternalViewEmbedder::SubmitFrame(GrDirectContext* context,
                                          std::unique_ptr<SurfaceFrame> frame) {
  TRACE_EVENT0("flutter", "IOSExternalViewEmbedder::SubmitFrame");
  FML_CHECK(platform_views_controller_);
  // BD MOD : START
  // platform_views_controller_->SubmitFrame(std::move(context), ios_context_, std::move(frame));
  bool submitted = platform_views_controller_->SubmitFrame(std::move(context), ios_context_, std::move(frame));
  if (submitted) {
      [CATransaction commit];
  }
  // END
  TRACE_EVENT0("flutter", "IOSExternalViewEmbedder::DidSubmitFrame");
}

// |ExternalViewEmbedder|
void IOSExternalViewEmbedder::EndFrame(bool should_resubmit_frame,
                                       fml::RefPtr<fml::RasterThreadMerger> raster_thread_merger) {
  TRACE_EVENT0("flutter", "IOSExternalViewEmbedder::EndFrame");
  FML_CHECK(platform_views_controller_);
}

// |ExternalViewEmbedder|
bool IOSExternalViewEmbedder::SupportsDynamicThreadMerging() {
  return true;
}

}  // namespace flutter
