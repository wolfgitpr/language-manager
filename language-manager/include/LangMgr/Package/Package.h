#ifndef LANGUAGE_MANAGER_PACKAGEREF_H
#define LANGUAGE_MANAGER_PACKAGEREF_H

#include <filesystem>
#include <string>
#include <vector>

#include <LangMgr/Support/DisplayText.h>
#include <LangMgr/Support/Expected.h>
#include <stdcorelib/adt/array_view.h>
#include <stdcorelib/support/versionnumber.h>

namespace LangMgr
{
    class Manager;
    class ModuleSpec;
    class PackageData;
    class ScopedPackageRef;

    class LANGMGR_EXPORT Package {
    public:
        Package();
        ~Package();

        bool isValid() const { return Mgr() != nullptr; }
        bool close();

        const std::string &id() const;
        stdc::VersionNumber version() const;
        stdc::VersionNumber compatVersion() const;

        DisplayText description() const;
        DisplayText vendor() const;
        DisplayText copyright() const;
        const std::filesystem::path &readme() const;
        const std::string &url() const;

        std::vector<ModuleSpec *> moduleSpecs(const std::string_view &category) const;
        ModuleSpec *moduleSpec(const std::string_view &category, const std::string_view &id) const;

        const std::filesystem::path &path() const;
        Error error() const;
        bool isLoaded() const;
        PackageManager *Mgr() const;

    private:
        explicit Package(PackageData *data) : _data(data) {}
        PackageData *_data;

        friend class ModuleSpec;
        friend class PackageManager;
        friend class ScopedPackageRef;
    };

    class ScopedPackageRef : public Package {
    public:
        ScopedPackageRef() = default;
        explicit ScopedPackageRef(Package &&RHS) { std::swap(_data, RHS._data); }
        ~ScopedPackageRef() { forceClose(); }

        ScopedPackageRef &operator=(Package &&RHS) {
            if (this != &RHS) {
                forceClose();
                std::swap(_data, RHS._data);
            }
            return *this;
        }

        Package release() {
            Package ref;
            std::swap(_data, ref._data);
            return ref;
        }

    private:
        LANGMGR_EXPORT void forceClose();
        STDCORELIB_DISABLE_COPY(ScopedPackageRef);
    };
} // namespace LangMgr
#endif
