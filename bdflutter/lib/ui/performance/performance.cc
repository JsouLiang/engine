// BD ADD

#include <flutter/fml/make_copyable.h>
#include "performance.h"
#include "flutter/flow/instrumentation.h"
#include "flutter/lib/ui/window/platform_configuration.h"
#include "flutter/lib/ui/ui_dart_state.h"
#include "third_party/dart/runtime/include/dart_api.h"
#include "third_party/tonic/converter/dart_converter.h"
#include "third_party/tonic/dart_args.h"
#include "third_party/tonic/dart_library_natives.h"
#include "third_party/tonic/logging/dart_invoke.h"
#include "bdflutter/common/fps_recorder.h"
#include "boost.h"

using tonic::DartConverter;
using tonic::ToDart;

namespace flutter {
static std::mutex mtx;
Performance::Performance():
    dart_image_memory_usage(0) {}

int64_t Performance::GetImageMemoryUsageKB() {
  int64_t sizeInKB = dart_image_memory_usage.load(std::memory_order_relaxed);
  return  sizeInKB > 0 ? sizeInKB : 0;
}

void Performance::SubImageMemoryUsage(int64_t sizeInKB) {
  dart_image_memory_usage.fetch_sub(sizeInKB, std::memory_order_relaxed);
}

void Performance::AddImageMemoryUsage(int64_t sizeInKB) {
  dart_image_memory_usage.fetch_add(sizeInKB, std::memory_order_relaxed);
}

void Performance::DisableMips(bool disable) {
  disable_mipmaps_ = disable;
}

bool Performance::IsDisableMips(){
  return disable_mipmaps_ ;
}

void Performance_imageMemoryUsage(Dart_NativeArguments args) {
  Dart_SetReturnValue(args, tonic::DartConverter<int64_t>::ToDart(
      Performance::GetInstance()->GetImageMemoryUsageKB()));
}

void Performance_getEngineMainEnterMicros(Dart_NativeArguments args) {
  Dart_SetIntegerReturnValue(args,
                             UIDartState::Current()->platform_configuration()->client()->GetEngineMainEnterMicros());
}

void AddNextFrameCallback(Dart_Handle callback) {
  UIDartState* dart_state = UIDartState::Current();
  if (!dart_state->platform_configuration()) {
    return;
  }

  tonic::DartPersistentValue* next_frame_callback =
      new tonic::DartPersistentValue(dart_state, callback);
  dart_state->platform_configuration()->client()->AddNextFrameCallback(
      [next_frame_callback]() mutable {
        std::shared_ptr<tonic::DartState> dart_state_ =
            next_frame_callback->dart_state().lock();
        if (!dart_state_) {
          return;
        }
        tonic::DartState::Scope scope(dart_state_);
        tonic::DartInvokeVoid(next_frame_callback->value());

        // next_frame_callback is associated with the Dart isolate and must be
        // deleted on the UI thread
        delete next_frame_callback;
      });
}

void Performance_addNextFrameCallback(Dart_NativeArguments args) {
  tonic::DartCall(&AddNextFrameCallback, args);
}

void Performance_startRecordFps(Dart_NativeArguments args) {
  Dart_Handle exception = nullptr;
  std::string key = tonic::DartConverter<std::string>::FromArguments(args, 1, exception);
  if (exception) {
    Dart_ThrowException(exception);
    return;
  }
  FpsRecorder::Current()->StartRecordFps(key);
}

void Performance_obtainFps(Dart_NativeArguments args) {
  Dart_Handle exception = nullptr;
  std::string key = tonic::DartConverter<std::string>::FromArguments(args, 1, exception);
  if (exception) {
    Dart_ThrowException(exception);
    return;
  }
  bool stop_record = tonic::DartConverter<bool>::FromArguments(args, 2, exception);
  if (exception) {
    Dart_ThrowException(exception);
    return;
  }
  std::vector<double> fpsInfo = FpsRecorder::Current()->ObtainFpsData(key, stop_record);
  int size = 3;
  Dart_Handle data_handle = Dart_NewList(size);
  for (int i = 0; i != size; i++) {
    Dart_Handle value = Dart_NewDouble(fpsInfo[i]);
    Dart_ListSetAt(data_handle, i, value);
  }
  Dart_SetReturnValue(args, data_handle);
}

void Performance_getMaxSamples(Dart_NativeArguments args) {
  int max_samples = Stopwatch::GetMaxSamples();
  Dart_SetIntegerReturnValue(args, max_samples);
}

void performance_getFps(Dart_NativeArguments args) {
  Dart_Handle exception = nullptr;
  int thread_type =
      tonic::DartConverter<int>::FromArguments(args, 1, exception);
  int fps_type = tonic::DartConverter<int>::FromArguments(args, 2, exception);
  bool do_clear = tonic::DartConverter<bool>::FromArguments(args, 3, exception);
  std::vector<double> fpsInfo = UIDartState::Current()->platform_configuration()->client()->GetFps(
      thread_type, fps_type, do_clear);
  Dart_Handle data_handle = Dart_NewList(fpsInfo.size());
  for(std::vector<int>::size_type i = 0; i != fpsInfo.size(); i++) {
    Dart_Handle value = Dart_NewDouble(fpsInfo[i]);
    Dart_ListSetAt(data_handle, i, value);
  }
  Dart_SetReturnValue(args, data_handle);
}

void Performance_startBoost(Dart_NativeArguments args) {
  uint16_t flags = static_cast<uint16_t>(tonic::DartConverter<int>::FromDart(
      Dart_GetNativeArgument(args, 1)));
  int millis = tonic::DartConverter<int>::FromDart(Dart_GetNativeArgument(args, 2));
  Boost::Current()->StartUp(flags, millis);
}

void Performance_finishBoost(Dart_NativeArguments args) {
  uint16_t flags = static_cast<uint16_t>(tonic::DartConverter<int>::FromDart(
      Dart_GetNativeArgument(args, 1)));
  Boost::Current()->Finish(flags);
}

void Performance_forceGC(Dart_NativeArguments args) {
  Boost::Current()->ForceGC();
}

void Performance_disableMips(Dart_NativeArguments args) {
  bool disable = DartConverter<bool>::FromDart(Dart_GetNativeArgument(args, 1));
  Performance::GetInstance()->DisableMips(disable);
}

void Performance_startStackTraceSamples(Dart_NativeArguments args) {
  #if THIRD_PARTY_DART_BD
  Dart_StartProfiling2();
  #else
  Dart_StartProfiling();
  #endif
}

void Performance_stopStackTraceSamples(Dart_NativeArguments args) {
  #if THIRD_PARTY_DART_BD
  Dart_StopProfiling2();
  #else
  Dart_StopProfiling();
  #endif
}

void Performance_getStackTraceSamples(Dart_NativeArguments args) {
  #if THIRD_PARTY_DART_BD
  int64_t microseconds = (int64_t)DartConverter<int64_t>::FromDart(Dart_GetNativeArgument(args, 0));
  Dart_Handle res = Dart_GetStackSamples(microseconds);
  Dart_SetReturnValue(args, res);
  #endif
}

void Performance_requestHeapSnapshot(Dart_NativeArguments args) {
  const char* filePath = nullptr;
  Dart_StringToCString(Dart_GetNativeArgument(args, 0), &filePath);
  #if THIRD_PARTY_DART_BD
  Dart_Handle res = Dart_RequestSnapshot(filePath);
  Dart_SetReturnValue(args, res);
  #endif
}

void Performance_heapInfo(Dart_NativeArguments args) {
  #if THIRD_PARTY_DART_BD
  Dart_SetReturnValue(args, Dart_HeapInfo());
  #endif
}

int64_t Performance::CurrentTimestamp() {
  return chrono::system_clock::now().time_since_epoch().count();
}

void Performance::TraceApmStartAndEnd(const string& event, int64_t start) {
  std::lock_guard<std::mutex> lck (mtx);
  apm_map[event + "_start"] = start;
  apm_map[event + "_end"] = CurrentTimestamp();
}

vector<int64_t> Performance::GetEngineInitApmInfo() {
  std::lock_guard<std::mutex> lck (mtx);
  vector<int64_t> engine_init_apm_info = {
    apm_map["init_task_start"],
    apm_map["init_task_end"],
    apm_map["native_init_start"],
    apm_map["native_init_end"],
    apm_map["native_init_end"], // ber_shell_create_start
    apm_map["log_icu_init_start"],  // ber_shell_create_end
    apm_map["log_icu_init_start"],
    apm_map["log_icu_init_end"],
    apm_map["dartvm_create_start"],
    apm_map["dartvm_create_end"],
    apm_map["platform_view_init_start"],
    apm_map["platform_view_init_end"],
    apm_map["vsync_waiter_start"],
    apm_map["vsync_waiter_end"],
    apm_map["rasterizer_init_start"],
    apm_map["rasterizer_init_end"],
    apm_map["shell_io_manger_start"],
    apm_map["shell_io_manger_end"],
    apm_map["ui_animator_start"],
    apm_map["read_isolate_snapshort_start"], // animator_pre_end
    apm_map["read_isolate_snapshort_start"],
    apm_map["read_isolate_snapshort_end"],
    apm_map["read_isolate_snapshort_end"], // animator_after_start
    apm_map["ui_animator_end"],
    apm_map["shell_wait_start"],
    apm_map["shell_wait_end"],
    apm_map["plugin_registry_start"],
    apm_map["plugin_registry_end"],
    apm_map["execute_dart_entry_start"],
    apm_map["execute_dart_entry_end"],
  };
  return engine_init_apm_info;
}

void Performance_getEngineInitApmInfo(Dart_NativeArguments args) {
  vector<int64_t> info = Performance::GetInstance()->GetEngineInitApmInfo();
  Dart_Handle result = Dart_NewList(info.size());
  for (size_t index = 0; index < info.size(); ++index) {
    Dart_ListSetAt(result, index, Dart_NewInteger(info[index]));
  }
  Dart_SetReturnValue(args, result);
}


  void Performance::GetSkGraphicMemUsageKB(int64_t* bitmapMem,
                                         int64_t* fontMem, int64_t* imageFilter, int64_t* mallocSize) {
  *bitmapMem = SkGraphics::GetResourceCacheTotalBytesUsed() >> 10;
  *fontMem = SkGraphics::GetFontCacheUsed() >> 10;
  *imageFilter = SkGraphics::GetImageFilterCacheUsed() >> 10;
  *mallocSize = SkGraphics::GetSKMallocMemSize();
  if (*mallocSize >= 0) {
    *mallocSize = *mallocSize >> 10;
  } else {
    *mallocSize = 0;
  }
}

void Performance::GetGpuCacheUsageKB(int64_t* grTotalMem,
                                     int64_t* grResMem, int64_t* grPurgeableMem) {
  if (rasterizer_ && rasterizer_.get()) {
    size_t totalBytes = 0;
    size_t resourceBytes = 0;
    size_t purgeableBytes = 0;

#if defined(OS_IOS) || defined(OS_ANDROID)
    rasterizer_->getResourceCacheBytes(&totalBytes, &resourceBytes, &purgeableBytes);
#endif
    *grTotalMem = totalBytes >> 10;
    *grResMem = resourceBytes >> 10;
    *grPurgeableMem = purgeableBytes >> 10;
  }
}

void Performance::GetIOGpuCacheUsageKB(int64_t* grTotalMem,
                                       int64_t* grResMem, int64_t* grPurgeableMem) {
#if defined(OS_IOS) || defined(OS_ANDROID)
  if (iOManager_ && iOManager_.get() && iOManager_->GetResourceContext()) {
    size_t totalBytes = 0;
    size_t resourceBytes = 0;
    size_t purgeableBytes = 0;
    iOManager_->GetResourceContext()->getResourceCacheBytes(&totalBytes, &resourceBytes, &purgeableBytes);
    *grTotalMem = totalBytes >> 10;
    *grResMem = resourceBytes >> 10;
    *grPurgeableMem = purgeableBytes >> 10;
  }
#endif
}

void Performance_skGraphicCacheMemoryUsage(Dart_NativeArguments args) {
  int64_t bitmapMem = 0;
  int64_t fontMem = 0;
  int64_t imageFilter = 0;
  int64_t mallocSize = 0;
  Performance::GetInstance()->GetSkGraphicMemUsageKB(&bitmapMem, &fontMem, &imageFilter, &mallocSize);
  Dart_Handle data_handle = Dart_NewList(4);
  Dart_ListSetAt(data_handle, 0, Dart_NewInteger(bitmapMem));
  Dart_ListSetAt(data_handle, 1, Dart_NewInteger(fontMem));
  Dart_ListSetAt(data_handle, 2, Dart_NewInteger(imageFilter));
  Dart_ListSetAt(data_handle, 3, Dart_NewInteger(mallocSize));
  Dart_SetReturnValue(args, data_handle);
}

void Performance_getGpuCacheUsageKBInfo(Dart_NativeArguments args) {
  Dart_Handle callback_handle = Dart_GetNativeArgument(args, 1);
  if (!Dart_IsClosure(callback_handle)) {
    Dart_SetReturnValue(args, ToDart("Callback must be a function"));
    return;
  }
  if (Performance::GetInstance()->IsExitApp()) {
    return;
  }
  auto* dart_state = UIDartState::Current();

  const auto& task_runners = dart_state->GetTaskRunners();
  task_runners.GetIOTaskRunner()->PostTask(fml::MakeCopyable(
    [callback = std::make_unique<tonic::DartPersistentValue>(
      tonic::DartState::Current(), callback_handle),
      ui_task_runner = task_runners.GetUITaskRunner(),
      gpu_task_runner = task_runners.GetRasterTaskRunner()]() mutable {
      int64_t iOGrTotalMem = 0;
      int64_t iOGrResMem = 0;
      int64_t iOGrPurgeableMem = 0;
      Performance::GetInstance()->GetIOGpuCacheUsageKB(&iOGrTotalMem, &iOGrResMem, &iOGrPurgeableMem);
      if (!gpu_task_runner.get() || Performance::GetInstance()->IsExitApp()) {
        ui_task_runner->PostTask(fml::MakeCopyable(
          [callback = std::move(callback)]() { callback->Clear(); }));
        return;
      }
      gpu_task_runner->PostTask(fml::MakeCopyable(
        [callback = std::move(callback), ui_task_runner, iOGrTotalMem, iOGrResMem, iOGrPurgeableMem]() mutable {
          if (ui_task_runner.get()) {
            if (Performance::GetInstance()->IsExitApp()) {
              ui_task_runner->PostTask(fml::MakeCopyable(
                [callback = std::move(callback)]() { callback->Clear(); }));
              return;
            }
            int64_t grTotalMem = 0;
            int64_t grResMem = 0;
            int64_t grPurgeableMem = 0;
            Performance::GetInstance()->GetGpuCacheUsageKB(&grTotalMem, &grResMem, &grPurgeableMem);
            ui_task_runner->PostTask(fml::MakeCopyable(
              [callback = std::move(callback), grTotalMem, grResMem, grPurgeableMem, iOGrTotalMem, iOGrResMem, iOGrPurgeableMem]() {
                std::shared_ptr<tonic::DartState> dart_state = callback->dart_state().lock();
                if (!dart_state || !dart_state.get() || Performance::GetInstance()->IsExitApp()) {
                  callback->Clear();
                  return;
                }
                tonic::DartState::Scope scope(dart_state);
                Dart_Handle data_handle = Dart_NewList(6);
                Dart_ListSetAt(data_handle, 0, Dart_NewInteger(grTotalMem));
                Dart_ListSetAt(data_handle, 1, Dart_NewInteger(grResMem));
                Dart_ListSetAt(data_handle, 2, Dart_NewInteger(grPurgeableMem));
                Dart_ListSetAt(data_handle, 3, Dart_NewInteger(iOGrTotalMem));
                Dart_ListSetAt(data_handle, 4, Dart_NewInteger(iOGrResMem));
                Dart_ListSetAt(data_handle, 5, Dart_NewInteger(iOGrPurgeableMem));
                tonic::DartInvoke(callback->value(), {data_handle});
              }));
          }
        }));
    }));
}

void Performance_getTotalExtMemInfo(Dart_NativeArguments args) {
  Dart_Handle callback_handle = Dart_GetNativeArgument(args, 1);
  if (!Dart_IsClosure(callback_handle)) {
    Dart_SetReturnValue(args, ToDart("Callback must be a function"));
    return;
  }
  Performance* performance = Performance::GetInstance();
  if (performance->IsExitApp()) {
    return;
  }
  auto* dart_state = UIDartState::Current();

  const auto& task_runners = dart_state->GetTaskRunners();
  int64_t imageMem = performance->GetImageMemoryUsageKB();
  int64_t bitmapMem = 0;
  int64_t fontMem = 0;
  int64_t imageFilter = 0;
  int64_t mallocSize = 0;
  performance->GetSkGraphicMemUsageKB(&bitmapMem, &fontMem, &imageFilter, &mallocSize);

  task_runners.GetIOTaskRunner()->PostTask(fml::MakeCopyable(
    [callback = std::make_unique<tonic::DartPersistentValue>(
      tonic::DartState::Current(), callback_handle),
      ui_task_runner = task_runners.GetUITaskRunner(),
      gpu_task_runner = task_runners.GetRasterTaskRunner(), imageMem, bitmapMem, fontMem, imageFilter, mallocSize]() mutable {
      if (!gpu_task_runner.get() || Performance::GetInstance()->IsExitApp()) {
        ui_task_runner->PostTask(fml::MakeCopyable(
          [callback = std::move(callback)]() { callback->Clear(); }));
        return;
      }
      int64_t iOGrTotalMem = 0;
      int64_t iOGrResMem = 0;
      int64_t iOGrPurgeableMem = 0;
      Performance::GetInstance()->GetIOGpuCacheUsageKB(&iOGrTotalMem, &iOGrResMem, &iOGrPurgeableMem);
      gpu_task_runner->PostTask(fml::MakeCopyable(
        [callback = std::move(callback),
          ui_task_runner, imageMem, bitmapMem, fontMem, imageFilter, iOGrTotalMem, iOGrResMem, iOGrPurgeableMem, mallocSize
        ]() mutable {
          if (ui_task_runner.get()) {
            if (Performance::GetInstance()->IsExitApp()) {
              ui_task_runner->PostTask(fml::MakeCopyable(
                [callback = std::move(callback)]() { callback->Clear(); }));
              return;
            }
            int64_t grTotalMem = 0;
            int64_t grResMem = 0;
            int64_t grPurgeableMem = 0;
            Performance::GetInstance()->GetGpuCacheUsageKB(&grTotalMem, &grResMem, &grPurgeableMem);
            ui_task_runner->PostTask(fml::MakeCopyable(
              [callback = std::move(callback), imageMem, bitmapMem, fontMem, imageFilter, grTotalMem,
                grResMem, grPurgeableMem, iOGrTotalMem, iOGrResMem, iOGrPurgeableMem, mallocSize]() {
                std::shared_ptr<tonic::DartState> dart_state = callback->dart_state().lock();
                if (!dart_state || !dart_state.get() || Performance::GetInstance()->IsExitApp()) {
                  callback->Clear();
                  return;
                }
                tonic::DartState::Scope scope(dart_state);
                Dart_Handle data_handle = Dart_NewList(11);
                Dart_ListSetAt(data_handle, 0, Dart_NewInteger(imageMem));
                Dart_ListSetAt(data_handle, 1, Dart_NewInteger(grTotalMem));
                Dart_ListSetAt(data_handle, 2, Dart_NewInteger(grResMem));
                Dart_ListSetAt(data_handle, 3, Dart_NewInteger(grPurgeableMem));
                Dart_ListSetAt(data_handle, 4, Dart_NewInteger(iOGrTotalMem));
                Dart_ListSetAt(data_handle, 5, Dart_NewInteger(iOGrResMem));
                Dart_ListSetAt(data_handle, 6, Dart_NewInteger(iOGrPurgeableMem));
                Dart_ListSetAt(data_handle, 7, Dart_NewInteger(bitmapMem));
                Dart_ListSetAt(data_handle, 8, Dart_NewInteger(fontMem));
                Dart_ListSetAt(data_handle, 9, Dart_NewInteger(imageFilter));
                Dart_ListSetAt(data_handle, 10, Dart_NewInteger(mallocSize));
                tonic::DartInvoke(callback->value(), {data_handle});
              }));
          }
        }));
    }));
}

void Performance::SetRasterizerAndIOManager(fml::TaskRunnerAffineWeakPtr<flutter::Rasterizer> rasterizer,
                                            fml::WeakPtr<flutter::ShellIOManager> ioManager) {
  rasterizer_ = std::move(rasterizer);
  iOManager_ = std::move(ioManager);
  isExitApp_ = false;
}

void Performance::SetExitStatus(bool isExitApp) {
  isExitApp_ = isExitApp;
}

bool Performance::IsExitApp() {
  return isExitApp_;
}

void Performance::RegisterNatives(tonic::DartLibraryNatives* natives) {
  natives->Register({
      {"Performance_imageMemoryUsage", Performance_imageMemoryUsage, 1, true},
      {"Performance_getEngineMainEnterMicros", Performance_getEngineMainEnterMicros, 1, true},
      {"Performance_addNextFrameCallback", Performance_addNextFrameCallback, 2, true},
      {"Performance_startRecordFps", Performance_startRecordFps, 2, true},
      {"Performance_obtainFps", Performance_obtainFps, 3, true},
      {"Performance_getMaxSamples", Performance_getMaxSamples, 1, true},
      {"performance_getFps", performance_getFps, 4, true},
      {"Performance_startBoost", Performance_startBoost, 3, true},
      {"Performance_finishBoost", Performance_finishBoost, 2, true},
      {"Performance_forceGC", Performance_forceGC, 1, true},
      {"Performance_disableMips", Performance_disableMips, 2, true},
      {"Performance_startStackTraceSamples", Performance_startStackTraceSamples, 1, true},
      {"Performance_stopStackTraceSamples", Performance_stopStackTraceSamples, 1, true},
      {"Performance_getStackTraceSamples", Performance_getStackTraceSamples, 2, true},
      {"Performance_requestHeapSnapshot", Performance_requestHeapSnapshot, 2, true},
      {"Performance_heapInfo", Performance_heapInfo, 1, true},
      {"Performance_getEngineInitApmInfo", Performance_getEngineInitApmInfo, 1, true},
      {"Performance_getTotalExtMemInfo", Performance_getTotalExtMemInfo, 2, true},
      {"Performance_skGraphicCacheMemoryUsage", Performance_skGraphicCacheMemoryUsage, 1, true},
      {"Performance_getGpuCacheUsageKBInfo", Performance_getGpuCacheUsageKBInfo, 2, true},
  });
}

}
