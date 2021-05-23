// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/common/vsync_waiter_fallback.h"

#include "flutter/fml/logging.h"

namespace flutter {
namespace {

static fml::TimePoint SnapToNextTick(fml::TimePoint value,
                                     fml::TimePoint tick_phase,
                                     fml::TimeDelta tick_interval) {
  fml::TimeDelta offset = (tick_phase - value) % tick_interval;
  if (offset != fml::TimeDelta::Zero())
    offset = offset + tick_interval;
  return value + offset;
}

}  // namespace

VsyncWaiterFallback::VsyncWaiterFallback(TaskRunners task_runners)
    : VsyncWaiter(std::move(task_runners)), phase_(fml::TimePoint::Now()) {}

VsyncWaiterFallback::~VsyncWaiterFallback() = default;

// |VsyncWaiter|
void VsyncWaiterFallback::AwaitVSync() {
  constexpr fml::TimeDelta kSingleFrameInterval =
      fml::TimeDelta::FromSecondsF(1.0 / 60.0);

  auto next =
      SnapToNextTick(fml::TimePoint::Now(), phase_, kSingleFrameInterval);
  // BD MOD: START
  // FireCallback(next, next + kSingleFrameInterval);
  // Fix flutter test failed problem: "Shell subprocess crashed with segmentation fault after tests finished."
  // Call FireCallback() in PostTask to avoid stack overflow
  // View this wiki for more detail: https://bytedance.feishu.cn/wiki/wikcntkzgBuSIowTVUFZw62n64f
  task_runners_.GetUITaskRunner()->PostTask([this, next, kSingleFrameInterval]() {
    FireCallback(next, next + kSingleFrameInterval);
  });
  // END
}

}  // namespace flutter
