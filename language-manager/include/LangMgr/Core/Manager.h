#ifndef LANGUAGE_MANAGER_H
#define LANGUAGE_MANAGER_H

#include <filesystem>
#include <string>
#include <vector>

#include <stdcorelib/support/versionnumber.h>

#include <LangMgr/Core/LangCommon.h>
#include <LangMgr/LangMgrGlobal.h>
#include <LangMgr/Plugin/PluginFactory.h>
#include <LangMgr/Support/Expected.h>
#include <LangMgr/Task/Task.h>

#include "NamedObject.h"

namespace LangMgr
{
    class Package;
    class ModuleCategory;

    template <class T>
    class ModuleCategoryRegistrar;

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

    protected:
        class Impl;
        static void registerCategoryFactory(ModuleCategory *(*fac)(Manager *));

        friend class Package;
        friend class ModuleCategory;
        template <class T>
        friend class ModuleCategoryRegistrar;
    };

    inline void Manager::addPackagePath(const std::filesystem::path &path) { addPackagePaths({path}); }

} // namespace LangMgr

#endif // LANGUAGE_MANAGER_H
