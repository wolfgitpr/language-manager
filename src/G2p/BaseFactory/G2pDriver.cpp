#include "G2pDriver.h"

#include <iostream>
#include <onnxruntime_cxx_api.h>

#include <stdcorelib/str.h>
#include <stdcorelib/support/sharedlibrary.h>

#if defined(_WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace LangMgr
{
    class G2pDriver::Impl {
    public:
        std::unique_ptr<stdc::SharedLibrary> ortDSO;
        bool fromCurrentProcess = false;
    };

    G2pDriver::G2pDriver() : impl_(std::make_unique<Impl>()) {}

    G2pDriver::~G2pDriver() { unload(); }

    bool G2pDriver::load(const std::filesystem::path &path) {
        if (loaded_) {
            std::cout << "G2pDriver::load: already loaded" << std::endl;
            return true;
        }

        auto dylib = std::make_unique<stdc::SharedLibrary>();

#ifdef _WIN32
        const auto orgLibPath = stdc::SharedLibrary::setLibraryPath(path.parent_path());
#endif

        if (!dylib->open(path, stdc::SharedLibrary::ResolveAllSymbolsHint)) {
#ifdef _WIN32
            stdc::SharedLibrary::setLibraryPath(orgLibPath);
#endif
            std::cerr << "Load library failed: " << dylib->lastError() << " [" << path << "]" << std::endl;
            return false;
        }

#ifdef _WIN32
        stdc::SharedLibrary::setLibraryPath(orgLibPath);
#endif

        const auto handle = dylib->resolve("OrtGetApiBase");
        if (!handle) {
            std::cerr << "Failed to get API handle: " << dylib->lastError() << " [" << path << "]" << std::endl;
            return false;
        }

        impl_->ortDSO.swap(dylib);
        return initializeFromHandle(handle);
    }

    bool G2pDriver::loadFromProcess() {
        if (loaded_) {
            std::cout << "G2pDriver::loadFromProcess: already loaded" << std::endl;
            return true;
        }

        void *handle = nullptr;

#if defined(_WIN32)
        // Windows: Use GetModuleHandle and GetProcAddress
        if (const HMODULE module = GetModuleHandleA("onnxruntime.dll")) {
            handle = GetProcAddress(module, "OrtGetApiBase");
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
            std::cerr << "Failed to find OrtGetApiBase in current process" << std::endl;
            return false;
        }

        impl_->fromCurrentProcess = true;
        return initializeFromHandle(handle);
    }

    bool G2pDriver::initializeFromHandle(void *handle) {
        if (!handle) {
            std::cerr << "Invalid handle for OrtGetApiBase" << std::endl;
            return false;
        }

        // Cast the handle to the appropriate function pointer type
        const auto getApiBase = reinterpret_cast<OrtApiBase *(ORT_API_CALL *)()>(handle);
        const OrtApiBase *apiBase = getApiBase();
        if (!apiBase) {
            std::cerr << "Failed to get API base" << std::endl;
            return false;
        }

        const auto api = apiBase->GetApi(ORT_API_VERSION);
        if (!api) {
            std::cerr << "Failed to get API instance for version " << ORT_API_VERSION << std::endl;
            return false;
        }

        ortApiBase_ = apiBase;
        ortApi_ = api;
        loaded_ = true;

        std::cout << "ONNX Runtime loaded successfully" << std::endl;
        return true;
    }

    void G2pDriver::unload() {
        if (loaded_) {
            // Only close the library if it was loaded from a file
            if (impl_->ortDSO) {
                impl_->ortDSO->close();
                impl_->ortDSO.reset();
            }
            ortApiBase_ = nullptr;
            ortApi_ = nullptr;
            loaded_ = false;
            impl_->fromCurrentProcess = false;

            std::cout << "ONNX Runtime unloaded" << std::endl;
        }
    }
} // namespace LangMgr
