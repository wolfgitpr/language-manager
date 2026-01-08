#ifndef LANGUAGE_MANAGER_PACKAGEREF_H
#define LANGUAGE_MANAGER_PACKAGEREF_H

#include <filesystem>
#include <string>
#include <utility>

#include <stdcorelib/adt/array_view.h>
#include <stdcorelib/support/versionnumber.h>

#include <LangMgr/Support/DisplayText.h>
#include <LangMgr/Support/Expected.h>

namespace LangMgr
{

    class Manager;
    class ModuleDefinition;

    struct PackageDependency {
        std::string id;
        stdc::VersionNumber version;
        bool required;

        explicit PackageDependency(const bool required = true) : required(required) {}

        PackageDependency(std::string id, const stdc::VersionNumber version, const bool required = true) :
            id(std::move(id)), version(version), required(required) {}

        bool operator==(const PackageDependency &other) const { return id == other.id && version == other.version; }

        LANGMGR_EXPORT static Expected<PackageDependency> fromJsonValue(const JsonValue &val);
    };

    class PackageData;

    class ScopedPackageRef;

    /// PackageRef - Represents a reference to a package opened by \c Manager, does not own the
    /// package resources.
    class LANGMGR_EXPORT Package {
    public:
        Package();
        ~Package();

        bool isValid() const { return Mgr() != nullptr; }

        /// Close the package or reduce its reference count in \c Manager. When all \c PackageRef
        /// instances opened using \c Manager::open are closed, its shared internal data will be
        /// deleted. Anyone creating a \c PackageRef instance using a copy construct should be aware
        /// of the lifetime of the internal data.
        bool close();

        const std::string &id() const;
        stdc::VersionNumber version() const;
        stdc::VersionNumber compatVersion() const; // maybe not used

        /// Author information, for display purposes only.
        DisplayText description() const;
        DisplayText vendor() const;
        DisplayText copyright() const;
        const std::filesystem::path &readme() const;
        const std::string &url() const;

        std::vector<ModuleDefinition *> moduleSpecs(const std::string_view &category) const;
        ModuleDefinition *moduleSpec(const std::string_view &category, const std::string_view &id) const;

        /// Loader-specific
        const std::filesystem::path &path() const;
        stdc::array_view<PackageDependency> dependencies() const;

        /// The error will be set if the package is not opened or loaded correctly.
        Error error() const;

        /// Returns true if and only if the \c noLoad option is not specified when opening the
        /// package and the loading is successful.
        ///
        /// If the package is successfully loaded, its resources are managed by \c Manager, which
        /// maintains its reference count.
        bool isLoaded() const;

        /// Returns the \c Manager instance that loaded this package.
        Manager *Mgr() const;

    private:
        explicit Package(PackageData *data) : _data(data) {}

        PackageData *_data;

        friend class ModuleDefinition;
        friend class Manager;
        friend class ScopedPackageRef;
    };

    /// ScopedPackageRef - Represents a unique reference to a package opened by \c Manager, and
    /// closes it upon destruction.
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

#endif // LANGUAGE_MANAGER_PACKAGEREF_H
