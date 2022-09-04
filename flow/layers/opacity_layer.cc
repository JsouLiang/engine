// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/flow/layers/opacity_layer.h"

#include "flutter/flow/layers/cacheable_layer.h"
#include "third_party/skia/include/core/SkPaint.h"

namespace flutter {

// the opacity_layer couldn't cache itself, so the cache_threshold is the
// max_int
OpacityLayer::OpacityLayer(SkAlpha alpha, const SkPoint& offset)
    : CacheableContainerLayer(std::numeric_limits<int>::max(), true),
      alpha_(alpha),
      offset_(offset),
      children_can_accept_opacity_(false) {}

void OpacityLayer::Diff(DiffContext* context, const Layer* old_layer) {
  DiffContext::AutoSubtreeRestore subtree(context);
  auto* prev = static_cast<const OpacityLayer*>(old_layer);
  if (!context->IsSubtreeDirty()) {
    FML_DCHECK(prev);
    if (alpha_ != prev->alpha_ || offset_ != prev->offset_) {
      context->MarkSubtreeDirty(context->GetOldLayerPaintRegion(old_layer));
    }
  }
  context->PushTransform(SkMatrix::Translate(offset_.fX, offset_.fY));
  DiffChildren(context, prev);
  context->SetLayerPaintRegion(this, context->CurrentSubtreeRegion());
}

void OpacityLayer::Preroll(PrerollContext* context, const SkMatrix& matrix) {
  TRACE_EVENT0("flutter", "OpacityLayer::Preroll");
  FML_DCHECK(!layers().empty());  // We can't be a leaf.

  SkMatrix child_matrix = matrix;
  child_matrix.preTranslate(offset_.fX, offset_.fY);

  // Similar to what's done in TransformLayer::Preroll, we have to apply the
  // reverse transformation to the cull rect to properly cull child layers.
  context->cull_rect = context->cull_rect.makeOffset(-offset_.fX, -offset_.fY);

  context->mutators_stack.PushTransform(
      SkMatrix::Translate(offset_.fX, offset_.fY));
  context->mutators_stack.PushOpacity(alpha_);

  AutoCache auto_cache =
      AutoCache(layer_raster_cache_item_.get(), context, matrix);
  Layer::AutoPrerollSaveLayerState save =
      Layer::AutoPrerollSaveLayerState::Create(context);

  ContainerLayer::Preroll(context, child_matrix);
  // We store the inheritance ability of our children for |Paint|
  set_children_can_accept_opacity((context->rendering_state_flags &
                                   LayerStateStack::CALLER_CAN_APPLY_OPACITY) !=
                                  0);

  // Now we let our parent layers know that we, too, can inherit opacity
  // regardless of what our children are capable of
  context->rendering_state_flags = LayerStateStack::CALLER_CAN_APPLY_OPACITY;
  context->mutators_stack.Pop();
  context->mutators_stack.Pop();

  set_paint_bounds(paint_bounds().makeOffset(offset_.fX, offset_.fY));

  if (children_can_accept_opacity()) {
    // For opacity layer, we can use raster_cache children only when the
    // children can't accept opacity so if the children_can_accept_opacity we
    // should tell the AutoCache object don't do raster_cache.
    auto_cache.ShouldNotBeCached();
  }

  // Restore cull_rect
  context->cull_rect = context->cull_rect.makeOffset(offset_.fX, offset_.fY);
}

void OpacityLayer::Paint(PaintContext& context) const {
  TRACE_EVENT0("flutter", "OpacityLayer::Paint");
  FML_DCHECK(needs_painting(context));

  auto save = context.state_stack.save();
  context.state_stack.translate(offset_.fX, offset_.fY);

  auto restore = context.state_stack.saveWithOpacity(
      &paint_bounds(), opacity(), checkerboard_bounds(context));

  PaintChildren(context);
}

}  // namespace flutter
