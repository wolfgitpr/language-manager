#include "G2pDriver.h"

#include <iostream>
#include <onnxruntime_cxx_api.h>

#include <stdcorelib/str.h>
#include <stdcorelib/support/sharedlibrary.h>

namespace LangMgr
{
    class G2pDriver::Impl {
    public:
        std::unique_ptr<stdc::SharedLibrary> ortDSO;
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
            std::cerr << "Load library failed: %1 [%2]" << dylib->lastError() << path;
            return false;
        }

#ifdef _WIN32
        stdc::SharedLibrary::setLibraryPath(orgLibPath);
#endif

        const auto handle = reinterpret_cast<OrtApiBase *(ORT_API_CALL *)()>(dylib->resolve("OrtGetApiBase"));
        if (!handle) {
            std::cerr << "Failed to get API handle: %1 [%2]" << dylib->lastError() << path;
            return false;
        }

        const auto apiBase = handle();
        const auto api = apiBase->GetApi(ORT_API_VERSION);
        if (!api) {
            std::cout << "Failed to get API instance for version %1" << ORT_API_VERSION;
            return false;
        }

        impl_->ortDSO.swap(dylib);
        ortApiBase_ = apiBase;
        ortApi_ = api;
        loaded_ = true;
        return true;
    }

    void G2pDriver::unload() {
        if (loaded_) {
            impl_->ortDSO->close();
            impl_->ortDSO.reset();
            ortApiBase_ = nullptr;
            ortApi_ = nullptr;
            loaded_ = false;
        }
    }
} // namespace LangMgr
