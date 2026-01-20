#ifndef LANGUAGE_MODULE_P_H
#define LANGUAGE_MODULE_P_H

#include <list>
#include <map>
#include <shared_mutex>
#include <unordered_map>

#include <LangMgr/Module/Module.h>
#include <LangMgr/Support/Expected.h>
#include <LangMgr/Task/TaskFactory.h>

#include "Manager_p.h"
#include "ObjectPool_p.h"

namespace LangMgr
{

    class LANGMGR_EXPORT ModuleSpec::Impl {
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

        NO<TaskFactory> interp = nullptr;

        JsonObject manifestConfiguration;
        NO<TaskConfiguration> configuration;

        stdc::VersionNumber fmtVersion;

        State state;
        PackageData *package;
    };

    class LANGMGR_EXPORT ModuleCategory::Impl : public ObjectPool::Impl {
    public:
        explicit Impl(ModuleCategory *decl, std::string name, PackageManager *mgr) :
            ObjectPool::Impl(decl), name(std::move(name)), mgr(mgr) {}
        ~Impl() override;

        std::string name;
        PackageManager *mgr;

        std::list<ModuleSpec *> modules;
        std::map<std::string, NO<TaskFactory>> interpreters;
        std::map<
            std::string,
            std::unordered_map<stdc::VersionNumber, std::map<std::string, std::map<int, decltype(modules)::iterator>>>>
            indexes;

        std::shared_mutex &su_mtx() const { return static_cast<PackageManager::Impl *>(mgr->_impl.get())->su_mtx; }

        std::vector<ModuleSpec *> findModuleSpecs(const ModuleLocator &loc) const;
    };

} // namespace LangMgr

#endif // LANGUAGE_MODULE_P_H
