// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/flow/layers/cacheable_layer.h"
#include "flutter/flow/raster_cache.h"

namespace flutter {

AutoCache::AutoCache(RasterCacheItem* raster_cache_item,
                     PrerollContext* context,
                     const SkMatrix& matrix)
    : raster_cache_item_(raster_cache_item),
      context_(context),
      matrix_(matrix) {
  if (context_ && context_->raster_cache) {
    current_index_ = context_->raster_cached_entries->size();
  }
}

AutoCache::~AutoCache() {
  raster_cache_item_->PrerollFinalize(context_, matrix_);
}

AutoCache AutoCache::Create(LayerRasterCacheItem* raster_cache_item,
                            PrerollContext* context,
                            const SkMatrix& matrix) {
  raster_cache_item->PrerollSetup(context, matrix);
  return AutoCache(raster_cache_item, context, matrix);
}

AutoCache AutoCache::Create(DisplayListRasterCacheItem* raster_cache_item,
                            PrerollContext* context,
                            const SkMatrix& matrix) {
  if (context->raster_cache) {
    raster_cache_item->PrerollSetup(context, matrix);
  }
  return AutoCache(raster_cache_item, context, matrix);
}

AutoCache AutoCache::Create(SkPictureRasterCacheItem* raster_cache_item,
                            PrerollContext* context,
                            const SkMatrix& matrix) {
  if (context->raster_cache) {
    raster_cache_item->PrerollSetup(context, matrix);
  }
  return AutoCache(raster_cache_item, context, matrix);
}

CacheableContainerLayer::CacheableContainerLayer(int layer_cached_threshold,
                                                 bool can_cache_children) {
  layer_raster_cache_item_ = RasterCacheItem::MakeLayerRasterCacheItem(
      this, layer_cached_threshold, can_cache_children);
}

}  // namespace flutter
