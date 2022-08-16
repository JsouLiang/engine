// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/flow/layers/layer.h"

#include "flutter/flow/paint_utils.h"
#include "third_party/skia/include/core/SkColorFilter.h"

namespace flutter {

Layer::Layer()
    : paint_bounds_(SkRect::MakeEmpty()),
      unique_id_(NextUniqueID()),
      original_layer_id_(unique_id_),
      subtree_has_platform_view_(false) {}

Layer::~Layer() = default;

uint64_t Layer::NextUniqueID() {
  static std::atomic<uint64_t> nextID(1);
  uint64_t id;
  do {
    id = nextID.fetch_add(1);
  } while (id == 0);  // 0 is reserved for an invalid id.
  return id;
}

void Layer::Preroll(PrerollContext* context, const SkMatrix& matrix) {}

Layer::AutoPrerollSaveLayerState::AutoPrerollSaveLayerState(
    PrerollContext* preroll_context,
    bool save_layer_is_active,
    bool layer_itself_performs_readback)
    : preroll_context_(preroll_context),
      save_layer_is_active_(save_layer_is_active),
      layer_itself_performs_readback_(layer_itself_performs_readback) {
  if (save_layer_is_active_) {
    prev_surface_needs_readback_ = preroll_context_->surface_needs_readback;
    preroll_context_->surface_needs_readback = false;
  }
}

Layer::AutoPrerollSaveLayerState Layer::AutoPrerollSaveLayerState::Create(
    PrerollContext* preroll_context,
    bool save_layer_is_active,
    bool layer_itself_performs_readback) {
  return Layer::AutoPrerollSaveLayerState(preroll_context, save_layer_is_active,
                                          layer_itself_performs_readback);
}

Layer::AutoPrerollSaveLayerState::~AutoPrerollSaveLayerState() {
  if (save_layer_is_active_) {
    preroll_context_->surface_needs_readback =
        (prev_surface_needs_readback_ || layer_itself_performs_readback_);
  }
}

LayerStateStack::LayerStateStack() {
  state_stack_.emplace_back(SkM44());
}

void LayerStateStack::setCanvasDelegate(SkCanvas* canvas) {
  if (canvas_) {
    canvas_->restoreToCount(canvas_restore_count_);
    canvas_ = nullptr;
  }
  if (canvas) {
    canvas_restore_count_ = canvas->getSaveCount();
    canvas_ = canvas;
    for (auto& state : state_stack_) {
      if (state.is_layer) {
        canvas->saveLayer(state.save_bounds(), state.save_skpaint());
      } else {
        canvas->save();
      }
      canvas->setMatrix(state.matrix);
      for (auto& path : state.clip_paths) {
        canvas->clipPath(path);
      }
    }
  }
}

void LayerStateStack::setBuilderDelegate(DisplayListBuilder* builder) {
  if (builder_) {
    builder_->restoreToCount(builder_restore_count_);
    builder_ = nullptr;
  }
  if (builder) {
    builder_restore_count_ = builder->getSaveCount();
    builder_ = builder;
    for (auto& state : state_stack_) {
      if (state.is_layer) {
        builder->saveLayer(state.save_bounds(), state.save_skpaint());
      } else {
        builder->save();
      }
      builder->transformReset();
      builder->transform(state.matrix);
      for (auto& path : state.clip_paths) {
        builder->clipPath(path, SkClipOp::kIntersect, false);
      }
    }
  }
}

void LayerStateStack::setMutatorDelegate(MutatorsStack* mutators) {
  // Does a MutatorsStack do restoreToCount?
  if (mutators_) {
    // builder_->restoreToCount(builder_restore_count_);
    mutators_ = nullptr;
  }
  if (mutators) {
    // builder_restore_count_ = mutators->getSaveCount();
    mutators_ = mutators;
  }
}

LayerStateStack::AutoRestore::AutoRestore(LayerStateStack* stack)
    : stack_(stack), stack_restore_count_(stack->getSaveCount()) {}

LayerStateStack::AutoRestore::~AutoRestore() {
  stack_->restoreToCount(stack_restore_count_);
}

void LayerStateStack::save() {
  state_stack_.emplace_back(state_stack_.back().matrix);
}

LayerStateStack::AutoRestore LayerStateStack::autoSave() {
  save();
  return AutoRestore(this);
}

LayerStateStack::RenderState::RenderState(SkM44& incoming_matrix)
    : matrix(incoming_matrix) {}

Layer::AutoSaveLayer::AutoSaveLayer(const PaintContext& paint_context,
                                    const SkRect& bounds,
                                    const SkPaint* paint,
                                    SaveMode save_mode)
    : paint_context_(paint_context),
      bounds_(bounds),
      canvas_(save_mode == SaveMode::kInternalNodesCanvas
                  ? *(paint_context.internal_nodes_canvas)
                  : *(paint_context.leaf_nodes_canvas)) {
  TRACE_EVENT0("flutter", "Canvas::saveLayer");
  canvas_.saveLayer(bounds_, paint);
}

Layer::AutoSaveLayer::AutoSaveLayer(const PaintContext& paint_context,
                                    const SkCanvas::SaveLayerRec& layer_rec,
                                    SaveMode save_mode)
    : paint_context_(paint_context),
      bounds_(*layer_rec.fBounds),
      canvas_(save_mode == SaveMode::kInternalNodesCanvas
                  ? *(paint_context.internal_nodes_canvas)
                  : *(paint_context.leaf_nodes_canvas)) {
  TRACE_EVENT0("flutter", "Canvas::saveLayer");
  canvas_.saveLayer(layer_rec);
}

Layer::AutoSaveLayer Layer::AutoSaveLayer::Create(
    const PaintContext& paint_context,
    const SkRect& bounds,
    const SkPaint* paint,
    SaveMode save_mode) {
  return Layer::AutoSaveLayer(paint_context, bounds, paint, save_mode);
}

Layer::AutoSaveLayer Layer::AutoSaveLayer::Create(
    const PaintContext& paint_context,
    const SkCanvas::SaveLayerRec& layer_rec,
    SaveMode save_mode) {
  return Layer::AutoSaveLayer(paint_context, layer_rec, save_mode);
}

Layer::AutoSaveLayer::~AutoSaveLayer() {
  if (paint_context_.checkerboard_offscreen_layers) {
    DrawCheckerboard(&canvas_, bounds_);
  }
  canvas_.restore();
}

}  // namespace flutter
