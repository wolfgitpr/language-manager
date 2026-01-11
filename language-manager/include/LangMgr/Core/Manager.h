#ifndef LANGUAGE_MANAGER_H
#define LANGUAGE_MANAGER_H

#include <filesystem>
#include <string>
#include <vector>

#include <stdcorelib/support/versionnumber.h>

#include <LangMgr/Base/LangCommon.h>
#include <LangMgr/Base/NamedObject.h>
#include <LangMgr/Core/PluginFactory.h>
#include <LangMgr/LangMgrGlobal.h>
#include <LangMgr/Module/Dependency.h>
#include <LangMgr/Module/Module.h>

namespace LangMgr
{

    class Package;
    class Task;
    struct TaggerRes;
    class ModuleCategory;

    template <class T>
    class Expected;

    class LANGMGR_EXPORT Manager : public PluginFactory {
    public:
        Manager();
        ~Manager() override;

        ModuleCategory *category(const std::string_view &name) const;

        static Manager *instance();

        bool initialize(std::string &errMsg);
        bool initialized() const;

    public:
        Expected<NO<Task>> tagger(const std::string &id) const;
        std::vector<NO<Task>> taggers() const;

        std::vector<std::string> defaultOrder() const;
        void setDefaultOrder(const std::vector<std::string> &order);

        std::vector<TaggerRes> split(const std::string &input,
                                     const std::vector<std::string> &priorityTaggerIds = {}) const;
        void convert(const std::vector<TaggerRes *> &input) const;

        std::vector<std::string> tag(const std::vector<std::string> &input,
                                     const std::vector<std::string> &priorityTaggerIds = {},
                                     const std::vector<std::string> &reservedTokens = {}) const;

    public:
        void addPackagePath(const std::filesystem::path &path);
        void addPackagePaths(stdc::array_view<std::filesystem::path> paths);
        void setPackagePaths(stdc::array_view<std::filesystem::path> paths);
        std::vector<std::filesystem::path> packagePaths() const;

        Expected<Package> open(const std::filesystem::path &path, bool noLoad);
        Package find(const std::string_view &id, const stdc::VersionNumber &version) const;
        std::vector<Package> find(const std::string_view &id) const;
        std::vector<Package> packages() const;

        std::vector<ModuleInfo> getModuleInfos();

    protected:
        class Impl;
        static void registerCategoryFactory(ModuleCategory *(*fac)(Manager *));

        void collectModuleInfo(const std::string &packageId, const std::string &packageVersion,
                               const std::filesystem::path &packagePath, const JsonObject &modulesObj);
        static void extractModuleInfoFromJson(const std::string &packageId, const std::string &packageVersion,
                                              const JsonObject &moduleEntry, ModuleInfo &info);


        friend class Package;
        friend class ModuleCategory;

        template <class T>
        friend class ModuleCategoryRegistrar;
    };

    inline void Manager::addPackagePath(const std::filesystem::path &path) { addPackagePaths({path}); }

    template <class T>
    class ModuleCategoryRegistrar {
        static_assert(std::is_base_of_v<ModuleCategory, T>, "T should inherit from LangMgr::ModuleCategory");

    public:
        ModuleCategoryRegistrar(ModuleCategory *(*fac)(Manager *)) { Manager::registerCategoryFactory(fac); }

        ModuleCategoryRegistrar() {
            Manager::registerCategoryFactory([](Manager *mgr) -> ModuleCategory * { return new T(mgr); });
        }
    };

} // namespace LangMgr

#endif // LANGUAGE_MANAGER_H
