#ifndef LANGUAGE_MANAGER_MODULE_H
#define LANGUAGE_MANAGER_MODULE_H

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <LangMgr/Base/NamedObject.h>
#include <LangMgr/Base/ObjectPool.h>
#include <LangMgr/Support/DisplayText.h>
#include <LangMgr/Support/Expected.h>
#include <LangMgr/Support/JSON.h>
#include <LangMgr/Task/Task.h>
#include <stdcorelib/support/versionnumber.h>

namespace LangMgr
{
    class ModuleLocator {
    public:
        ModuleLocator(std::string package, stdc::VersionNumber version, std::string id) :
            _package(std::move(package)), _version(std::move(version)), _id(std::move(id)) {}
        ModuleLocator(std::string package, stdc::VersionNumber version) :
            _package(std::move(package)), _version(std::move(version)) {}
        ModuleLocator(std::string package, std::string id) : _package(std::move(package)), _id(std::move(id)) {}
        ModuleLocator(std::string id) : _id(std::move(id)) {}

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
    class ModuleCategory;
    class Manager;

    class LANGMGR_EXPORT ModuleDefinition {
    public:
        enum State {
            Invalid,
            Initialized,
            Ready,
            Finished,
            Deleted,
        };

        virtual ~ModuleDefinition();

        const std::string &id() const;
        const std::string &category() const;
        const std::string &className() const;
        DisplayText name() const;
        int apiLevel() const;

        const JsonObject &manifestConfiguration() const;
        NO<TaskConfiguration> configuration() const;
        const std::filesystem::path &path() const;

        Expected<NO<Task>> createTask(const NO<TaskRuntimeOptions> &runtimeOptions) const;

        State state() const;
        Package parent() const;
        Manager *Mgr() const;

        template <class T>
        constexpr T *as();

        template <class T>
        constexpr const T *as() const;

    protected:
        class Impl;
        std::unique_ptr<Impl> _impl;
        explicit ModuleDefinition(Impl &impl);
        explicit ModuleDefinition(std::string category);

        friend class ModuleCategory;
        friend class Manager;
    };

    template <class T>
    constexpr T *ModuleDefinition::as() {
        static_assert(std::is_base_of_v<ModuleDefinition, T>, "T must inherit from LangMgr::ModuleDefinition");
        return static_cast<T *>(this);
    }

    template <class T>
    constexpr const T *ModuleDefinition::as() const {
        static_assert(std::is_base_of_v<ModuleDefinition, T>, "T must inherit from LangMgr::ModuleDefinition");
        return static_cast<const T *>(this);
    }

    class LANGMGR_EXPORT ModuleCategory : public ObjectPool {
    public:
        ~ModuleCategory() override;

        const std::string &name() const;
        Manager *Mgr() const;

        std::vector<ModuleDefinition *> findDefinitions(const ModuleLocator &identifier) const;
        std::vector<ModuleDefinition *> definitions() const;

        template <class T>
        constexpr T *as();

        template <class T>
        constexpr const T *as() const;

    protected:
        virtual std::string key() const = 0;
        virtual std::string category() const = 0;

        Expected<ModuleDefinition *> parseDefinition(const std::filesystem::path &basePath,
                                                     const JsonValue &config) const;
        Expected<void> loadDefinitionBase(ModuleDefinition *definition, const ModuleDefinition::State state);
        Expected<void> loadDefinition(ModuleDefinition *spec, ModuleDefinition::State state);

        std::vector<ModuleDefinition *> find(const ModuleLocator &loc) const;

        class Impl;
        explicit ModuleCategory(Impl &impl);
        ModuleCategory(std::string name, Manager *mgr);

        friend class Manager;
        friend class Package;
        friend class PackageData;
    };

    template <class T>
    constexpr T *ModuleCategory::as() {
        static_assert(std::is_base_of_v<ModuleCategory, T>, "T must inherit from LangMgr::ModuleCategory");
        return static_cast<T *>(this);
    }

    template <class T>
    constexpr const T *ModuleCategory::as() const {
        static_assert(std::is_base_of_v<ModuleCategory, T>, "T must inherit from LangMgr::ModuleCategory");
        return static_cast<const T *>(this);
    }
} // namespace LangMgr
#endif
