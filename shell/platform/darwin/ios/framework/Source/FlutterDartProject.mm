// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#define FML_USED_ON_EMBEDDER

#import "flutter/shell/platform/darwin/ios/framework/Source/FlutterDartProject_Internal.h"

#include "flutter/common/constants.h"
#include "flutter/common/task_runners.h"
#include "flutter/fml/mapping.h"
#include "flutter/fml/message_loop.h"
#include "flutter/fml/platform/darwin/scoped_nsobject.h"
#include "flutter/runtime/dart_vm.h"
#include "flutter/shell/common/shell.h"
#include "flutter/shell/common/switches.h"
#import "flutter/shell/platform/darwin/common/command_line.h"
#import "flutter/shell/platform/darwin/ios/framework/Headers/FlutterViewController.h"
// BD ADD: START
#include "flutter/fml/file.h"
#include "flutter/fml/mapping.h"
#include "flutter/bdflutter/shell/platform/darwin/ios/framework/Source/FlutterCompressSizeModeManager.h"
// END

extern "C" {
#if FLUTTER_RUNTIME_MODE == FLUTTER_RUNTIME_MODE_DEBUG
// Used for debugging dart:* sources.
extern const uint8_t kPlatformStrongDill[];
extern const intptr_t kPlatformStrongDillSize;
#endif
}

static const char* kApplicationKernelSnapshotFileName = "kernel_blob.bin";
// BD ADD: START
NSString* const FlutterCompressSizeModeErrorDomain = @"FlutterCompressSizeModeErrorDomain";
static NSString* const kFLTAssetsPath = @"FLTAssetsPath";
static NSString* const kFlutterAssets = @"flutter_assets";
static FlutterCompressSizeModeMonitor kFlutterCompressSizeModeMonitor = nil;
static BOOL highQoS = false;
// END

flutter::Settings FLTDefaultSettingsForBundle(NSBundle* bundle) {
  auto command_line = flutter::CommandLineFromNSProcessInfo();

  // Precedence:
  // 1. Settings from the specified NSBundle.
  // 2. Settings passed explicitly via command-line arguments.
  // 3. Settings from the NSBundle with the default bundle ID.
  // 4. Settings from the main NSBundle and default values.

  NSBundle* mainBundle = [NSBundle mainBundle];
  NSBundle* engineBundle = [NSBundle bundleForClass:[FlutterViewController class]];

  bool hasExplicitBundle = bundle != nil;
  if (bundle == nil) {
    bundle = [NSBundle bundleWithIdentifier:[FlutterDartProject defaultBundleIdentifier]];
  }
  if (bundle == nil) {
    bundle = mainBundle;
  }

  auto settings = flutter::SettingsFromCommandLine(command_line);

  settings.task_observer_add = [](intptr_t key, fml::closure callback) {
    fml::MessageLoop::GetCurrent().AddTaskObserver(key, std::move(callback));
  };

  settings.task_observer_remove = [](intptr_t key) {
    fml::MessageLoop::GetCurrent().RemoveTaskObserver(key);
  };

  // The command line arguments may not always be complete. If they aren't, attempt to fill in
  // defaults.

  // Flutter ships the ICU data file in the bundle of the engine. Look for it there.
  if (settings.icu_data_path.size() == 0) {
    NSString* icuDataPath = [engineBundle pathForResource:@"icudtl" ofType:@"dat"];
    if (icuDataPath.length > 0) {
      settings.icu_data_path = icuDataPath.UTF8String;
    }
  }

  if (flutter::DartVM::IsRunningPrecompiledCode()) {
    if (hasExplicitBundle) {
      NSString* executablePath = bundle.executablePath;
      if ([[NSFileManager defaultManager] fileExistsAtPath:executablePath]) {
        settings.application_library_path.push_back(executablePath.UTF8String);
      }
    }

    // No application bundle specified.  Try a known location from the main bundle's Info.plist.
    if (settings.application_library_path.size() == 0) {
      NSString* libraryName = [mainBundle objectForInfoDictionaryKey:@"FLTLibraryPath"];
      NSString* libraryPath = [mainBundle pathForResource:libraryName ofType:@""];
      if (libraryPath.length > 0) {
        NSString* executablePath = [NSBundle bundleWithPath:libraryPath].executablePath;
        if (executablePath.length > 0) {
          settings.application_library_path.push_back(executablePath.UTF8String);
        }
      }
    }

    // In case the application bundle is still not specified, look for the App.framework in the
    // Frameworks directory.
    if (settings.application_library_path.size() == 0) {
      NSString* applicationFrameworkPath = [mainBundle pathForResource:@"Frameworks/App.framework"
                                                                ofType:@""];
      if (applicationFrameworkPath.length > 0) {
        NSString* executablePath =
            [NSBundle bundleWithPath:applicationFrameworkPath].executablePath;
        if (executablePath.length > 0) {
          settings.application_library_path.push_back(executablePath.UTF8String);
        }
      }
    }
  }

  // Checks to see if the flutter assets directory is already present.
  if (settings.assets_path.size() == 0) {
    NSString* assetsName = [FlutterDartProject flutterAssetsName:bundle];
    NSString* assetsPath = [bundle pathForResource:assetsName ofType:@""];

    if (assetsPath.length == 0) {
      assetsPath = [mainBundle pathForResource:assetsName ofType:@""];
    }

    if (assetsPath.length == 0) {
      // BD MOD: START
      // NSLog(@"Failed to find assets path for \"%@\"", assetsName);
      if (![FlutterCompressSizeModeManager sharedInstance].isCompressSizeMode) {
        NSLog(@"Failed to find assets path for \"%@\"", assetsName);
      }
      // END
    } else {
      settings.assets_path = assetsPath.UTF8String;

      // Check if there is an application kernel snapshot in the assets directory we could
      // potentially use.  Looking for the snapshot makes sense only if we have a VM that can use
      // it.
      if (!flutter::DartVM::IsRunningPrecompiledCode()) {
        NSURL* applicationKernelSnapshotURL =
            [NSURL URLWithString:@(kApplicationKernelSnapshotFileName)
                   relativeToURL:[NSURL fileURLWithPath:assetsPath]];
        if ([[NSFileManager defaultManager] fileExistsAtPath:applicationKernelSnapshotURL.path]) {
          settings.application_kernel_asset = applicationKernelSnapshotURL.path.UTF8String;
        } else {
          NSLog(@"Failed to find snapshot: %@", applicationKernelSnapshotURL.path);
        }
      }
    }
  }

  // Domain network configuration
  NSDictionary* appTransportSecurity =
      [mainBundle objectForInfoDictionaryKey:@"NSAppTransportSecurity"];
  settings.may_insecurely_connect_to_all_domains =
      [FlutterDartProject allowsArbitraryLoads:appTransportSecurity];
  settings.domain_network_policy =
      [FlutterDartProject domainNetworkPolicy:appTransportSecurity].UTF8String;

  // SkParagraph text layout library
  NSNumber* enableSkParagraph = [mainBundle objectForInfoDictionaryKey:@"FLTEnableSkParagraph"];
  settings.enable_skparagraph = (enableSkParagraph != nil) ? enableSkParagraph.boolValue : false;

#if FLUTTER_RUNTIME_MODE == FLUTTER_RUNTIME_MODE_DEBUG
  // There are no ownership concerns here as all mappings are owned by the
  // embedder and not the engine.
  auto make_mapping_callback = [](const uint8_t* mapping, size_t size) {
    return [mapping, size]() { return std::make_unique<fml::NonOwnedMapping>(mapping, size); };
  };

  settings.dart_library_sources_kernel =
      make_mapping_callback(kPlatformStrongDill, kPlatformStrongDillSize);
#endif  // FLUTTER_RUNTIME_MODE == FLUTTER_RUNTIME_MODE_DEBUG

  // If we even support setting this e.g. from the command line or the plist,
  // we should let the user override it.
  // Otherwise, we want to set this to a value that will avoid having the OS
  // kill us. On most iOS devices, that happens somewhere near half
  // the available memory.
  // The VM expects this value to be in megabytes.
  if (settings.old_gen_heap_size <= 0) {
    settings.old_gen_heap_size = std::round([NSProcessInfo processInfo].physicalMemory * .48 /
                                            flutter::kMegaByteSizeInBytes);
  }
  // BD ADD:
  settings.high_qos = highQoS;
  return settings;
}

@implementation FlutterDartProject {
  flutter::Settings _settings;
}

#pragma mark - Override base class designated initializers

- (instancetype)init {
  return [self initWithPrecompiledDartBundle:nil];
}

#pragma mark - Designated initializers

- (instancetype)initWithPrecompiledDartBundle:(nullable NSBundle*)bundle {
  self = [super init];

  if (self) {
    _settings = FLTDefaultSettingsForBundle(bundle);
  }

  return self;
}

- (instancetype)initWithSettings:(const flutter::Settings&)settings {
  self = [self initWithPrecompiledDartBundle:nil];

  if (self) {
    _settings = settings;
    // BD ADD: START
    [[FlutterCompressSizeModeManager sharedInstance]
        updateSettingsIfNeeded:_settings
                       monitor:kFlutterCompressSizeModeMonitor];
    // END
  }

  return self;
}

#pragma mark - PlatformData accessors

- (const flutter::PlatformData)defaultPlatformData {
  flutter::PlatformData PlatformData;
  PlatformData.lifecycle_state = std::string("AppLifecycleState.detached");
  return PlatformData;
}

#pragma mark - Settings accessors

- (const flutter::Settings&)settings {
  return _settings;
}

- (flutter::RunConfiguration)runConfiguration {
  return [self runConfigurationForEntrypoint:nil];
}

- (flutter::RunConfiguration)runConfigurationForEntrypoint:(nullable NSString*)entrypointOrNil {
  return [self runConfigurationForEntrypoint:entrypointOrNil libraryOrNil:nil];
}

- (flutter::RunConfiguration)runConfigurationForEntrypoint:(nullable NSString*)entrypointOrNil
                                              libraryOrNil:(nullable NSString*)dartLibraryOrNil {
  auto config = flutter::RunConfiguration::InferFromSettings(_settings);
  if (dartLibraryOrNil && entrypointOrNil) {
    config.SetEntrypointAndLibrary(std::string([entrypointOrNil UTF8String]),
                                   std::string([dartLibraryOrNil UTF8String]));

  } else if (entrypointOrNil) {
    config.SetEntrypoint(std::string([entrypointOrNil UTF8String]));
  }
  return config;
}

#pragma mark - Assets-related utilities

+ (NSString*)flutterAssetsName:(NSBundle*)bundle {
  if (bundle == nil) {
    bundle = [NSBundle bundleWithIdentifier:[FlutterDartProject defaultBundleIdentifier]];
  }
  if (bundle == nil) {
    bundle = [NSBundle mainBundle];
  }
  // BD MOD:
  // NSString* flutterAssetsName = [bundle objectForInfoDictionaryKey:@"FLTAssetsPath"];
  NSString* flutterAssetsName = [bundle objectForInfoDictionaryKey:kFLTAssetsPath];
  if (flutterAssetsName == nil) {
    // BD MOD:
    // flutterAssetsName = @"Frameworks/App.framework/flutter_assets";
    flutterAssetsName = [NSString stringWithFormat:@"Frameworks/App.framework/%@", kFlutterAssets];
  }
  return flutterAssetsName;
}

+ (NSString*)domainNetworkPolicy:(NSDictionary*)appTransportSecurity {
  // https://developer.apple.com/documentation/bundleresources/information_property_list/nsapptransportsecurity/nsexceptiondomains
  NSDictionary* exceptionDomains = [appTransportSecurity objectForKey:@"NSExceptionDomains"];
  if (exceptionDomains == nil) {
    return @"";
  }
  NSMutableArray* networkConfigArray = [[NSMutableArray alloc] init];
  for (NSString* domain in exceptionDomains) {
    NSDictionary* domainConfiguration = [exceptionDomains objectForKey:domain];
    // Default value is false.
    bool includesSubDomains =
        [[domainConfiguration objectForKey:@"NSIncludesSubdomains"] boolValue];
    bool allowsCleartextCommunication =
        [[domainConfiguration objectForKey:@"NSExceptionAllowsInsecureHTTPLoads"] boolValue];
    [networkConfigArray addObject:@[
      domain, includesSubDomains ? @YES : @NO, allowsCleartextCommunication ? @YES : @NO
    ]];
  }
  NSData* jsonData = [NSJSONSerialization dataWithJSONObject:networkConfigArray
                                                     options:0
                                                       error:NULL];
  return [[NSString alloc] initWithData:jsonData encoding:NSUTF8StringEncoding];
}

+ (bool)allowsArbitraryLoads:(NSDictionary*)appTransportSecurity {
  return [[appTransportSecurity objectForKey:@"NSAllowsArbitraryLoads"] boolValue];
}

+ (NSString*)lookupKeyForAsset:(NSString*)asset {
  return [self lookupKeyForAsset:asset fromBundle:nil];
}

+ (NSString*)lookupKeyForAsset:(NSString*)asset fromBundle:(nullable NSBundle*)bundle {
  NSString* flutterAssetsName = [FlutterDartProject flutterAssetsName:bundle];
  return [NSString stringWithFormat:@"%@/%@", flutterAssetsName, asset];
}

+ (NSString*)lookupKeyForAsset:(NSString*)asset fromPackage:(NSString*)package {
  return [self lookupKeyForAsset:asset fromPackage:package fromBundle:nil];
}

+ (NSString*)lookupKeyForAsset:(NSString*)asset
                   fromPackage:(NSString*)package
                    fromBundle:(nullable NSBundle*)bundle {
  return [self lookupKeyForAsset:[NSString stringWithFormat:@"packages/%@/%@", package, asset]
                      fromBundle:bundle];
}

+ (NSString*)defaultBundleIdentifier {
  return @"io.flutter.flutter.app";
}

// BD ADD: START
+ (NSString *)flutterAssetAbsolutePath:(NSString *)asset fromPackage:(NSString *)package fromBundle:(NSBundle *)bundle {
  if (bundle == nil) {
    bundle = [NSBundle bundleWithIdentifier:[FlutterDartProject defaultBundleIdentifier]];
  }
  if (bundle == nil) {
    bundle = [NSBundle mainBundle];
  }
  NSString* flutterAssetsName = [bundle objectForInfoDictionaryKey:kFLTAssetsPath];
  if (flutterAssetsName == nil) {
    flutterAssetsName = [NSString stringWithFormat:@"Frameworks/App.framework/%@", kFlutterAssets];
  }
  NSString *assetName = asset;
  if (package.length > 0) {
    assetName = [NSString stringWithFormat:@"packages/%@/%@", package, asset];
  }
  if (self.isCompressSizeMode) {
    FlutterCompressSizeModeManager *compressMgr = [FlutterCompressSizeModeManager sharedInstance];
    return [NSString stringWithFormat:@"%@/%@", compressMgr.assetsPath, assetName];
  } else {
    NSString *bundle = [[NSBundle mainBundle] pathForResource:flutterAssetsName ofType:@""];
    return [NSString stringWithFormat:@"%@/%@", bundle, assetName];
  }
}
// END

#pragma mark - Settings utilities

- (void)setPersistentIsolateData:(NSData*)data {
  if (data == nil) {
    return;
  }

  NSData* persistent_isolate_data = [data copy];
  fml::NonOwnedMapping::ReleaseProc data_release_proc = [persistent_isolate_data](auto, auto) {
    [persistent_isolate_data release];
  };
  _settings.persistent_isolate_data = std::make_shared<fml::NonOwnedMapping>(
      static_cast<const uint8_t*>(persistent_isolate_data.bytes),  // bytes
      persistent_isolate_data.length,                              // byte length
      data_release_proc                                            // release proc
  );
}

#pragma mark - PlatformData utilities

// BD ADD: START
+ (void)setCompressSizeModeMonitor:(FlutterCompressSizeModeMonitor)flutterCompressSizeModeMonitor {
  kFlutterCompressSizeModeMonitor = [flutterCompressSizeModeMonitor copy];
}

+ (void)predecompressData {
  [[FlutterCompressSizeModeManager sharedInstance]
      decompressDataAsyncIfNeeded:YES
                          monitor:kFlutterCompressSizeModeMonitor];
  [[FlutterCompressSizeModeManager sharedInstance] removePreviousDecompressedDataAsync];
}

+ (NSString*)flutterAssetsPath {
  NSBundle* bundle = [NSBundle bundleWithIdentifier:[FlutterDartProject defaultBundleIdentifier]];
  if (bundle == nil) {
    bundle = [NSBundle mainBundle];
  }
  NSString* flutterAssetsPath = [bundle objectForInfoDictionaryKey:kFLTAssetsPath];
  if (flutterAssetsPath == nil) {
    flutterAssetsPath = kFlutterAssets;
  }
  return flutterAssetsPath;
}

+ (BOOL)isCompressSizeMode {
  return [FlutterCompressSizeModeManager sharedInstance].isCompressSizeMode;
}

+ (BOOL)decompressData:(NSError**)error {
  [self predecompressData];
  return YES;
}

+ (BOOL)needDecompressData {
  return [[FlutterCompressSizeModeManager sharedInstance] needDecompressData];
}

- (void)setLeakDartVMEnabled:(BOOL)enabled {
  _settings.leak_vm = enabled;
}

+ (void)setThreadHighQoS:(BOOL)enabled {
  highQoS = enabled;
}
// END

@end
