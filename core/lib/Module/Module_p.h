#ifndef LANGCORE_MODULE_P_H
#define LANGCORE_MODULE_P_H

#include <list>
#include <map>
#include <shared_mutex>
#include <unordered_map>

#include <LangCore/Module/Module.h>
#include <LangCore/Support/Expected.h>
#include <LangCore/Task/TaskFactory.h>

#include "ObjectPool_p.h"
#include "PackageManager_p.h"

namespace LangCore
{

    class LANGCORE_EXPORT ModuleSpec::Impl {
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

    class LANGCORE_EXPORT ModuleCategory::Impl : public ObjectPool::Impl {
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

        std::shared_mutex &su_mtx() const { return mgr->_impl.get()->su_mtx; }

        std::vector<ModuleSpec *> findModuleSpecs(const ModuleLocator &loc) const;
    };

} // namespace LangCore

#endif // LANGCORE_MODULE_P_H
