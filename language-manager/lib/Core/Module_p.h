#ifndef LANGUAGE_MANAGER_CONTRIBUTE_P_H
#define LANGUAGE_MANAGER_CONTRIBUTE_P_H

#include <list>
#include <map>
#include <shared_mutex>
#include <unordered_map>

#include "Expected.h"
#include "Module.h"

#include "EngineFactory.h"

#include "Manager_p.h"
#include "ObjectPool_p.h"

namespace LangMgr
{

    class PackageData;

    class ModuleDefinition::Impl {
    public:
        explicit Impl(std::string category) : category(std::move(category)), state(Invalid), package(nullptr) {}
        virtual ~Impl() = default;

        Expected<void> read(const std::filesystem::path &basePath, const JsonObject &obj);

        std::string id;

        std::string category;

        std::filesystem::path path;

        std::string className;

        DisplayText name;
        int apiLevel = 0;

        NO<EngineFactory> interp = nullptr;

        JsonObject manifestConfiguration;
        NO<TaskConfiguration> configuration;

        stdc::VersionNumber fmtVersion;

        State state;
        PackageData *package;
    };

    class ModuleCategory::Impl : public ObjectPool::Impl {
    public:
        explicit Impl(ModuleCategory *decl, std::string name, Manager *mgr) :
            ObjectPool::Impl(decl), name(std::move(name)), mgr(mgr) {}
        ~Impl() override;

        std::string name;
        Manager *mgr;

        std::list<ModuleDefinition *> modules;
        std::map<std::string, NO<EngineFactory>> interpreters;
        std::map<std::string,
                 std::unordered_map<stdc::VersionNumber, std::map<std::string, decltype(modules)::iterator>>>
            indexes;

        std::shared_mutex &su_mtx() const { return static_cast<Manager::Impl *>(mgr->_impl.get())->su_mtx; }

        std::vector<ModuleDefinition *> findModuleSpecs(const ModuleLocator &loc) const;
    };

} // namespace LangMgr

#endif // LANGUAGE_MANAGER_CONTRIBUTE_P_H
