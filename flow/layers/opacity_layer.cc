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
#ifndef SUPPORT_FRACTIONAL_TRANSLATION
  context->SetTransform(
      RasterCacheUtil::GetIntegralTransCTM(context->GetTransform()));
#endif
  DiffChildren(context, prev);
  context->SetLayerPaintRegion(this, context->CurrentSubtreeRegion());
}

void OpacityLayer::Preroll(PrerollContext* context, const SkMatrix& matrix) {
  TRACE_EVENT0("flutter", "OpacityLayer::Preroll");
  FML_DCHECK(!layers().empty());  // We can't be a leaf.

  child_matrix_ = matrix;
  child_matrix_.preTranslate(offset_.fX, offset_.fY);

  // Similar to what's done in TransformLayer::Preroll, we have to apply the
  // reverse transformation to the cull rect to properly cull child layers.
  context->cull_rect = context->cull_rect.makeOffset(-offset_.fX, -offset_.fY);

  context->mutators_stack.PushTransform(
      SkMatrix::Translate(offset_.fX, offset_.fY));
  context->mutators_stack.PushOpacity(alpha_);

  AutoCache cache =
      AutoCache::Create(layer_raster_cache_item_.get(), context, matrix);
  Layer::AutoPrerollSaveLayerState save =
      Layer::AutoPrerollSaveLayerState::Create(context);

  // Collect inheritance information on our children in Preroll so that
  // we can decide whether or not to use a saveLayer in Paint.
  context->subtree_can_inherit_opacity = true;
  // ContainerLayer will turn the flag off if any children are
  // incompatible or if they overlap
  ContainerLayer::Preroll(context, child_matrix_);
  // We store the inheritance ability of our children for |Paint|
  set_children_can_accept_opacity(context->subtree_can_inherit_opacity);

  // Now we let our parent layers know that we, too, can inherit opacity
  // regardless of what our children are capable of
  context->subtree_can_inherit_opacity = true;
  context->mutators_stack.Pop();
  context->mutators_stack.Pop();

  set_paint_bounds(paint_bounds().makeOffset(offset_.fX, offset_.fY));

  if (!children_can_accept_opacity()) {
    auto child_matrix = child_matrix_;
#ifndef SUPPORT_FRACTIONAL_TRANSLATION
    child_matrix = RasterCacheUtil::GetIntegralTransCTM(child_matrix_);
#endif
    layer_raster_cache_item_->CacheChildren(child_matrix);
  }

  // Restore cull_rect
  context->cull_rect = context->cull_rect.makeOffset(offset_.fX, offset_.fY);
}

void OpacityLayer::Paint(PaintContext& context) const {
  TRACE_EVENT0("flutter", "OpacityLayer::Paint");
  FML_DCHECK(needs_painting(context));

  SkAutoCanvasRestore save(context.internal_nodes_canvas, true);
  context.internal_nodes_canvas->translate(offset_.fX, offset_.fY);

#ifndef SUPPORT_FRACTIONAL_TRANSLATION
  context.internal_nodes_canvas->setMatrix(RasterCacheUtil::GetIntegralTransCTM(
      context.leaf_nodes_canvas->getTotalMatrix()));
#endif

  SkScalar inherited_opacity = context.inherited_opacity;
  SkScalar subtree_opacity = opacity() * inherited_opacity;

  if (children_can_accept_opacity()) {
    context.inherited_opacity = subtree_opacity;
    PaintChildren(context);
    context.inherited_opacity = inherited_opacity;
    return;
  }

  SkPaint paint;
  paint.setAlphaf(subtree_opacity);

  if (layer_raster_cache_item_->Draw(context, &paint)) {
    return;
  }

  // Skia may clip the content with saveLayerBounds (although it's not a
  // guaranteed clip). So we have to provide a big enough saveLayerBounds. To do
  // so, we first remove the offset from paint bounds since it's already in the
  // matrix. Then we round out the bounds.
  //
  // Note that the following lines are only accessible when the raster cache is
  // not available (e.g., when we're using the software backend in golden
  // tests).
  SkRect saveLayerBounds;
  paint_bounds()
      .makeOffset(-offset_.fX, -offset_.fY)
      .roundOut(&saveLayerBounds);

  Layer::AutoSaveLayer save_layer =
      Layer::AutoSaveLayer::Create(context, saveLayerBounds, &paint);
  context.inherited_opacity = SK_Scalar1;
  PaintChildren(context);
  context.inherited_opacity = inherited_opacity;
}

}  // namespace flutter
