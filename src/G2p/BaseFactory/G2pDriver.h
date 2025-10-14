#ifndef LANGUAGE_MANAGER_G2PDRIVER_H
#define LANGUAGE_MANAGER_G2PDRIVER_H

#include <filesystem>
#include <memory>
#include <synthrt/Support/Expected.h>

struct OrtApi;
struct OrtApiBase;

namespace LangMgr
{
    class G2pDriver {
    public:
        G2pDriver();
        ~G2pDriver();

        srt::Expected<void> load(const std::filesystem::path &path);
        bool isLoaded() const { return loaded_; }
        void unload();

        const OrtApi *api() const { return ortApi_; }
        const OrtApiBase *apiBase() const { return ortApiBase_; }

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;
        bool loaded_ = false;
        const OrtApiBase *ortApiBase_ = nullptr;
        const OrtApi *ortApi_ = nullptr;
    };

} // namespace LangMgr

#endif // LANGUAGE_MANAGER_G2PDRIVER_H
