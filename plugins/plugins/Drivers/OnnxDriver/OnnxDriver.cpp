#include "OnnxDriver.h"

#include <stdcorelib/pimpl.h>
#include <stdcorelib/str.h>
#include <stdcorelib/support/sharedlibrary.h>

#include <LangPlugins/Api/Drivers/Onnx/1/OnnxDriverApiL1.h>

#include "OnnxDriver_Logger.h"
#include "OnnxSession.h"
#include "internal/Env.h"

#ifndef ORT_API_MANUAL_INIT
#error "LangPlugins requires ort to be manually initialized, but ORT_API_MANUAL_INIT is not set!"
#endif

#include <onnxruntime_cxx_api.h>

#if defined(_WIN32)
#include <windows.h>
#define ONNXRUNTIME_DYLIB_FILENAME _TSTR("onnxruntime.dll")
#else
#include <dlfcn.h>
#if defined(__APPLE__)
#define ONNXRUNTIME_DYLIB_FILENAME _TSTR("libonnxruntime.dylib")
#else
#define ONNXRUNTIME_DYLIB_FILENAME _TSTR("libonnxruntime.so")
#endif
#endif

namespace fs = std::filesystem;

namespace LangPlugins
{

    using namespace Api;

    namespace onnxDriver
    {

        LangCore::LogCategory Log("onnxDriver");

    }

    using onnxDriver::Log;

    class OnnxDriver::Impl {
    public:
        Impl() {}

        ~Impl() {}

        LangCore::Expected<void> load(const fs::path &path) {
            Log.langCoreInfo("Init - Loading onnx environment from path: %1", path);

            auto dylib = std::make_unique<stdc::SharedLibrary>();

            /**
             *  1. Load Ort shared library and create handle
             */
            Log.langCoreDebug("Init - Loading ORT shared library from %1", path);
#ifdef _WIN32
            const auto orgLibPath = stdc::SharedLibrary::setLibraryPath(path.parent_path());
#endif
            if (!dylib->open(path, stdc::SharedLibrary::ResolveAllSymbolsHint)) {
                std::string msg = stdc::formatN("Load library failed: %1 [%2]", dylib->lastError(), path);
                Log.langCoreCritical("Init - %1", msg);
                return LangCore::Error(LangCore::Error::SessionError, std::move(msg));
            }
#ifdef _WIN32
            stdc::SharedLibrary::setLibraryPath(orgLibPath);
#endif

            /**
             *  2. Get Ort API getter handle
             */
            Log.langCoreDebug("Init - Getting ORT API handle");
            const auto handle = reinterpret_cast<OrtApiBase *(ORT_API_CALL *)()>(dylib->resolve("OrtGetApiBase"));
            if (!handle) {
                std::string msg = stdc::formatN("Failed to get API handle: %1 [%2]", dylib->lastError(), path);
                Log.langCoreCritical("Init - %1", msg);
                return LangCore::Error(LangCore::Error::SessionError, std::move(msg));
            }

            return initializeFromHandle(handle, std::move(dylib));
        }

        LangCore::Expected<void> loadFromProcess() {
            Log.langCoreInfo("Init - Loading onnx environment from current process");

            if (loaded) {
                Log.langCoreWarning("Init - Onnx environment already loaded");
                return LangCore::Expected<void>();
            }

            void *handle = nullptr;

#if defined(_WIN32)
            // Windows: Use GetModuleHandle and GetProcAddress
            if (const HMODULE module = GetModuleHandleA("onnxruntime.dll")) {
                handle = reinterpret_cast<void *>(GetProcAddress(module, "OrtGetApiBase"));
            }
#else
            // Linux/Mac: Use dlopen and dlsym with RTLD_NOLOAD to search already loaded libraries
            void *library = dlopen("libonnxruntime.so", RTLD_LAZY | RTLD_NOLOAD);
            if (!library) {
                library = dlopen("libonnxruntime.dylib", RTLD_LAZY | RTLD_NOLOAD);
            }
            if (library) {
                handle = dlsym(library, "OrtGetApiBase");
                dlclose(library); // We don't need to keep this handle open
            }
#endif

            if (!handle) {
                std::string msg = "Failed to find OrtGetApiBase in current process";
                Log.langCoreCritical("Init - %1", msg);
                return LangCore::Error(LangCore::Error::SessionError, std::move(msg));
            }

            fromCurrentProcess = true;
            return initializeFromHandle(reinterpret_cast<OrtApiBase *(ORT_API_CALL *)()>(handle),
                                        nullptr // No need to manage library lifecycle for process-loaded
            );
        }

        LangCore::Expected<void> initializeFromHandle(OrtApiBase *(ORT_API_CALL *handle)(),
                                                      std::unique_ptr<stdc::SharedLibrary> dylib) {
            /**
             *  3. Check Ort API
             */
            Log.langCoreDebug("Init - ORT_API_VERSION is %1", ORT_API_VERSION);
            const auto apiBase = handle();
            const auto api = apiBase->GetApi(ORT_API_VERSION);
            if (!api) {
                std::string msg = stdc::formatN("Failed to get API instance for version %1", ORT_API_VERSION);
                Log.langCoreCritical("Init - %1", msg);
                return LangCore::Error(LangCore::Error::SessionError, std::move(msg));
            }
            Log.langCoreDebug("Init - ORT library version is %1", apiBase->GetVersionString());

            /**
             *  4. Successfully get Ort API
             */
            Ort::InitApi(api);

            if (dylib) {
                ortDSO.swap(dylib);
            }

            loaded = true;
            ortApiBase = apiBase;
            ortApi = api;

            Log.langCoreInfo("Init - Onnx environment Load successful");
            return LangCore::Expected<void>();
        }

        std::unique_ptr<stdc::SharedLibrary> ortDSO;

        // Metadata
        bool loaded = false;
        bool fromCurrentProcess = false;
        fs::path ortPath;

        // Library data
        void *hLibrary = nullptr;
        const OrtApi *ortApi = nullptr;
        const OrtApiBase *ortApiBase = nullptr;
    };

    OnnxDriver::OnnxDriver() : _impl(std::make_unique<Impl>()) {}

    OnnxDriver::~OnnxDriver() {}

    std::string OnnxDriver::arch() const { return "onnx"; }

    std::string OnnxDriver::backend() const { return Onnx::L1::API_NAME; }

    LangCore::Expected<void> OnnxDriver::initialize(const LangCore::NO<LangCore::TaskInitArgs> &args) {
        __stdc_impl_t;

        if (args->objectName() != Onnx::L1::API_NAME) {
            return LangCore::Error{
                LangCore::Error::InvalidArgument,
                stdc::formatN(R"(invalid driver name: expected "%s", got "%s")", Onnx::L1::API_NAME,
                              args->objectName()),
            };
        }

        const auto onnxArgs = args.as<Onnx::L1::DriverInitArgs>();
        if (!onnxArgs) {
            return LangCore::Error{LangCore::Error::InvalidArgument, "onnx args is null pointer"};
        }

        // Example logging
        Log.langCoreDebug("initialize: driver name: %1", args->objectName());

        if (impl.loaded) {
            return LangCore::Error{
                LangCore::Error::FileDuplicated,
                "onnx runtime has been initialized by another instance",
            };
        }

        if (!onnxArgs->loadFromProcess) {
            const auto dllPath = onnxArgs->runtimePath / ONNXRUNTIME_DYLIB_FILENAME;
            if (auto result = impl.load(dllPath); !result)
                return result;
        } else {
            if (auto result = impl.loadFromProcess(); !result)
                return result;
        }

        onnxDriver::Env::DeviceConfig devConfig;
        devConfig.ep = onnxArgs->ep;
        devConfig.deviceIndex = onnxArgs->deviceIndex;
        onnxDriver::Env::setDeviceConfig(devConfig);
        return LangCore::Expected<void>();
    }

    LangCore::Expected<void> OnnxDriver::loadFromProcess() const { return _impl->loadFromProcess(); }

    LangCore::NO<LangCore::SessionTask> OnnxDriver::createSession() {
        auto session = LangCore::NO<OnnxSession>::create();
        return session;
    }

} // namespace LangPlugins
