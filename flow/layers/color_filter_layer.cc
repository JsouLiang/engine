// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/flow/layers/color_filter_layer.h"
#include "flutter/flow/raster_cacheable_entry.h"

namespace flutter {

ColorFilterLayer::ColorFilterLayer(sk_sp<SkColorFilter> filter)
    : filter_(std::move(filter)), render_count_(1) {}

void ColorFilterLayer::Diff(DiffContext* context, const Layer* old_layer) {
  DiffContext::AutoSubtreeRestore subtree(context);
  auto* prev = static_cast<const ColorFilterLayer*>(old_layer);
  if (!context->IsSubtreeDirty()) {
    FML_DCHECK(prev);
    if (filter_ != prev->filter_) {
      context->MarkSubtreeDirty(context->GetOldLayerPaintRegion(old_layer));
    }
  }

#ifndef SUPPORT_FRACTIONAL_TRANSLATION
  context->SetTransform(
      RasterCache::GetIntegralTransCTM(context->GetTransform()));
#endif

  DiffChildren(context, prev);

  context->SetLayerPaintRegion(this, context->CurrentSubtreeRegion());
}

void ColorFilterLayer::Preroll(PrerollContext* context,
                               const SkMatrix& matrix) {
  Layer::AutoPrerollSaveLayerState save =
      Layer::AutoPrerollSaveLayerState::Create(context);
  auto cacheable_entry =
      RasterCacheableEntry::MarkLayerCacheable(this, *context, matrix);
  context->raster_cached_entries.push_back(cacheable_entry);

  auto current_index = context->raster_cached_entries.size();

  ContainerLayer::Preroll(context, matrix);

  cacheable_entry->num_child_entries =
      context->raster_cached_entries.size() - current_index;
  auto cache_type = NeedCaching(context, matrix);
  if (cache_type == CacheableLayer::CacheType::kChildren) {
    cacheable_entry->MarkLayerChildrenNeedCached();
  }
}

CacheableLayer::CacheType ColorFilterLayer::NeedCaching(PrerollContext* context,
                                                        const SkMatrix& ctm) {
  if (render_count_ >= kMinimumRendersBeforeCachingFilterLayer) {
    return CacheableLayer::CacheType::kCurrent;
  } else {
    render_count_++;
    return CacheableLayer::CacheType::kChildren;
  }
}

void ColorFilterLayer::Paint(PaintContext& context) const {
  TRACE_EVENT0("flutter", "ColorFilterLayer::Paint");
  FML_DCHECK(needs_painting(context));

  if (context.raster_cache) {
    if (context.raster_cache->Draw(this, *context.leaf_nodes_canvas,
                                   RasterCacheLayerStrategy::kLayer)) {
      return;
    }
  SkPaint paint;
  paint.setColorFilter(filter_);

  Layer::AutoSaveLayer save =
      Layer::AutoSaveLayer::Create(context, paint_bounds(), &paint);
  PaintChildren(context);
}

}  // namespace flutter
