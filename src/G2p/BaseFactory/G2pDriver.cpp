#include "G2pDriver.h"
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

    srt::Expected<void> G2pDriver::load(const std::filesystem::path &path) {
        if (loaded_) {
            return srt::Error(srt::Error::, "ORT driver already loaded");
        }

        auto dylib = std::make_unique<stdc::SharedLibrary>();

#ifdef _WIN32
        auto orgLibPath = stdc::SharedLibrary::setLibraryPath(path.parent_path());
#endif

        if (!dylib->open(path, stdc::SharedLibrary::ResolveAllSymbolsHint)) {
#ifdef _WIN32
            stdc::SharedLibrary::setLibraryPath(orgLibPath);
#endif
            return srt::Error(srt::Error::SessionError,
                              stdc::formatN("Load library failed: %1 [%2]", dylib->lastError(), path));
        }

#ifdef _WIN32
        stdc::SharedLibrary::setLibraryPath(orgLibPath);
#endif

        // 获取ORT API
        auto handle = reinterpret_cast<OrtApiBase *(ORT_API_CALL *)()>(dylib->resolve("OrtGetApiBase"));
        if (!handle) {
            return srt::Error(srt::Error::SessionError,
                              stdc::formatN("Failed to get API handle: %1 [%2]", dylib->lastError(), path));
        }

        auto apiBase = handle();
        auto api = apiBase->GetApi(ORT_API_VERSION);
        if (!api) {
            return srt::Error(srt::Error::SessionError,
                              stdc::formatN("Failed to get API instance for version %1", ORT_API_VERSION));
        }

        impl_->ortDSO.swap(dylib);
        ortApiBase_ = apiBase;
        ortApi_ = api;
        loaded_ = true;

        return {};
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
