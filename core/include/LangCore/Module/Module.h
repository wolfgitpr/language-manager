#ifndef LANGCORE_MODULE_H
#define LANGCORE_MODULE_H

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <LangCore/Base/NamedObject.h>
#include <LangCore/Base/ObjectPool.h>
#include <LangCore/Support/DisplayText.h>
#include <LangCore/Support/Expected.h>
#include <LangCore/Support/JSON.h>
#include <LangCore/Task/Task.h>
#include <stdcorelib/support/versionnumber.h>

namespace LangCore
{
    class ModuleLocator {
    public:
        ModuleLocator(std::string package, const stdc::VersionNumber version, std::string id) :
            _package(std::move(package)), _version(version), _id(std::move(id)) {}
        ModuleLocator(std::string package, const stdc::VersionNumber version) :
            _package(std::move(package)), _version(version) {}
        ModuleLocator(std::string package, std::string id) : _package(std::move(package)), _id(std::move(id)) {}
        explicit ModuleLocator(std::string id) : _id(std::move(id)) {}

        ModuleLocator() = default;

        const std::string &package() const { return _package; }
        stdc::VersionNumber version() const { return _version; }
        const std::string &id() const { return _id; }
        bool isEmpty() const { return _id.empty(); }

        std::string toString() const;
        static ModuleLocator fromString(const std::string_view &token);
        static bool isValidLocator(const std::string_view &token);

        bool operator==(const ModuleLocator &other) const {
            return _package == other._package && _version == other._version && _id == other._id;
        }

        bool operator!=(const ModuleLocator &other) const { return !(*this == other); }

    private:
        std::string _package;
        stdc::VersionNumber _version;
        std::string _id;
    };

    class PackageData;
    class Package;
    class PackageManager;

    class LANGCORE_EXPORT ModuleSpec {
    public:
        enum State {
            Invalid,
            Initialized,
            Ready,
            Finished,
            Deleted,
        };

        virtual ~ModuleSpec();

        const std::string &id() const;
        const std::string &category() const;
        const std::string &className() const;
        DisplayText name() const;
        int apiLevel() const;

        const JsonObject &manifestConfiguration() const;
        NO<TaskConfiguration> configuration() const;
        const std::filesystem::path &path() const;

        State state() const;
        Package parent() const;
        PackageManager *Mgr() const;

        template <class T>
        constexpr T *as();

        template <class T>
        constexpr const T *as() const;

    protected:
        class Impl;
        std::unique_ptr<Impl> _impl;
        explicit ModuleSpec(Impl &impl);
        explicit ModuleSpec(std::string category);

        friend class ModuleCategory;
        friend class PackageManager;
    };

    template <class T>
    constexpr T *ModuleSpec::as() {
        static_assert(std::is_base_of_v<ModuleSpec, T>, "T must inherit from LangPlugins::ModuleSpec");
        return static_cast<T *>(this);
    }

    template <class T>
    constexpr const T *ModuleSpec::as() const {
        static_assert(std::is_base_of_v<ModuleSpec, T>, "T must inherit from LangPlugins::ModuleSpec");
        return static_cast<const T *>(this);
    }

    class LANGCORE_EXPORT ModuleCategory : public ObjectPool {
    public:
        ~ModuleCategory() override;

        const std::string &name() const;
        PackageManager *Mgr() const;

        std::vector<ModuleSpec *> findSpec(const ModuleLocator &identifier) const;
        std::vector<ModuleSpec *> specs() const;

        template <class T>
        constexpr T *as();

        template <class T>
        constexpr const T *as() const;

    protected:
        virtual std::string key() const = 0;
        virtual std::string category() const = 0;

        Expected<ModuleSpec *> parseSpec(const std::filesystem::path &basePath, const JsonValue &config) const;
        Expected<void> loadSpecBase(ModuleSpec *spec, ModuleSpec::State state);
        Expected<void> loadSpec(ModuleSpec *spec, ModuleSpec::State state);

        std::vector<ModuleSpec *> find(const ModuleLocator &loc) const;

        class Impl;
        explicit ModuleCategory(Impl &impl);
        ModuleCategory(std::string name, PackageManager *mgr);

        friend class PackageManager;
        friend class Package;
        friend class PackageData;
    };

    template <class T>
    constexpr T *ModuleCategory::as() {
        static_assert(std::is_base_of_v<ModuleCategory, T>, "T must inherit from LangPlugins::ModuleCategory");
        return static_cast<T *>(this);
    }

    template <class T>
    constexpr const T *ModuleCategory::as() const {
        static_assert(std::is_base_of_v<ModuleCategory, T>, "T must inherit from LangPlugins::ModuleCategory");
        return static_cast<const T *>(this);
    }
} // namespace LangCore
#endif // LANGCORE_MODULE_H
