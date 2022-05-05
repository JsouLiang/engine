// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/flow/display_list_raster_cache_item.h"

#include <optional>
#include <utility>

#include "flutter/display_list/display_list.h"
#include "flutter/flow/layers/layer.h"
#include "flutter/flow/raster_cache.h"
#include "flutter/flow/raster_cache_key.h"
#include "flutter/flow/raster_cache_util.h"
#include "flutter/flow/skia_gpu_object.h"

namespace flutter {

static bool IsDisplayListWorthRasterizing(
    DisplayList* display_list,
    bool will_change,
    bool is_complex,
    DisplayListComplexityCalculator* complexity_calculator) {
  if (will_change) {
    // If the display list is going to change in the future, there is no point
    // in doing to extra work to rasterize.
    return false;
  }

  if (display_list == nullptr ||
      !RasterCacheUtil::CanRasterizeRect(display_list->bounds())) {
    // No point in deciding whether the display list is worth rasterizing if it
    // cannot be rasterized at all.
    return false;
  }

  if (is_complex) {
    // The caller seems to have extra information about the display list and
    // thinks the display list is always worth rasterizing.
    return true;
  }

  unsigned int complexity_score = complexity_calculator->Compute(display_list);
  return complexity_calculator->ShouldBeCached(complexity_score);
}

DisplayListRasterCacheItem::DisplayListRasterCacheItem(
    DisplayList* display_list,
    const SkPoint& offset,
    bool is_complex,
    bool will_change)
    : RasterCacheItem(RasterCacheKeyID(display_list->unique_id(),
                                       RasterCacheKeyType::kDisplayList),
                      CacheState::kCurrent),
      display_list_(display_list),
      offset_(offset),
      is_complex_(is_complex),
      will_change_(will_change) {}

void DisplayListRasterCacheItem::PrerollSetup(PrerollContext* context,
                                              const SkMatrix& matrix) {
  cache_state_ = CacheState::kNone;
  if (!context->raster_cache->GenerateNewCacheInThisFrame()) {
    return;
  }

  DisplayListComplexityCalculator* complexity_calculator =
      context->gr_context ? DisplayListComplexityCalculator::GetForBackend(
                                context->gr_context->backend())
                          : DisplayListComplexityCalculator::GetForSoftware();

  if (!IsDisplayListWorthRasterizing(display_list_, will_change_, is_complex_,
                                     complexity_calculator)) {
    // We only deal with display lists that are worthy of rasterization.
    return;
  }

  transformation_matrix_ = matrix;
  transformation_matrix_.preTranslate(offset_.x(), offset_.y());

  if (!transformation_matrix_.invert(nullptr)) {
    // The matrix was singular. No point in going further.
    return;
  }

  if (context->raster_cached_entries && context->raster_cache) {
    context->raster_cached_entries->push_back(this);
    cache_state_ = CacheState::kCurrent;
  }
  return;
}

void DisplayListRasterCacheItem::PrerollFinalize(PrerollContext* context,
                                                 const SkMatrix& matrix) {
  if (cache_state_ == CacheState::kNone || !context->raster_cache ||
      !context->raster_cached_entries) {
    return;
  }
  SkRect bounds = display_list_->bounds().makeOffset(offset_.x(), offset_.y());
  // We've marked the cache entry that we would like to cache so it stays
  // alive, but if the following conditions apply then we need to set our
  // state back to !cacheable_ so that we don't populate the entry later.
  if (!SkRect::Intersects(context->cull_rect, bounds)) {
    cache_state_ = CacheState::kNone;
    return;
  }
  if (context->raster_cache) {
    // Frame threshold has not yet been reached.
    if (num_cache_attempts_ <= context->raster_cache->access_threshold()) {
      if (!context->raster_cache->HasEntry(key_id_, transformation_matrix_)) {
        // Creates an entry, if not present prior.
        context->raster_cache->MarkSeen(key_id_, transformation_matrix_);
      }
      cache_state_ = CacheState::kNone;
    } else {
      context->subtree_can_inherit_opacity = true;
      cache_state_ = CacheState::kCurrent;
    }
    return;
  }
}

bool DisplayListRasterCacheItem::Draw(const PaintContext& context,
                                      const SkPaint* paint) const {
  return Draw(context, context.leaf_nodes_canvas, paint);
}

bool DisplayListRasterCacheItem::Draw(const PaintContext& context,
                                      SkCanvas* canvas,
                                      const SkPaint* paint) const {
  if (!context.raster_cache || !canvas) {
    return false;
  }
  if (context.raster_cache->HasEntry(key_id_, canvas->getTotalMatrix())) {
    num_cache_attempts_++;
  }
  if (cache_state_ == CacheState::kCurrent) {
    return context.raster_cache->Draw(key_id_, *canvas, paint);
  }
  return false;
}

static const auto* flow_type = "RasterCacheFlow::DisplayList";

bool DisplayListRasterCacheItem::TryToPrepareRasterCache(
    const PaintContext& context) const {
  if (!context.raster_cache) {
    return false;
  }
  if (cache_state_ != kNone &&
      num_cache_attempts_ >= context.raster_cache->access_threshold()) {
    auto transformation_matrix = transformation_matrix_;
// GetIntegralTransCTM effect for matrix which only contains scale,
// translate, so it won't affect result of matrix decomposition and cache
// key.
#ifndef SUPPORT_FRACTIONAL_TRANSLATION
    transformation_matrix =
        RasterCacheUtil::GetIntegralTransCTM(transformation_matrix);
#endif
    SkRect bounds =
        display_list_->bounds().makeOffset(offset_.x(), offset_.y());
    RasterCache::Context r_context = {
        // clang-format off
      .gr_context         = context.gr_context,
      .dst_color_space    = context.dst_color_space,
      .matrix             = transformation_matrix,
      .logical_rect       = bounds,
      .flow_type          = flow_type,
      .checkerboard       = context.checkerboard_offscreen_layers,
        // clang-format on
    };
    return context.raster_cache->UpdateCacheEntry(
        GetId().value(), r_context,
        [=](SkCanvas* canvas) { display_list_->RenderTo(canvas); });
  }
  return false;
}
}  // namespace flutter
