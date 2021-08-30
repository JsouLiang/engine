// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#define RAPIDJSON_HAS_STDSTRING 1
#include "flutter/shell/common/shell.h"

#include <memory>
#include <sstream>
#include <vector>

#include "flutter/assets/directory_asset_bundle.h"
#include "flutter/fml/file.h"
#include "flutter/fml/icu_util.h"
#include "flutter/fml/log_settings.h"
#include "flutter/fml/logging.h"
#include "flutter/fml/make_copyable.h"
#include "flutter/fml/message_loop.h"
#include "flutter/fml/paths.h"
#include "flutter/fml/trace_event.h"
#include "flutter/fml/unique_fd.h"
#include "flutter/runtime/dart_vm.h"
#include "flutter/runtime/start_up.h"
#include "flutter/shell/common/engine.h"
#include "flutter/shell/common/persistent_cache.h"
#include "flutter/shell/common/skia_event_tracer_impl.h"
#include "flutter/shell/common/switches.h"
#include "flutter/shell/common/vsync_waiter.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
// BD ADD:
#include "flutter/lib/ui/boost.h"
#include "flutter/lib/ui/performance.h"
// END
#include "third_party/dart/runtime/include/dart_tools_api.h"
#include "third_party/skia/include/core/SkGraphics.h"
#include "third_party/tonic/common/log.h"
#include "flutter/lib/ui/performance.h"

namespace flutter {

constexpr char kSkiaChannel[] = "flutter/skia";
constexpr char kSystemChannel[] = "flutter/system";
constexpr char kTypeKey[] = "type";
constexpr char kFontChange[] = "fontsChange";

std::unique_ptr<Shell> Shell::CreateShellOnPlatformThread(
    DartVMRef vm,
    TaskRunners task_runners,
    Settings settings,
    fml::RefPtr<const DartSnapshot> isolate_snapshot,
    const Shell::CreateCallback<PlatformView>& on_create_platform_view,
    const Shell::CreateCallback<Rasterizer>& on_create_rasterizer,
    // BD ADD:
    bool preLoad) {
  if (!task_runners.IsValid()) {
    FML_LOG(ERROR) << "Task runners to run the shell were invalid.";
    return nullptr;
  }

  auto shell =
      std::unique_ptr<Shell>(new Shell(std::move(vm), task_runners, settings));
  // BD ADD:
  shell->SetPreloadState(preLoad);

  // Create the rasterizer on the GPU thread.
  std::promise<std::unique_ptr<Rasterizer>> rasterizer_promise;
  auto rasterizer_future = rasterizer_promise.get_future();
  std::promise<fml::WeakPtr<SnapshotDelegate>> snapshot_delegate_promise;
  auto snapshot_delegate_future = snapshot_delegate_promise.get_future();
  fml::TaskRunner::RunNowOrPostTask(
      task_runners.GetGPUTaskRunner(), [&rasterizer_promise,  //
                                        &snapshot_delegate_promise,
                                        on_create_rasterizer,  //
                                        shell = shell.get()    //
  ]() {
        TRACE_EVENT0("flutter", "ShellSetupGPUSubsystem");
        // BD ADD:
        int64_t rasterizer_init_start_timestamp = Performance::CurrentTimestamp();
        std::unique_ptr<Rasterizer> rasterizer(on_create_rasterizer(*shell));
        snapshot_delegate_promise.set_value(rasterizer->GetSnapshotDelegate());
        rasterizer_promise.set_value(std::move(rasterizer));
        // BD ADD:
        Performance::GetInstance()->TraceApmStartAndEnd("rasterizer_init", rasterizer_init_start_timestamp);
      });

  // Create the platform view on the platform thread (this thread).
  // BD ADD:
  int64_t platform_view_create_start_timestamp = Performance::CurrentTimestamp();
  auto platform_view = on_create_platform_view(*shell.get());
  // BD ADD:
  Performance::GetInstance()->TraceApmStartAndEnd("platform_view_init", platform_view_create_start_timestamp);
  if (!platform_view || !platform_view->GetWeakPtr()) {
    return nullptr;
  }

  // Ask the platform view for the vsync waiter. This will be used by the engine
  // to create the animator.
  // BD ADD:
  int64_t vsync_waiter_create_start_timestamp = Performance::CurrentTimestamp();
  auto vsync_waiter = platform_view->CreateVSyncWaiter();
  // BD ADD:
  Performance::GetInstance()->TraceApmStartAndEnd("vsync_waiter", vsync_waiter_create_start_timestamp);
  if (!vsync_waiter) {
    return nullptr;
  }

  // Create the IO manager on the IO thread. The IO manager must be initialized
  // first because it has state that the other subsystems depend on. It must
  // first be booted and the necessary references obtained to initialize the
  // other subsystems.
  std::promise<std::unique_ptr<ShellIOManager>> io_manager_promise;
  auto io_manager_future = io_manager_promise.get_future();
  std::promise<fml::WeakPtr<ShellIOManager>> weak_io_manager_promise;
  auto weak_io_manager_future = weak_io_manager_promise.get_future();
  std::promise<fml::RefPtr<SkiaUnrefQueue>> unref_queue_promise;
  auto unref_queue_future = unref_queue_promise.get_future();
  auto io_task_runner = shell->GetTaskRunners().GetIOTaskRunner();

  // TODO(gw280): The WeakPtr here asserts that we are derefing it on the
  // same thread as it was created on. We are currently on the IO thread
  // inside this lambda but we need to deref the PlatformView, which was
  // constructed on the platform thread.
  //
  // https://github.com/flutter/flutter/issues/42948
  fml::TaskRunner::RunNowOrPostTask(
      io_task_runner,
      [&io_manager_promise,                                               //
       &weak_io_manager_promise,                                          //
       &unref_queue_promise,                                              //
       platform_view = platform_view->GetWeakPtr(),                       //
       io_task_runner,                                                    //
       is_backgrounded_sync_switch = shell->GetIsGpuDisabledSyncSwitch()  //
  ]() {
        TRACE_EVENT0("flutter", "ShellSetupIOSubsystem");
        // BD ADD:
        int64_t io_manager_create_timestamp = Performance::CurrentTimestamp();
        auto io_manager = std::make_unique<ShellIOManager>(
            platform_view.getUnsafe()->CreateResourceContext(),
            is_backgrounded_sync_switch, io_task_runner);
        weak_io_manager_promise.set_value(io_manager->GetWeakPtr());
        unref_queue_promise.set_value(io_manager->GetSkiaUnrefQueue());
        io_manager_promise.set_value(std::move(io_manager));
        // BD ADD:
        Performance::GetInstance()->TraceApmStartAndEnd("shell_io_manger", io_manager_create_timestamp);
      });

  // Send dispatcher_maker to the engine constructor because shell won't have
  // platform_view set until Shell::Setup is called later.
  auto dispatcher_maker = platform_view->GetDispatcherMaker();

  // Create the engine on the UI thread.
  std::promise<std::unique_ptr<Engine>> engine_promise;
  auto engine_future = engine_promise.get_future();
  // BD ADD:
  fml::AutoResetWaitableEvent latch;

  fml::TaskRunner::RunNowOrPostTask(
      shell->GetTaskRunners().GetUITaskRunner(),
      fml::MakeCopyable([&engine_promise,                                 //
                         shell = shell.get(),                             //
                         isolate_snapshot = std::move(isolate_snapshot),  //
                         vsync_waiter = std::move(vsync_waiter),          //
                         &weak_io_manager_future,                         //
                         &snapshot_delegate_future,                       //
                         &unref_queue_future,                              //
                         // BD ADD:
                         preLoad,
                         &latch,
                         dispatcher_maker = std::move(dispatcher_maker)
  ]() mutable {

        TRACE_EVENT0("flutter", "ShellSetupUISubsystem");
        const auto& task_runners = shell->GetTaskRunners();
        // BD ADD: START
        auto weak_io_manager = weak_io_manager_future.get();
        auto unref_queue = unref_queue_future.get();
        auto snapshot_delegate = snapshot_delegate_future.get();

        int64_t ui_animator_pre_start_timestamp = Performance::CurrentTimestamp();
        // BD ADD: END

        // The animator is owned by the UI thread but it gets its vsync pulses
        // from the platform.
        auto animator = std::make_unique<Animator>(*shell, task_runners,
                                                   std::move(vsync_waiter));

        // BD MOD: START
        // engine_promise.set_value(std::make_unique<Engine>(
        //     *shell,                         //
        //     dispatcher_maker,               //
        //     *shell->GetDartVM(),            //
        //     std::move(isolate_snapshot),    //
        //     task_runners,                   //
        //     shell->GetSettings(),           //
        //     std::move(animator),            //
        //     weak_io_manager_future.get(),   //
        //     unref_queue_future.get(),       //
        //     snapshot_delegate_future.get()  //
        //     ));
        if (preLoad) {
          latch.Signal();
          auto engine = std::make_unique<Engine>(
            *shell,                         //
            dispatcher_maker,               //
            *shell->GetDartVM(),            //
            std::move(isolate_snapshot),    //
            task_runners,                   //
            shell->GetSettings(),           //
            std::move(animator),            //
            weak_io_manager,   //
            unref_queue,       //
            snapshot_delegate  //
          );
          shell->SetupEngine(std::move(engine));
        } else {
           engine_promise.set_value(std::make_unique<Engine>(
               *shell,                         //
               dispatcher_maker,               //
               *shell->GetDartVM(),            //
               std::move(isolate_snapshot),    //
               task_runners,                   //
               shell->GetSettings(),           //
               std::move(animator),            //
               weak_io_manager,   //
               unref_queue,       //
               snapshot_delegate  //
               ));
        }
        Performance::GetInstance()->TraceApmStartAndEnd("ui_animator", ui_animator_pre_start_timestamp);
        // BD MOD: END
      }));

  // BD MOD: START
  // if (!shell->Setup(std::move(platform_view),  //
  //                   engine_future.get(),       //
  //                   rasterizer_future.get(),   //
  //                   io_manager_future.get())   //
  // ) {
  //   return nullptr;
  // }
  // BD ADD:
  int64_t shell_wait_start_timestamp = Performance::CurrentTimestamp();
  if (!preLoad) {
     if (!shell->Setup(std::move(platform_view),  //
                       engine_future.get(),       //
                       rasterizer_future.get(),   //
                       io_manager_future.get())   //
     ) {
       return nullptr;
     }
  } else {
    FML_LOG(ERROR) << "BDFlutter: Now is in shell_not_block mode";
    if (!shell->SetupWithoutEngine(std::move(platform_view),  //
                      rasterizer_future.get(),   //
                      io_manager_future.get())   //
      ) {
      return nullptr;
    }
    latch.Wait();
  }
  Performance::GetInstance()->SetRasterizerAndIOManager(
    shell->weak_rasterizer_, shell->io_manager_->GetWeakPtr());
  // BD ADD:
  Performance::GetInstance()->TraceApmStartAndEnd("shell_wait", shell_wait_start_timestamp);
  // END
  return shell;
}

static void RecordStartupTimestamp() {
  if (engine_main_enter_ts == 0) {
    engine_main_enter_ts = Dart_TimelineGetMicros();
  }
}

// BD ADD:
int64_t Shell::GetEngineMainEnterMicros() {
  return engine_main_enter_ts;
}
// END

// Though there can be multiple shells, some settings apply to all components in
// the process. These have to be setup before the shell or any of its
// sub-components can be initialized. In a perfect world, this would be empty.
// TODO(chinmaygarde): The unfortunate side effect of this call is that settings
// that cause shell initialization failures will still lead to some of their
// settings being applied.
static void PerformInitializationTasks(const Settings& settings) {
    // BD ADD:
    int64_t logIcuInitStartTimestamp = Performance::CurrentTimestamp();
  {
    fml::LogSettings log_settings;
    log_settings.min_log_level =
        settings.verbose_logging ? fml::LOG_INFO : fml::LOG_ERROR;
    fml::SetLogSettings(log_settings);
  }

  static std::once_flag gShellSettingsInitialization = {};
    // BD MOD: Start
    // std::call_once(gShellSettingsInitialization, [&settings] {
  std::call_once(gShellSettingsInitialization, [&settings, &logIcuInitStartTimestamp] {
    // BD MOD: END
    RecordStartupTimestamp();

    tonic::SetLogHandler(
        [](const char* message) { FML_LOG(ERROR) << message; });

    if (settings.trace_skia) {
      InitSkiaEventTracer(settings.trace_skia);
    }

    if (!settings.skia_deterministic_rendering_on_cpu) {
      SkGraphics::Init();
    } else {
      FML_DLOG(INFO) << "Skia deterministic rendering is enabled.";
    }

    if (settings.icu_initialization_required) {
      if (settings.icu_data_path.size() != 0) {
        fml::icu::InitializeICU(settings.icu_data_path);
      } else if (settings.icu_mapper) {
        fml::icu::InitializeICUFromMapping(settings.icu_mapper());
      } else {
        FML_DLOG(WARNING) << "Skipping ICU initialization in the shell.";
      }
    }

  });
  // BD ADD:
  Performance::GetInstance()->TraceApmStartAndEnd("log_icu_init", logIcuInitStartTimestamp);
}

// BD ADD: START
std::unique_ptr<Shell> Shell::Create(
  TaskRunners task_runners,
  Settings settings,
  const Shell::CreateCallback<PlatformView>& on_create_platform_view,
  const Shell::CreateCallback<Rasterizer>& on_create_rasterizer) {
  return Shell::Create(task_runners,             // task runners
                settings,                // settings
                on_create_platform_view,  // platform view create callback
                on_create_rasterizer,     // rasterizer create callback
                false
  );
}
// END

std::unique_ptr<Shell> Shell::Create(
    TaskRunners task_runners,
    Settings settings,
    const Shell::CreateCallback<PlatformView>& on_create_platform_view,
    const Shell::CreateCallback<Rasterizer>& on_create_rasterizer,
    // BD ADD:
    bool preLoad) {
  PerformInitializationTasks(settings);
  PersistentCache::SetCacheSkSL(settings.cache_sksl);

  TRACE_EVENT0("flutter", "Shell::Create");
  // BD ADD:
  int64_t dartvmCreateStartTimestamp = Performance::CurrentTimestamp();
  auto vm = DartVMRef::Create(settings);
  // BD ADD:
  Performance::GetInstance()->TraceApmStartAndEnd("dartvm_create", dartvmCreateStartTimestamp);
  FML_CHECK(vm) << "Must be able to initialize the VM.";

  auto vm_data = vm->GetVMData();

  return Shell::Create(std::move(task_runners),        //
                       std::move(settings),            //
                       vm_data->GetIsolateSnapshot(),  // isolate snapshot
                       on_create_platform_view,        //
                       on_create_rasterizer,           //
                       std::move(vm),                  //
                       preLoad
  );
}

std::unique_ptr<Shell> Shell::Create(
    TaskRunners task_runners,
    Settings settings,
    fml::RefPtr<const DartSnapshot> isolate_snapshot,
    const Shell::CreateCallback<PlatformView>& on_create_platform_view,
    const Shell::CreateCallback<Rasterizer>& on_create_rasterizer,
    DartVMRef vm,
    // BD ADD:
    bool preLoad) {
  PerformInitializationTasks(settings);
  PersistentCache::SetCacheSkSL(settings.cache_sksl);

  TRACE_EVENT0("flutter", "Shell::CreateWithSnapshots");

  if (!task_runners.IsValid() || !on_create_platform_view ||
      !on_create_rasterizer) {
    return nullptr;
  }

  fml::AutoResetWaitableEvent latch;
  std::unique_ptr<Shell> shell;
  fml::TaskRunner::RunNowOrPostTask(
      task_runners.GetPlatformTaskRunner(),
      fml::MakeCopyable([&latch,                                          //
                         vm = std::move(vm),                              //
                         &shell,                                          //
                         task_runners = std::move(task_runners),          //
                         settings,                                        //
                         isolate_snapshot = std::move(isolate_snapshot),  //
                         on_create_platform_view,                         //
                         on_create_rasterizer,                            //
                         // BD ADD:
                         preLoad
  ]() mutable {
        shell = CreateShellOnPlatformThread(std::move(vm),
                                            std::move(task_runners),      //
                                            settings,                     //
                                            std::move(isolate_snapshot),  //
                                            on_create_platform_view,      //
                                            on_create_rasterizer,         //
                                            // BD ADD:
                                            preLoad
        );
        latch.Signal();
      }));
  latch.Wait();
  return shell;
}

Shell::Shell(DartVMRef vm, TaskRunners task_runners, Settings settings)
    : task_runners_(std::move(task_runners)),
      settings_(std::move(settings)),
      vm_(std::move(vm)),
      is_gpu_disabled_sync_switch_(new fml::SyncSwitch()),
      weak_factory_(this),
      weak_factory_gpu_(nullptr) {
  FML_CHECK(vm_) << "Must have access to VM to create a shell.";
  FML_DCHECK(task_runners_.IsValid());
  FML_DCHECK(task_runners_.GetPlatformTaskRunner()->RunsTasksOnCurrentThread());

  // Generate a WeakPtrFactory for use with the GPU thread. This does not need
  // to wait on a latch because it can only ever be used from the GPU thread
  // from this class, so we have ordering guarantees.
  fml::TaskRunner::RunNowOrPostTask(
      task_runners_.GetGPUTaskRunner(), fml::MakeCopyable([this]() mutable {
        this->weak_factory_gpu_ =
            std::make_unique<fml::WeakPtrFactory<Shell>>(this);
      }));

  // Install service protocol handlers.

  service_protocol_handlers_[ServiceProtocol::kScreenshotExtensionName] = {
      task_runners_.GetGPUTaskRunner(),
      std::bind(&Shell::OnServiceProtocolScreenshot, this,
                std::placeholders::_1, std::placeholders::_2)};
  service_protocol_handlers_[ServiceProtocol::kScreenshotSkpExtensionName] = {
      task_runners_.GetGPUTaskRunner(),
      std::bind(&Shell::OnServiceProtocolScreenshotSKP, this,
                std::placeholders::_1, std::placeholders::_2)};
  service_protocol_handlers_[ServiceProtocol::kRunInViewExtensionName] = {
      task_runners_.GetUITaskRunner(),
      std::bind(&Shell::OnServiceProtocolRunInView, this, std::placeholders::_1,
                std::placeholders::_2)};
  service_protocol_handlers_
      [ServiceProtocol::kFlushUIThreadTasksExtensionName] = {
          task_runners_.GetUITaskRunner(),
          std::bind(&Shell::OnServiceProtocolFlushUIThreadTasks, this,
                    std::placeholders::_1, std::placeholders::_2)};
  service_protocol_handlers_
      [ServiceProtocol::kSetAssetBundlePathExtensionName] = {
          task_runners_.GetUITaskRunner(),
          std::bind(&Shell::OnServiceProtocolSetAssetBundlePath, this,
                    std::placeholders::_1, std::placeholders::_2)};
  service_protocol_handlers_
      [ServiceProtocol::kGetDisplayRefreshRateExtensionName] = {
          task_runners_.GetUITaskRunner(),
          std::bind(&Shell::OnServiceProtocolGetDisplayRefreshRate, this,
                    std::placeholders::_1, std::placeholders::_2)};
}

Shell::~Shell() {
  PersistentCache::GetCacheForProcess()->RemoveWorkerTaskRunner(
      task_runners_.GetIOTaskRunner());

  vm_->GetServiceProtocol()->RemoveHandler(this);

  fml::AutoResetWaitableEvent ui_latch, gpu_latch, platform_latch, io_latch;

  fml::TaskRunner::RunNowOrPostTask(
      task_runners_.GetUITaskRunner(),
      fml::MakeCopyable([engine = std::move(engine_), &ui_latch]() mutable {
        engine.reset();
        ui_latch.Signal();
      }));
  ui_latch.Wait();

  fml::TaskRunner::RunNowOrPostTask(
      task_runners_.GetGPUTaskRunner(),
      fml::MakeCopyable([rasterizer = std::move(rasterizer_),
                         weak_factory_gpu = std::move(weak_factory_gpu_),
                         &gpu_latch]() mutable {
        rasterizer.reset();
        weak_factory_gpu.reset();
        gpu_latch.Signal();
      }));
  gpu_latch.Wait();

  fml::TaskRunner::RunNowOrPostTask(
      task_runners_.GetIOTaskRunner(),
      fml::MakeCopyable([io_manager = std::move(io_manager_),
                         platform_view = platform_view_.get(),
                         &io_latch]() mutable {
        io_manager.reset();
        if (platform_view) {
          platform_view->ReleaseResourceContext();
        }
        io_latch.Signal();
      }));

  io_latch.Wait();

  // The platform view must go last because it may be holding onto platform side
  // counterparts to resources owned by subsystems running on other threads. For
  // example, the NSOpenGLContext on the Mac.
  fml::TaskRunner::RunNowOrPostTask(
      task_runners_.GetPlatformTaskRunner(),
      fml::MakeCopyable([platform_view = std::move(platform_view_),
                         &platform_latch]() mutable {
        platform_view.reset();
        platform_latch.Signal();
      }));
  platform_latch.Wait();
}

void Shell::NotifyLowMemoryWarning() const {
  task_runners_.GetGPUTaskRunner()->PostTask(
      [rasterizer = rasterizer_->GetWeakPtr()]() {
        if (rasterizer) {
          rasterizer->NotifyLowMemoryWarning();
        }
      });
  // The IO Manager uses resource cache limits of 0, so it is not necessary
  // to purge them.

  // BD ADD: START
  if (engine_) {
      engine_->NotifyLowMemoryWarning();
  }

  auto io_task = [io_manager = io_manager_->GetWeakPtr()]() {
      if (io_manager) {
          io_manager->GetSkiaUnrefQueue()->Drain();
          if (io_manager->GetResourceContext()) {
              io_manager->GetResourceContext()->freeGpuResources();
          }
      }
  };
  // Dart VM对象的释放：
  // 1.Dart_NotifyLowMemory()->Isolate::NotifyLowMemory()->Isolate::KillAllIsolates(LibMsgId
  // msg_id)->Isolate::KillLocked(LibMsgId msg_id)
  // 2.通过Isolate的Port，将kLowMemoryMsg加入消息队列
  // 3.在UI线程中执行IsolateMessageHandler::HandleLibMessage(const Array&
  // message)
  // 4.对于图片内存的释放需要确保图片sk_sp<SkImage>外面包裹的Codec、FrameInfo、CanvasImage先销毁
  task_runners_.GetUITaskRunner()->PostTask(
          [io_task_runner = task_runners_.GetIOTaskRunner(), io_task] {
              Performance::GetInstance()->ClearLayoutCache();
              io_task_runner->PostTask(io_task);
          });
  // END
}

void Shell::RunEngine(RunConfiguration run_configuration) {
  RunEngine(std::move(run_configuration), nullptr);
}

void Shell::RunEngine(
    RunConfiguration run_configuration,
    const std::function<void(Engine::RunStatus)>& result_callback) {
  auto result = [platform_runner = task_runners_.GetPlatformTaskRunner(),
                 result_callback](Engine::RunStatus run_result) {
    if (!result_callback) {
      return;
    }
    platform_runner->PostTask(
        [result_callback, run_result]() { result_callback(run_result); });
  };
  // BD MOD:
  // FML_DCHECK(is_setup_);
  FML_DCHECK(is_preload_ || is_setup_);
  FML_DCHECK(task_runners_.GetPlatformTaskRunner()->RunsTasksOnCurrentThread());

  //  BD MOD: START
  //  fml::TaskRunner::RunNowOrPostTask(
  //    task_runners_.GetUITaskRunner(),
  //    fml::MakeCopyable(
  //        [run_configuration = std::move(run_configuration),
  //         weak_engine = weak_engine_, result]() mutable {
  //          if (!weak_engine) {
  //            FML_LOG(ERROR)
  //                << "Could not launch engine with configuration - no engine.";
  //            result(Engine::RunStatus::Failure);
  //            return;
  //          }
  //          auto run_result = weak_engine->Run(std::move(run_configuration));
  //          if (run_result == flutter::Engine::RunStatus::Failure) {
  //            FML_LOG(ERROR) << "Could not launch engine with configuration.";
  //          }
  //          result(run_result);
  //        }));
  if (!is_setup_ && is_preload_) {
    fml::TaskRunner::RunNowOrPostTask(
      task_runners_.GetUITaskRunner(),
      fml::MakeCopyable(
        [run_configuration = std::move(run_configuration),
          shell = this, result]() mutable {
          if (!shell && shell->weak_engine_) {
            FML_LOG(ERROR)
            << "Could not launch engine with configuration - no engine.";
            result(Engine::RunStatus::Failure);
            return;
          }
          auto run_result = shell->weak_engine_->Run(std::move(run_configuration));
          if (run_result == flutter::Engine::RunStatus::Failure) {
            FML_LOG(ERROR) << "Could not launch engine with configuration.";
          }
          result(run_result);
        }));
  } else {
    fml::TaskRunner::RunNowOrPostTask(
      task_runners_.GetUITaskRunner(),
      fml::MakeCopyable(
        [run_configuration = std::move(run_configuration),
          weak_engine = weak_engine_, result]() mutable {
          if (!weak_engine) {
            FML_LOG(ERROR)
            << "Could not launch engine with configuration - no engine.";
            result(Engine::RunStatus::Failure);
            return;
          }
          auto run_result = weak_engine->Run(std::move(run_configuration));
          if (run_result == flutter::Engine::RunStatus::Failure) {
            FML_LOG(ERROR) << "Could not launch engine with configuration.";
          }
          result(run_result);
        }));
  }
  // END
}

std::optional<DartErrorCode> Shell::GetUIIsolateLastError() const {
  // BD MOD:
  // FML_DCHECK(is_setup_);
  FML_DCHECK(is_preload_ || is_setup_);
  FML_DCHECK(task_runners_.GetUITaskRunner()->RunsTasksOnCurrentThread());

  if (!weak_engine_) {
    return std::nullopt;
  }
  switch (weak_engine_->GetUIIsolateLastError()) {
    case tonic::kCompilationErrorType:
      return DartErrorCode::CompilationError;
    case tonic::kApiErrorType:
      return DartErrorCode::ApiError;
    case tonic::kUnknownErrorType:
      return DartErrorCode::UnknownError;
    case tonic::kNoError:
      return DartErrorCode::NoError;
  }
  return DartErrorCode::UnknownError;
}

bool Shell::EngineHasLivePorts() const {
  // BD MOD:
  // FML_DCHECK(is_setup_);
  FML_DCHECK(is_preload_ || is_setup_);
  FML_DCHECK(task_runners_.GetUITaskRunner()->RunsTasksOnCurrentThread());

  if (!weak_engine_) {
    return false;
  }

  return weak_engine_->UIIsolateHasLivePorts();
}

bool Shell::IsSetup() const {
  return is_setup_;
}

bool Shell::Setup(std::unique_ptr<PlatformView> platform_view,
                  std::unique_ptr<Engine> engine,
                  std::unique_ptr<Rasterizer> rasterizer,
                  std::unique_ptr<ShellIOManager> io_manager) {
  if (is_setup_) {
    return false;
  }

  if (!platform_view || !engine || !rasterizer || !io_manager) {
    return false;
  }

  platform_view_ = std::move(platform_view);
  engine_ = std::move(engine);
  rasterizer_ = std::move(rasterizer);
  io_manager_ = std::move(io_manager);

  // The weak ptr must be generated in the platform thread which owns the unique
  // ptr.
  weak_engine_ = engine_->GetWeakPtr();
  weak_rasterizer_ = rasterizer_->GetWeakPtr();
  weak_platform_view_ = platform_view_->GetWeakPtr();

  is_setup_ = true;

  vm_->GetServiceProtocol()->AddHandler(this, GetServiceProtocolDescription());

  PersistentCache::GetCacheForProcess()->AddWorkerTaskRunner(
      task_runners_.GetIOTaskRunner());

  PersistentCache::GetCacheForProcess()->SetIsDumpingSkp(
      settings_.dump_skp_on_shader_compilation);

  // TODO(gw280): The WeakPtr here asserts that we are derefing it on the
  // same thread as it was created on. Shell is constructed on the platform
  // thread but we need to call into the Engine on the UI thread, so we need
  // to use getUnsafe() here to avoid failing the assertion.
  //
  // https://github.com/flutter/flutter/issues/42947
  display_refresh_rate_ = weak_engine_.getUnsafe()->GetDisplayRefreshRate();

  return true;
}

const Settings& Shell::GetSettings() const {
  return settings_;
}

const TaskRunners& Shell::GetTaskRunners() const {
  return task_runners_;
}

fml::WeakPtr<Rasterizer> Shell::GetRasterizer() const {
  // BD MOD:
  // FML_DCHECK(is_setup_);
  FML_DCHECK(is_preload_ || is_setup_);
  return weak_rasterizer_;
}

fml::WeakPtr<Engine> Shell::GetEngine() {
  // BD MOD:
  // FML_DCHECK(is_setup_);
  FML_DCHECK(is_preload_ || is_setup_);
  return weak_engine_;
}

fml::WeakPtr<PlatformView> Shell::GetPlatformView() {
  // BD MOD:
  // FML_DCHECK(is_setup_);
  FML_DCHECK(is_preload_ || is_setup_);
  return weak_platform_view_;
}

DartVM* Shell::GetDartVM() {
  return &vm_;
}

// |PlatformView::Delegate|
void Shell::OnPlatformViewCreated(std::unique_ptr<Surface> surface) {
  TRACE_EVENT0("flutter", "Shell::OnPlatformViewCreated");
  // BD MOD:
  // FML_DCHECK(is_setup_);
  FML_DCHECK(is_preload_ || is_setup_);
  FML_DCHECK(task_runners_.GetPlatformTaskRunner()->RunsTasksOnCurrentThread());
  // BD ADD: START
  if (!is_setup_ && is_preload_) {
    is_createView_post_ = true;
    auto platform_task = fml::MakeCopyable([shell = this, surface = std::move(surface)] () mutable {
      shell->OnPlatformViewCreated(std::move(surface));
    });
    auto ui_task = [platform_task_runner = task_runners_.GetPlatformTaskRunner(), platform_task] {
      fml::TaskRunner::RunNowOrPostTask(platform_task_runner, platform_task);
    };
    fml::TaskRunner::RunNowOrPostTask(task_runners_.GetUITaskRunner(), ui_task);
    return;
  }
  // END

  // Note:
  // This is a synchronous operation because certain platforms depend on
  // setup/suspension of all activities that may be interacting with the GPU in
  // a synchronous fashion.
  fml::AutoResetWaitableEvent latch;
  auto gpu_task =
      fml::MakeCopyable([& waiting_for_first_frame = waiting_for_first_frame_,
                         rasterizer = rasterizer_->GetWeakPtr(),  //
                         surface = std::move(surface),            //
                         &latch]() mutable {
        if (rasterizer) {
          rasterizer->Setup(std::move(surface));
        }

        waiting_for_first_frame.store(true);

        // Step 3: All done. Signal the latch that the platform thread is
        // waiting on.
        latch.Signal();
      });

  // The normal flow executed by this method is that the platform thread is
  // starting the sequence and waiting on the latch. Later the UI thread posts
  // gpu_task to the GPU thread which signals the latch. If the GPU the and
  // platform threads are the same this results in a deadlock as the gpu_task
  // will never be posted to the plaform/gpu thread that is blocked on a latch.
  // To avoid the described deadlock, if the gpu and the platform threads are
  // the same, should_post_gpu_task will be false, and then instead of posting a
  // task to the gpu thread, the ui thread just signals the latch and the
  // platform/gpu thread follows with executing gpu_task.
  bool should_post_gpu_task =
      task_runners_.GetGPUTaskRunner() != task_runners_.GetPlatformTaskRunner();

  auto ui_task = [engine = engine_->GetWeakPtr(),                      //
                   gpu_task_runner = task_runners_.GetGPUTaskRunner(),  //
                   gpu_task, should_post_gpu_task,
                   &latch,
                   // BD ADD:
                   is_post = is_createView_post_
  ] {
    if (engine) {
      engine->OnOutputSurfaceCreated();
    }
    // BD MOD: START
    // // Step 2: Next, tell the GPU thread that it should create a surface for its
    // // rasterizer.
    // if (should_post_gpu_task) {
    //   fml::TaskRunner::RunNowOrPostTask(gpu_task_runner, gpu_task);
    // } else {
    //   // See comment on should_post_gpu_task, in this case we just unblock
    //   // the platform thread.
    //   latch.Signal();
    // }
    if (is_post) {
      // Step 2: Next, tell the GPU thread that it should create a surface for its
       // rasterizer.
       if (should_post_gpu_task) {
         fml::TaskRunner::RunNowOrPostTask(gpu_task_runner, gpu_task);
       } else {
         // See comment on should_post_gpu_task, in this case we just unblock
         // the platform thread.
         latch.Signal();
       }
    } else if (!should_post_gpu_task) {
      // See comment on should_post_gpu_task, in this case we just unblock
      // the platform thread.
      latch.Signal();
    }
    // END
  };

  // Threading: Capture platform view by raw pointer and not the weak pointer.
  // We are going to use the pointer on the IO thread which is not safe with a
  // weak pointer. However, we are preventing the platform view from being
  // collected by using a latch.
  auto* platform_view = platform_view_.get();

  FML_DCHECK(platform_view);

  auto io_task = [io_manager = io_manager_->GetWeakPtr(), platform_view,
                  // BD ADD:
                  gpu_task, should_post_gpu_task, gpu_task_runner = task_runners_.GetGPUTaskRunner(), is_post = is_createView_post_,
                  ui_task_runner = task_runners_.GetUITaskRunner(), ui_task] {
    if (io_manager && !io_manager->GetResourceContext()) {
      io_manager->NotifyResourceContextAvailable(
          platform_view->CreateResourceContext());
    }
    // Step 1: Next, post a task on the UI thread to tell the engine that it has
    // an output surface.
    fml::TaskRunner::RunNowOrPostTask(ui_task_runner, ui_task);

    // BD ADD: START
    // Step 2: Next, tell the GPU thread that it should create a surface for its
    // rasterizer.
    if (!is_post && should_post_gpu_task) {
      fml::TaskRunner::RunNowOrPostTask(gpu_task_runner, gpu_task);
    }
    // END
  };

  fml::TaskRunner::RunNowOrPostTask(task_runners_.GetIOTaskRunner(), io_task);

  latch.Wait();
  if (!should_post_gpu_task) {
    // See comment on should_post_gpu_task, in this case the gpu_task
    // wasn't executed, and we just run it here as the platform thread
    // is the GPU thread.
    gpu_task();
  }
}

// |PlatformView::Delegate|
void Shell::OnPlatformViewDestroyed() {
  TRACE_EVENT0("flutter", "Shell::OnPlatformViewDestroyed");
  // BD MOD:
  // FML_DCHECK(is_setup_);
  FML_DCHECK(is_preload_ || is_setup_);
  FML_DCHECK(task_runners_.GetPlatformTaskRunner()->RunsTasksOnCurrentThread());
  // BD ADD: START
  if (!is_setup_ && is_preload_) {
    auto platform_task = [shell = this] {
      shell->OnPlatformViewDestroyed();
    };
    auto ui_task = [platform_task_runner = task_runners_.GetPlatformTaskRunner(), platform_task] {
      fml::TaskRunner::RunNowOrPostTask(platform_task_runner, platform_task);
    };
    fml::TaskRunner::RunNowOrPostTask(task_runners_.GetUITaskRunner(), ui_task);
    return;
  }
  // END

  // Note:
  // This is a synchronous operation because certain platforms depend on
  // setup/suspension of all activities that may be interacting with the GPU in
  // a synchronous fashion.

  fml::AutoResetWaitableEvent latch;

  auto io_task = [io_manager = io_manager_.get(), &latch]() {
    // Execute any pending Skia object deletions while GPU access is still
    // allowed.
    io_manager->GetIsGpuDisabledSyncSwitch()->Execute(
        fml::SyncSwitch::Handlers().SetIfFalse(
            [&] { io_manager->GetSkiaUnrefQueue()->Drain(); }));
    // Step 3: All done. Signal the latch that the platform thread is waiting
    // on.
    latch.Signal();
  };

  auto gpu_task = [rasterizer = rasterizer_->GetWeakPtr(),
                   io_task_runner = task_runners_.GetIOTaskRunner(),
                   io_task]() {
    if (rasterizer) {
      rasterizer->Teardown();
    }
    // Step 2: Next, tell the IO thread to complete its remaining work.
    fml::TaskRunner::RunNowOrPostTask(io_task_runner, io_task);
  };

  // The normal flow executed by this method is that the platform thread is
  // starting the sequence and waiting on the latch. Later the UI thread posts
  // gpu_task to the GPU thread triggers signaling the latch(on the IO thread).
  // If the GPU the and platform threads are the same this results in a deadlock
  // as the gpu_task will never be posted to the plaform/gpu thread that is
  // blocked on a latch.  To avoid the described deadlock, if the gpu and the
  // platform threads are the same, should_post_gpu_task will be false, and then
  // instead of posting a task to the gpu thread, the ui thread just signals the
  // latch and the platform/gpu thread follows with executing gpu_task.
  bool should_post_gpu_task =
      task_runners_.GetGPUTaskRunner() != task_runners_.GetPlatformTaskRunner();

  auto ui_task = [engine = engine_->GetWeakPtr(),
                  gpu_task_runner = task_runners_.GetGPUTaskRunner(), gpu_task,
                  should_post_gpu_task, &latch]() {
    if (engine) {
      engine->OnOutputSurfaceDestroyed();
    }
    // Step 1: Next, tell the GPU thread that its rasterizer should suspend
    // access to the underlying surface.
    if (should_post_gpu_task) {
      fml::TaskRunner::RunNowOrPostTask(gpu_task_runner, gpu_task);
    } else {
      // See comment on should_post_gpu_task, in this case we just unblock
      // the platform thread.
      latch.Signal();
    }
  };

  // Step 0: Post a task onto the UI thread to tell the engine that its output
  // surface is about to go away.
  fml::TaskRunner::RunNowOrPostTask(task_runners_.GetUITaskRunner(), ui_task);
  latch.Wait();
  if (!should_post_gpu_task) {
    // See comment on should_post_gpu_task, in this case the gpu_task
    // wasn't executed, and we just run it here as the platform thread
    // is the GPU thread.
    gpu_task();
    latch.Wait();
  }
}

// |PlatformView::Delegate|
void Shell::OnPlatformViewSetViewportMetrics(const ViewportMetrics& metrics) {
  // BD MOD:
  // FML_DCHECK(is_setup_);
  FML_DCHECK(is_preload_ || is_setup_);
  FML_DCHECK(task_runners_.GetPlatformTaskRunner()->RunsTasksOnCurrentThread());

  // This is the formula Android uses.
  // https://android.googlesource.com/platform/frameworks/base/+/master/libs/hwui/renderthread/CacheManager.cpp#41
  size_t max_bytes = metrics.physical_width * metrics.physical_height * 12 * 4;
  task_runners_.GetGPUTaskRunner()->PostTask(
      [rasterizer = rasterizer_->GetWeakPtr(), max_bytes] {
        if (rasterizer) {
          rasterizer->SetResourceCacheMaxBytes(max_bytes, false);
        }
      });

  // BD MOD: START
  //  task_runners_.GetUITaskRunner()->PostTask(
  //    [engine = engine_->GetWeakPtr(), metrics]() {
  //      if (engine) {
  //        engine->SetViewportMetrics(metrics);
  //      }
  //    });
  if (!is_setup_ && is_preload_) {
    if (engine_ != nullptr && engine_->GetWeakPtr()->IsViewportMetricsValid()) {
      return;
    }
    task_runners_.GetUITaskRunner()->PostTask(
      [shell = this, metrics]() {
        if (shell && shell->weak_engine_) {
          if (shell->weak_engine_->IsViewportMetricsValid() && metrics.physical_width == 0 ) {
            return;
          }
          shell->weak_engine_->SetViewportMetrics(metrics);
        }
      });
  } else {
    task_runners_.GetUITaskRunner()->PostTask(
      [engine = engine_->GetWeakPtr(), metrics]() {
        if (engine) {
          engine->SetViewportMetrics(metrics);
        }
      });
  }
  // END
}

// |PlatformView::Delegate|
void Shell::OnPlatformViewDispatchPlatformMessage(
    fml::RefPtr<PlatformMessage> message) {
  // BD MOD:
  // FML_DCHECK(is_setup_);
  FML_DCHECK(is_preload_ || is_setup_);
  FML_DCHECK(task_runners_.GetPlatformTaskRunner()->RunsTasksOnCurrentThread());

  // BD MOD: START
  //  task_runners_.GetUITaskRunner()->PostTask(
  //    [engine = engine_->GetWeakPtr(), message = std::move(message)] {
  //      if (engine) {
  //        engine->DispatchPlatformMessage(std::move(message));
  //      }
  //    });
  if (!is_setup_ && is_preload_) {
    task_runners_.GetUITaskRunner()->PostTask(
      [shell = this, message = std::move(message)] {
        if (shell && shell->weak_engine_) {
          shell->weak_engine_->DispatchPlatformMessage(std::move(message));
        }
      });
  } else {
    task_runners_.GetUITaskRunner()->PostTask(
      [engine = engine_->GetWeakPtr(), message = std::move(message)] {
        if (engine) {
          engine->DispatchPlatformMessage(std::move(message));
        }
      });
  }
  // END
}

// |PlatformView::Delegate|
void Shell::OnPlatformViewDispatchPointerDataPacket(
    std::unique_ptr<PointerDataPacket> packet) {
  TRACE_EVENT0("flutter", "Shell::OnPlatformViewDispatchPointerDataPacket");
  TRACE_FLOW_BEGIN("flutter", "PointerEvent", next_pointer_flow_id_);
  // BD MOD:
  // FML_DCHECK(is_setup_);
  FML_DCHECK(is_preload_ || is_setup_);
  FML_DCHECK(task_runners_.GetPlatformTaskRunner()->RunsTasksOnCurrentThread());
  // BD MOD: START
  //  task_runners_.GetUITaskRunner()->PostTask(
  //    fml::MakeCopyable([engine = weak_engine_, packet = std::move(packet),
  //                        flow_id = next_pointer_flow_id_]() mutable {
  //      if (engine) {
  //        engine->DispatchPointerDataPacket(std::move(packet), flow_id);
  //      }
  //    }));
  //  }
  if (!is_setup_ && is_preload_) {
    task_runners_.GetUITaskRunner()->PostTask(
      fml::MakeCopyable([shell = this, packet = std::move(packet),
                          flow_id = next_pointer_flow_id_]() mutable {
        if (shell && shell->weak_engine_) {
          shell->weak_engine_->DispatchPointerDataPacket(std::move(packet), flow_id);
        }
      }));
  } else {
    task_runners_.GetUITaskRunner()->PostTask(
      fml::MakeCopyable([engine = weak_engine_, packet = std::move(packet),
                         flow_id = next_pointer_flow_id_]() mutable {
        if (engine) {
          engine->DispatchPointerDataPacket(std::move(packet), flow_id);
        }
      }));
  }
  // END
  next_pointer_flow_id_++;
}

// |PlatformView::Delegate|
void Shell::OnPlatformViewDispatchSemanticsAction(int32_t id,
                                                  SemanticsAction action,
                                                  std::vector<uint8_t> args) {
  // BD MOD:
  // FML_DCHECK(is_setup_);
  FML_DCHECK(is_preload_ || is_setup_);
  FML_DCHECK(task_runners_.GetPlatformTaskRunner()->RunsTasksOnCurrentThread());

// BD MOD: START
//  task_runners_.GetUITaskRunner()->PostTask(
//      [engine = engine_->GetWeakPtr(), id, action, args = std::move(args)] {
//        if (engine) {
//          engine->DispatchSemanticsAction(id, action, std::move(args));
//        }
//      });
  if (!is_setup_ && is_preload_) {
    task_runners_.GetUITaskRunner()->PostTask(
      [shell = this, id, action, args = std::move(args)] {
        if (shell && shell->weak_engine_) {
          shell->weak_engine_->DispatchSemanticsAction(id, action, std::move(args));
        }
      });
  } else {
    task_runners_.GetUITaskRunner()->PostTask(
      [engine = engine_->GetWeakPtr(), id, action, args = std::move(args)] {
        if (engine) {
          engine->DispatchSemanticsAction(id, action, std::move(args));
        }
      });
  }
// END
}

// |PlatformView::Delegate|
void Shell::OnPlatformViewSetSemanticsEnabled(bool enabled) {
  // BD MOD:
  // FML_DCHECK(is_setup_);
  FML_DCHECK(is_preload_ || is_setup_);
  FML_DCHECK(task_runners_.GetPlatformTaskRunner()->RunsTasksOnCurrentThread());

// BD MOD: START
//  task_runners_.GetUITaskRunner()->PostTask(
//      [engine = engine_->GetWeakPtr(), enabled] {
//        if (engine) {
//          engine->SetSemanticsEnabled(enabled);
//        }
//      });

  if (!is_setup_ && is_preload_) {
    task_runners_.GetUITaskRunner()->PostTask(
      [shell = this, enabled] {
        if (shell && shell->weak_engine_) {
          shell->weak_engine_->SetSemanticsEnabled(enabled);
        }
      });
  } else {
    task_runners_.GetUITaskRunner()->PostTask(
      [engine = engine_->GetWeakPtr(), enabled] {
        if (engine) {
          engine->SetSemanticsEnabled(enabled);
        }
      });
  }
// END
}

// |PlatformView::Delegate|
void Shell::OnPlatformViewSetAccessibilityFeatures(int32_t flags) {
  // BD MOD:
  // FML_DCHECK(is_setup_);
  FML_DCHECK(is_preload_ || is_setup_);
  FML_DCHECK(task_runners_.GetPlatformTaskRunner()->RunsTasksOnCurrentThread());
// BD MOD: START
//  task_runners_.GetUITaskRunner()->PostTask(
//      [engine = engine_->GetWeakPtr(), flags] {
//        if (engine) {
//          engine->SetAccessibilityFeatures(flags);
//        }
//      });
  if (!is_setup_ && is_preload_) {
    task_runners_.GetUITaskRunner()->PostTask(
      [shell = this, flags] {
        if (shell && shell->weak_engine_) {
          shell->weak_engine_->SetAccessibilityFeatures(flags);
        }
      });
  } else {
    task_runners_.GetUITaskRunner()->PostTask(
      [engine = engine_->GetWeakPtr(), flags] {
        if (engine) {
          engine->SetAccessibilityFeatures(flags);
        }
      });
  }
// END
}

// |PlatformView::Delegate|
void Shell::OnPlatformViewRegisterTexture(
    std::shared_ptr<flutter::Texture> texture) {
  // BD MOD:
  // FML_DCHECK(is_setup_);
  FML_DCHECK(is_preload_ || is_setup_);
  FML_DCHECK(task_runners_.GetPlatformTaskRunner()->RunsTasksOnCurrentThread());

  task_runners_.GetGPUTaskRunner()->PostTask(
      [rasterizer = rasterizer_->GetWeakPtr(), texture] {
        if (rasterizer) {
          if (auto* registry = rasterizer->GetTextureRegistry()) {
            registry->RegisterTexture(texture);
          }
        }
      });
}

// |PlatformView::Delegate|
void Shell::OnPlatformViewUnregisterTexture(int64_t texture_id) {
  // BD MOD:
  // FML_DCHECK(is_setup_);
  FML_DCHECK(is_preload_ || is_setup_);
  FML_DCHECK(task_runners_.GetPlatformTaskRunner()->RunsTasksOnCurrentThread());

  task_runners_.GetGPUTaskRunner()->PostTask(
      [rasterizer = rasterizer_->GetWeakPtr(), texture_id]() {
        if (rasterizer) {
          if (auto* registry = rasterizer->GetTextureRegistry()) {
            registry->UnregisterTexture(texture_id);
          }
        }
      });
}

// |PlatformView::Delegate|
void Shell::OnPlatformViewMarkTextureFrameAvailable(int64_t texture_id) {
  // BD MOD:
  // FML_DCHECK(is_setup_);
  FML_DCHECK(is_preload_ || is_setup_);
  FML_DCHECK(task_runners_.GetPlatformTaskRunner()->RunsTasksOnCurrentThread());

  // Tell the rasterizer that one of its textures has a new frame available.
  task_runners_.GetGPUTaskRunner()->PostTask(
      [rasterizer = rasterizer_->GetWeakPtr(), texture_id]() {
        auto* registry = rasterizer->GetTextureRegistry();

        if (!registry) {
          return;
        }

        auto texture = registry->GetTexture(texture_id);

        if (!texture) {
          return;
        }

        texture->MarkNewFrameAvailable();
      });

  // Schedule a new frame without having to rebuild the layer tree.
  // BD MOD: START
  // task_runners_.GetUITaskRunner()->PostTask([engine = engine_->GetWeakPtr()]() {
  //   if (engine) {
  //     engine->ScheduleFrame(false);
  //   }
  // });
  if (!is_setup_ && is_preload_) {
    task_runners_.GetUITaskRunner()->PostTask([shell = this]() {
      if (shell && shell->weak_engine_) {
        shell->weak_engine_->ScheduleFrame(false);
      }
    });
  } else {
    task_runners_.GetUITaskRunner()->PostTask([engine = engine_->GetWeakPtr()]() {
      if (engine) {
        engine->ScheduleFrame(false);
      }
    });
  }
}

/**
 * BD ADD:
 */
// |PlatformView::Delegate|
void Shell::OnPlatformViewRegisterImageLoader(
    std::shared_ptr<flutter::ImageLoader> imageLoader) {
  // BD MOD:
  // FML_DCHECK(is_setup_);
  FML_DCHECK(is_preload_ || is_setup_);
  FML_DCHECK(task_runners_.GetPlatformTaskRunner()->RunsTasksOnCurrentThread());

  task_runners_.GetIOTaskRunner()->PostTask(
      [io_manager = io_manager_->GetWeakPtr(),
       imageLoader = std::move(imageLoader)] {
        if (io_manager) {
          io_manager->RegisterImageLoader(imageLoader);
        }
      });
}

// |PlatformView::Delegate|
void Shell::OnPlatformViewSetNextFrameCallback(const fml::closure& closure) {
  // BD MOD:
  // FML_DCHECK(is_setup_);
  FML_DCHECK(is_preload_ || is_setup_);
  FML_DCHECK(task_runners_.GetPlatformTaskRunner()->RunsTasksOnCurrentThread());

  task_runners_.GetGPUTaskRunner()->PostTask(
      [rasterizer = rasterizer_->GetWeakPtr(), closure = closure]() {
        if (rasterizer) {
          rasterizer->SetNextFrameCallback(std::move(closure));
        }
      });
}

// |Animator::Delegate|
void Shell::OnAnimatorBeginFrame(fml::TimePoint frame_time) {
  // BD MOD:
  // FML_DCHECK(is_setup_);
  FML_DCHECK(is_preload_ || is_setup_);
  FML_DCHECK(task_runners_.GetUITaskRunner()->RunsTasksOnCurrentThread());

  if (engine_) {
    engine_->BeginFrame(frame_time);
  }
}

// |Animator::Delegate|
// BD: MOD
// void Shell::OnAnimatorNotifyIdle(int64_t deadline) {
void Shell::OnAnimatorNotifyIdle(int64_t deadline, int type) {
  // BD MOD:
  // FML_DCHECK(is_setup_);
  FML_DCHECK(is_preload_ || is_setup_);
  FML_DCHECK(task_runners_.GetUITaskRunner()->RunsTasksOnCurrentThread());

  if (engine_) {
    // BD: MOD
    // engine_->NotifyIdle(deadline);
    engine_->NotifyIdle(deadline, type);
    // BD ADD: START
    // update IO Cache 10 sec
    if (lastIOMemUpdate_ == 0 || deadline - lastIOMemUpdate_ >= 9900000) {
      lastIOMemUpdate_ = deadline;
      if (task_runners_.IsValid()) {
        task_runners_.GetIOTaskRunner()->PostDelayedTask(
          [io_manager = io_manager_->GetWeakPtr()] {
            if (io_manager) {
              io_manager->GetResourceContext();
            }
          }, TimeDelta::FromSecondsF(10));
      }
    }
    // END
  }
}

// |Animator::Delegate|
void Shell::OnAnimatorDraw(fml::RefPtr<Pipeline<flutter::LayerTree>> pipeline) {
  // BD MOD:
  // FML_DCHECK(is_setup_);
  FML_DCHECK(is_preload_ || is_setup_);

  task_runners_.GetGPUTaskRunner()->PostTask(
      [& waiting_for_first_frame = waiting_for_first_frame_,
       &waiting_for_first_frame_condition = waiting_for_first_frame_condition_,
       rasterizer = rasterizer_->GetWeakPtr(),
       pipeline = std::move(pipeline)]() {
        if (rasterizer) {
          rasterizer->Draw(pipeline);

          if (waiting_for_first_frame.load()) {
            waiting_for_first_frame.store(false);
            waiting_for_first_frame_condition.notify_all();
          }
        }
      });
}

// |Animator::Delegate|
void Shell::OnAnimatorDrawLastLayerTree() {
  // BD MOD:
  // FML_DCHECK(is_setup_);
  FML_DCHECK(is_preload_ || is_setup_);

  task_runners_.GetGPUTaskRunner()->PostTask(
      [rasterizer = rasterizer_->GetWeakPtr()]() {
        if (rasterizer) {
          rasterizer->DrawLastLayerTree();
        }
      });
}

// |Engine::Delegate|
void Shell::OnEngineUpdateSemantics(SemanticsNodeUpdates update,
                                    CustomAccessibilityActionUpdates actions) {
  // BD MOD:
  // FML_DCHECK(is_setup_);
  FML_DCHECK(is_preload_ || is_setup_);
  FML_DCHECK(task_runners_.GetUITaskRunner()->RunsTasksOnCurrentThread());

  task_runners_.GetPlatformTaskRunner()->PostTask(
      [view = platform_view_->GetWeakPtr(), update = std::move(update),
       actions = std::move(actions)] {
        if (view) {
          view->UpdateSemantics(std::move(update), std::move(actions));
        }
      });
}

// |Engine::Delegate|
void Shell::OnEngineHandlePlatformMessage(
    fml::RefPtr<PlatformMessage> message) {
  // BD MOD:
  // FML_DCHECK(is_setup_);
  FML_DCHECK(is_preload_ || is_setup_);
  FML_DCHECK(task_runners_.GetUITaskRunner()->RunsTasksOnCurrentThread());

  if (message->channel() == kSkiaChannel) {
    HandleEngineSkiaMessage(std::move(message));
    return;
  }

  task_runners_.GetPlatformTaskRunner()->PostTask(
      [view = platform_view_->GetWeakPtr(), message = std::move(message)]() {
        if (view) {
          view->HandlePlatformMessage(std::move(message));
        }
        // BD MOD:
        // });
      },
      Boost::Current()->IsDelayPlatformMessage());
}

void Shell::HandleEngineSkiaMessage(fml::RefPtr<PlatformMessage> message) {
  const auto& data = message->data();

  rapidjson::Document document;
  document.Parse(reinterpret_cast<const char*>(data.data()), data.size());
  if (document.HasParseError() || !document.IsObject())
    return;
  auto root = document.GetObject();
  auto method = root.FindMember("method");
  if (method->value != "Skia.setResourceCacheMaxBytes")
    return;
  auto args = root.FindMember("args");
  if (args == root.MemberEnd() || !args->value.IsInt())
    return;

  task_runners_.GetGPUTaskRunner()->PostTask(
      [rasterizer = rasterizer_->GetWeakPtr(), max_bytes = args->value.GetInt(),
       response = std::move(message->response())] {
        if (rasterizer) {
          rasterizer->SetResourceCacheMaxBytes(static_cast<size_t>(max_bytes),
                                               true);
        }
        if (response) {
          // The framework side expects this to be valid json encoded as a list.
          // Return `[true]` to signal success.
          std::vector<uint8_t> data = {'[', 't', 'r', 'u', 'e', ']'};
          response->Complete(
              std::make_unique<fml::DataMapping>(std::move(data)));
        }
      });
}

// |Engine::Delegate|
void Shell::OnPreEngineRestart() {
  // BD MOD:
  // FML_DCHECK(is_setup_);
  FML_DCHECK(is_preload_ || is_setup_);
  FML_DCHECK(task_runners_.GetUITaskRunner()->RunsTasksOnCurrentThread());

  fml::AutoResetWaitableEvent latch;
  fml::TaskRunner::RunNowOrPostTask(
      task_runners_.GetPlatformTaskRunner(),
      [view = platform_view_->GetWeakPtr(), &latch]() {
        if (view) {
          view->OnPreEngineRestart();
        }
        latch.Signal();
      });
  // This is blocking as any embedded platform views has to be flushed before
  // we re-run the Dart code.
  latch.Wait();
}

// |Engine::Delegate|
void Shell::UpdateIsolateDescription(const std::string isolate_name,
                                     int64_t isolate_port) {
  Handler::Description description(isolate_port, isolate_name);
  vm_->GetServiceProtocol()->SetHandlerDescription(this, description);
}

void Shell::SetNeedsReportTimings(bool value) {
  needs_report_timings_ = value;
}

void Shell::ReportTimings() {
  // BD MOD:
  // FML_DCHECK(is_setup_);
  FML_DCHECK(is_preload_ || is_setup_);
  FML_DCHECK(task_runners_.GetGPUTaskRunner()->RunsTasksOnCurrentThread());

  auto timings = std::move(unreported_timings_);
  unreported_timings_ = {};
  task_runners_.GetUITaskRunner()->PostTask([timings, engine = weak_engine_] {
    if (engine) {
      engine->ReportTimings(std::move(timings));
    }
  });
}

size_t Shell::UnreportedFramesCount() const {
  // Check that this is running on the GPU thread to avoid race conditions.
  FML_DCHECK(task_runners_.GetGPUTaskRunner()->RunsTasksOnCurrentThread());
  FML_DCHECK(unreported_timings_.size() % FrameTiming::kCount == 0);
  return unreported_timings_.size() / FrameTiming::kCount;
}

void Shell::OnFrameRasterized(const FrameTiming& timing) {
  // BD MOD:
  // FML_DCHECK(is_setup_);
  FML_DCHECK(is_preload_ || is_setup_);
  FML_DCHECK(task_runners_.GetGPUTaskRunner()->RunsTasksOnCurrentThread());

  // The C++ callback defined in settings.h and set by Flutter runner. This is
  // independent of the timings report to the Dart side.
  if (settings_.frame_rasterized_callback) {
    settings_.frame_rasterized_callback(timing);
  }

  if (!needs_report_timings_) {
    return;
  }

  for (auto phase : FrameTiming::kPhases) {
    unreported_timings_.push_back(
        timing.Get(phase).ToEpochDelta().ToMicroseconds());
  }

  // In tests using iPhone 6S with profile mode, sending a batch of 1 frame or a
  // batch of 100 frames have roughly the same cost of less than 0.1ms. Sending
  // a batch of 500 frames costs about 0.2ms. The 1 second threshold usually
  // kicks in before we reaching the following 100 frames threshold. The 100
  // threshold here is mainly for unit tests (so we don't have to write a
  // 1-second unit test), and make sure that our vector won't grow too big with
  // future 120fps, 240fps, or 1000fps displays.
  //
  // In the profile/debug mode, the timings are used by development tools which
  // require a latency of no more than 100ms. Hence we lower that 1-second
  // threshold to 100ms because performance overhead isn't that critical in
  // those cases.
  if (!first_frame_rasterized_ || UnreportedFramesCount() >= 100) {
    first_frame_rasterized_ = true;
    ReportTimings();
  } else if (!frame_timings_report_scheduled_) {
    // BD MOD:
//#if FLUTTER_RELEASE
//    constexpr int kBatchTimeInMilliseconds = 1000;
//#else
    constexpr int kBatchTimeInMilliseconds = 100;
//#endif
    // END

    // Also make sure that frame times get reported with a max latency of 1
    // second. Otherwise, the timings of last few frames of an animation may
    // never be reported until the next animation starts.
    frame_timings_report_scheduled_ = true;
    task_runners_.GetGPUTaskRunner()->PostDelayedTask(
        [self = weak_factory_gpu_->GetWeakPtr()]() {
          if (!self.get()) {
            return;
          }
          self->frame_timings_report_scheduled_ = false;
          if (self->UnreportedFramesCount() > 0) {
            self->ReportTimings();
          }
        },
        fml::TimeDelta::FromMilliseconds(kBatchTimeInMilliseconds));
  }
}

fml::Milliseconds Shell::GetFrameBudget() {
  if (display_refresh_rate_ > 0) {
    return fml::RefreshRateToFrameBudget(display_refresh_rate_.load());
  } else {
    return fml::kDefaultFrameBudget;
  }
}

// BD ADD: XieRan
// |Engine::Delegate|
void Shell::AddNextFrameCallback(fml::closure callback) {
  task_runners_.GetGPUTaskRunner()->PostTask(
      [rasterizer = rasterizer_->GetWeakPtr(),
       callback = std::move(callback)]() {
        if (rasterizer) {
          rasterizer->AddNextFrameCallback(
              [callback = std::move(callback)]() { callback(); });
        };
      });
}
// END

// BD ADD: YuanHuihui
std::vector<double> Shell::GetFps(int thread_type,
                                  int fps_type,
                                  bool do_clear) {
  std::vector<double> result;
  fml::WeakPtr<Rasterizer> rasterizer = rasterizer_->GetWeakPtr();
  if (rasterizer) {
    if (thread_type == kUiThreadType) {
      result = rasterizer->compositor_context()->ui_time().GetFps(fps_type);
      if (do_clear) {
        task_runners_.GetUITaskRunner()->PostTask(
            [rasterizer = rasterizer_->GetWeakPtr()] {
              if (rasterizer) {
                rasterizer->compositor_context()->ui_time().ClearFps();
              }
            });
      }
    } else if (thread_type == kGpuThreadType) {
      result = rasterizer->compositor_context()->raster_time().GetFps(fps_type);
      if (do_clear) {
        task_runners_.GetGPUTaskRunner()->PostTask(
            [rasterizer = rasterizer_->GetWeakPtr()] {
              if (rasterizer) {
                rasterizer->compositor_context()->raster_time().ClearFps();
              }
            });
      }
    }
  }
  return result;
}
// END

// |ServiceProtocol::Handler|
fml::RefPtr<fml::TaskRunner> Shell::GetServiceProtocolHandlerTaskRunner(
    std::string_view method) const {
  // BD MOD:
  // FML_DCHECK(is_setup_);
  FML_DCHECK(is_preload_ || is_setup_);
  auto found = service_protocol_handlers_.find(method);
  if (found != service_protocol_handlers_.end()) {
    return found->second.first;
  }
  return task_runners_.GetUITaskRunner();
}

// |ServiceProtocol::Handler|
bool Shell::HandleServiceProtocolMessage(
    std::string_view method,  // one if the extension names specified above.
    const ServiceProtocolMap& params,
    rapidjson::Document& response) {
  auto found = service_protocol_handlers_.find(method);
  if (found != service_protocol_handlers_.end()) {
    return found->second.second(params, response);
  }
  return false;
}

// |ServiceProtocol::Handler|
ServiceProtocol::Handler::Description Shell::GetServiceProtocolDescription()
    const {
  return {
      engine_->GetUIIsolateMainPort(),
      engine_->GetUIIsolateName(),
  };
}

static void ServiceProtocolParameterError(rapidjson::Document& response,
                                          std::string error_details) {
  auto& allocator = response.GetAllocator();
  response.SetObject();
  const int64_t kInvalidParams = -32602;
  response.AddMember("code", kInvalidParams, allocator);
  response.AddMember("message", "Invalid params", allocator);
  {
    rapidjson::Value details(rapidjson::kObjectType);
    details.AddMember("details", error_details, allocator);
    response.AddMember("data", details, allocator);
  }
}

static void ServiceProtocolFailureError(rapidjson::Document& response,
                                        std::string message) {
  auto& allocator = response.GetAllocator();
  response.SetObject();
  const int64_t kJsonServerError = -32000;
  response.AddMember("code", kJsonServerError, allocator);
  response.AddMember("message", message, allocator);
}

// Service protocol handler
bool Shell::OnServiceProtocolScreenshot(
    const ServiceProtocol::Handler::ServiceProtocolMap& params,
    rapidjson::Document& response) {
  FML_DCHECK(task_runners_.GetGPUTaskRunner()->RunsTasksOnCurrentThread());
  auto screenshot = rasterizer_->ScreenshotLastLayerTree(
      Rasterizer::ScreenshotType::CompressedImage, true);
  if (screenshot.data) {
    response.SetObject();
    auto& allocator = response.GetAllocator();
    response.AddMember("type", "Screenshot", allocator);
    rapidjson::Value image;
    image.SetString(static_cast<const char*>(screenshot.data->data()),
                    screenshot.data->size(), allocator);
    response.AddMember("screenshot", image, allocator);
    return true;
  }
  ServiceProtocolFailureError(response, "Could not capture image screenshot.");
  return false;
}

// Service protocol handler
bool Shell::OnServiceProtocolScreenshotSKP(
    const ServiceProtocol::Handler::ServiceProtocolMap& params,
    rapidjson::Document& response) {
  FML_DCHECK(task_runners_.GetGPUTaskRunner()->RunsTasksOnCurrentThread());
  auto screenshot = rasterizer_->ScreenshotLastLayerTree(
      Rasterizer::ScreenshotType::SkiaPicture, true);
  if (screenshot.data) {
    response.SetObject();
    auto& allocator = response.GetAllocator();
    response.AddMember("type", "ScreenshotSkp", allocator);
    rapidjson::Value skp;
    skp.SetString(static_cast<const char*>(screenshot.data->data()),
                  screenshot.data->size(), allocator);
    response.AddMember("skp", skp, allocator);
    return true;
  }
  ServiceProtocolFailureError(response, "Could not capture SKP screenshot.");
  return false;
}

// Service protocol handler
bool Shell::OnServiceProtocolRunInView(
    const ServiceProtocol::Handler::ServiceProtocolMap& params,
    rapidjson::Document& response) {
  FML_DCHECK(task_runners_.GetUITaskRunner()->RunsTasksOnCurrentThread());

  if (params.count("mainScript") == 0) {
    ServiceProtocolParameterError(response,
                                  "'mainScript' parameter is missing.");
    return false;
  }

  // TODO(chinmaygarde): In case of hot-reload from .dill files, the packages
  // file is ignored. Currently, the tool is passing a junk packages file to
  // pass this check. Update the service protocol interface and remove this
  // workaround.
  if (params.count("packagesFile") == 0) {
    ServiceProtocolParameterError(response,
                                  "'packagesFile' parameter is missing.");
    return false;
  }

  if (params.count("assetDirectory") == 0) {
    ServiceProtocolParameterError(response,
                                  "'assetDirectory' parameter is missing.");
    return false;
  }

  std::string main_script_path =
      fml::paths::FromURI(params.at("mainScript").data());
  std::string packages_path =
      fml::paths::FromURI(params.at("packagesFile").data());
  std::string asset_directory_path =
      fml::paths::FromURI(params.at("assetDirectory").data());

  auto main_script_file_mapping =
      std::make_unique<fml::FileMapping>(fml::OpenFile(
          main_script_path.c_str(), false, fml::FilePermission::kRead));

  auto isolate_configuration = IsolateConfiguration::CreateForKernel(
      std::move(main_script_file_mapping));

  RunConfiguration configuration(std::move(isolate_configuration));

  configuration.SetEntrypointAndLibrary(engine_->GetLastEntrypoint(),
                                        engine_->GetLastEntrypointLibrary());

  configuration.AddAssetResolver(
      std::make_unique<DirectoryAssetBundle>(fml::OpenDirectory(
          asset_directory_path.c_str(), false, fml::FilePermission::kRead)));

  auto& allocator = response.GetAllocator();
  response.SetObject();
  if (engine_->Restart(std::move(configuration))) {
    response.AddMember("type", "Success", allocator);
    auto new_description = GetServiceProtocolDescription();
    rapidjson::Value view(rapidjson::kObjectType);
    new_description.Write(this, view, allocator);
    response.AddMember("view", view, allocator);
    return true;
  } else {
    FML_DLOG(ERROR) << "Could not run configuration in engine.";
    ServiceProtocolFailureError(response,
                                "Could not run configuration in engine.");
    return false;
  }

  FML_DCHECK(false);
  return false;
}

// Service protocol handler
bool Shell::OnServiceProtocolFlushUIThreadTasks(
    const ServiceProtocol::Handler::ServiceProtocolMap& params,
    rapidjson::Document& response) {
  FML_DCHECK(task_runners_.GetUITaskRunner()->RunsTasksOnCurrentThread());
  // This API should not be invoked by production code.
  // It can potentially starve the service isolate if the main isolate pauses
  // at a breakpoint or is in an infinite loop.
  //
  // It should be invoked from the VM Service and and blocks it until UI thread
  // tasks are processed.
  response.SetObject();
  response.AddMember("type", "Success", response.GetAllocator());
  return true;
}

bool Shell::OnServiceProtocolGetDisplayRefreshRate(
    const ServiceProtocol::Handler::ServiceProtocolMap& params,
    rapidjson::Document& response) {
  FML_DCHECK(task_runners_.GetUITaskRunner()->RunsTasksOnCurrentThread());
  response.SetObject();
  response.AddMember("type", "DisplayRefreshRate", response.GetAllocator());
  response.AddMember("fps", engine_->GetDisplayRefreshRate(),
                     response.GetAllocator());
  return true;
}

// Service protocol handler
bool Shell::OnServiceProtocolSetAssetBundlePath(
    const ServiceProtocol::Handler::ServiceProtocolMap& params,
    rapidjson::Document& response) {
  FML_DCHECK(task_runners_.GetUITaskRunner()->RunsTasksOnCurrentThread());

  if (params.count("assetDirectory") == 0) {
    ServiceProtocolParameterError(response,
                                  "'assetDirectory' parameter is missing.");
    return false;
  }

  auto& allocator = response.GetAllocator();
  response.SetObject();

  auto asset_manager = std::make_shared<AssetManager>();

  asset_manager->PushFront(std::make_unique<DirectoryAssetBundle>(
      fml::OpenDirectory(params.at("assetDirectory").data(), false,
                         fml::FilePermission::kRead)));

  if (engine_->UpdateAssetManager(std::move(asset_manager))) {
    response.AddMember("type", "Success", allocator);
    auto new_description = GetServiceProtocolDescription();
    rapidjson::Value view(rapidjson::kObjectType);
    new_description.Write(this, view, allocator);
    response.AddMember("view", view, allocator);
    return true;
  } else {
    FML_DLOG(ERROR) << "Could not update asset directory.";
    ServiceProtocolFailureError(response, "Could not update asset directory.");
    return false;
  }

  FML_DCHECK(false);
  return false;
}

Rasterizer::Screenshot Shell::Screenshot(
    Rasterizer::ScreenshotType screenshot_type,
    bool base64_encode) {
  TRACE_EVENT0("flutter", "Shell::Screenshot");
  fml::AutoResetWaitableEvent latch;
  Rasterizer::Screenshot screenshot;
  fml::TaskRunner::RunNowOrPostTask(
      task_runners_.GetGPUTaskRunner(), [&latch,                        //
                                         rasterizer = GetRasterizer(),  //
                                         &screenshot,                   //
                                         screenshot_type,               //
                                         base64_encode                  //
  ]() {
        if (rasterizer) {
          screenshot = rasterizer->ScreenshotLastLayerTree(screenshot_type,
                                                           base64_encode);
        }
        latch.Signal();
      });
  latch.Wait();
  return screenshot;
}

fml::Status Shell::WaitForFirstFrame(fml::TimeDelta timeout) {
  FML_DCHECK(is_setup_);
  if (task_runners_.GetUITaskRunner()->RunsTasksOnCurrentThread() ||
      task_runners_.GetGPUTaskRunner()->RunsTasksOnCurrentThread()) {
    return fml::Status(fml::StatusCode::kFailedPrecondition,
                       "WaitForFirstFrame called from thread that can't wait "
                       "because it is responsible for generating the frame.");
  }

  std::unique_lock<std::mutex> lock(waiting_for_first_frame_mutex_);
  bool success = waiting_for_first_frame_condition_.wait_for(
      lock, std::chrono::milliseconds(timeout.ToMilliseconds()),
      [& waiting_for_first_frame = waiting_for_first_frame_] {
        return !waiting_for_first_frame.load();
      });
  if (success) {
    return fml::Status();
  } else {
    return fml::Status(fml::StatusCode::kDeadlineExceeded, "timeout");
  }
}

bool Shell::ReloadSystemFonts() {
  FML_DCHECK(is_setup_);
  FML_DCHECK(task_runners_.GetPlatformTaskRunner()->RunsTasksOnCurrentThread());

  if (!engine_) {
    return false;
  }
  engine_->GetFontCollection().GetFontCollection()->SetupDefaultFontManager();
  engine_->GetFontCollection().GetFontCollection()->ClearFontFamilyCache();
  // After system fonts are reloaded, we send a system channel message
  // to notify flutter framework.
  rapidjson::Document document;
  document.SetObject();
  auto& allocator = document.GetAllocator();
  rapidjson::Value message_value;
  message_value.SetString(kFontChange, allocator);
  document.AddMember(kTypeKey, message_value, allocator);

  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  document.Accept(writer);
  std::string message = buffer.GetString();
  fml::RefPtr<PlatformMessage> fontsChangeMessage =
      fml::MakeRefCounted<flutter::PlatformMessage>(
          kSystemChannel, std::vector<uint8_t>(message.begin(), message.end()),
          nullptr);

  OnPlatformViewDispatchPlatformMessage(fontsChangeMessage);
  return true;
}

std::shared_ptr<fml::SyncSwitch> Shell::GetIsGpuDisabledSyncSwitch() const {
  return is_gpu_disabled_sync_switch_;
}

// BD ADD: QiuXinyue
void Shell::ExitApp(fml::closure closure) {
  Performance::GetInstance()->SetExitStatus(true);
  // 步骤1：通知Flutter退出App
  fml::TaskRunner::RunNowOrPostTask(
      task_runners_.GetUITaskRunner(),
      fml::MakeCopyable(
          [this, ui_task_runner = task_runners_.GetUITaskRunner(),
           platform_task_runner = task_runners_.GetPlatformTaskRunner(),
           closure = std::move(closure)]() {
            auto engine = engine_->GetWeakPtr();
            if (engine) {
              engine->ExitApp();
            }
            // 步骤2：完成UI线程剩余任务
            ui_task_runner->PostTask(fml::MakeCopyable(
                [platform_task_runner, closure = std::move(closure)] {
                  // 步骤3：完成Platform线程剩余任务
                  platform_task_runner->PostTask(fml::MakeCopyable(
                      [closure = std::move(closure)] { closure(); }));
                }));
          }));
}

void Shell::ScheduleBackgroundFrame() {
  // BD MOD:
  // FML_DCHECK(is_setup_);
  FML_DCHECK(is_preload_ || is_setup_);
  FML_DCHECK(task_runners_.GetPlatformTaskRunner()->RunsTasksOnCurrentThread());

  task_runners_.GetUITaskRunner()->PostTask([this] {
    auto engine = GetEngine();
    if (engine) {
      engine->ScheduleBackgroundFrame();
    }
  });
}

bool Shell::SetupWithoutEngine(std::unique_ptr<PlatformView> platform_view,
                               std::unique_ptr<Rasterizer> rasterizer,
                               std::unique_ptr<ShellIOManager> io_manager) {
  if (is_setup_) {
    FML_LOG(ERROR) << "BDFlutter SetupWithoutEngine return, is_setup_ is true";
    return false;
  }
  if (!platform_view || !rasterizer || !io_manager) {
    FML_LOG(ERROR) << "BDFlutter SetupWithoutEngine return, platform_view:"
    << !platform_view << " rasterizer: " << !rasterizer << "io_manager: " << !io_manager;
    return false;
  }
  is_without_engine_setup_ = true;
  platform_view_ = std::move(platform_view);
  rasterizer_ = std::move(rasterizer);
  weak_platform_view_ = platform_view_->GetWeakPtr();
  weak_rasterizer_ = rasterizer_->GetWeakPtr();
  io_manager_ = std::move(io_manager);
  is_setup_ = is_setup_ | is_engine_setup_;
  if (is_setup_) {
    FML_LOG(ERROR) << "BDFlutter SetupEngine is call before SetupWithoutEngine";
  }
  return true;

}

bool Shell::SetupEngine(std::unique_ptr<Engine> engine){
  if (is_setup_) {
    FML_LOG(ERROR) << "BDFlutter SetupEngine return, is_setup_ is true";
    return false;
  }
  if (!engine) {
    FML_LOG(ERROR) << "BDFlutter SetupEngine return, engine is nullptr";
    return false;
  }
  is_engine_setup_ = true;
  engine_ = std::move(engine);
  weak_engine_ = engine_->GetWeakPtr();
  is_setup_ = is_setup_ | is_without_engine_setup_;

  vm_->GetServiceProtocol()->AddHandler(this, GetServiceProtocolDescription());

  PersistentCache::GetCacheForProcess()->AddWorkerTaskRunner(
    task_runners_.GetIOTaskRunner());

  PersistentCache::GetCacheForProcess()->SetIsDumpingSkp(
    settings_.dump_skp_on_shader_compilation);

  display_refresh_rate_ = weak_engine_.getUnsafe()->GetDisplayRefreshRate();

  return true;
}

void Shell::SetPreloadState(bool preload) {
  is_preload_ = preload;
}

bool Shell::IsInShellNotBlockAndPosting() {
  return is_preload_ && !is_setup_;
}
// END

}  // namespace flutter
