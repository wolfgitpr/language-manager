#ifndef LANGUAGE_MANAGER_H
#define LANGUAGE_MANAGER_H

#include <filesystem>
#include <string>
#include <vector>

#include <stdcorelib/support/versionnumber.h>

#include <LangMgr/LangMgrGlobal.h>
#include <LangMgr/Plugin/IG2pFactory.h>
#include <LangMgr/Plugin/PluginFactory.h>
#include <LangMgr/Support/Expected.h>

namespace LangMgr
{

    class PackageRef;
    class ContribCategory;

    template <class T>
    class ContribCategoryRegistrar;

    class LANGMGR_EXPORT LanguageManager : public PluginFactory {
    public:
        LanguageManager();
        ~LanguageManager() override;

        ContribCategory *category(const std::string_view &name) const;

        static LanguageManager *instance();

        bool initialize(std::string &errMsg);
        bool initialized() const;

    public:
        IG2pFactory *g2p(const std::string &id) const;
        std::vector<IG2pFactory *> g2ps() const;

        bool addG2p(IG2pFactory *factory);
        bool removeG2p(const IG2pFactory *factory);
        bool removeG2p(const std::string &id);
        void clearG2ps();

        std::vector<std::string> defaultOrder() const;
        void setDefaultOrder(const std::vector<std::string> &order);

        std::vector<LangNote> split(const std::string &input,
                                    const std::vector<std::string> &priorityG2pIds = {}) const;
        void correct(const std::vector<LangNote *> &input, const std::vector<std::string> &priorityG2pIds = {},
                     const std::vector<std::string> &reservedTokens = {}) const;
        void convert(const std::vector<LangNote *> &input) const;

        std::string analysis(const std::string &input, const std::vector<std::string> &priorityG2pIds = {},
                             const std::vector<std::string> &reservedTokens = {}) const;
        std::vector<std::string> analysis(const std::vector<std::string> &input,
                                          const std::vector<std::string> &priorityG2pIds = {},
                                          const std::vector<std::string> &reservedTokens = {}) const;

    public:
        void addPackagePath(const std::filesystem::path &path);
        void addPackagePaths(stdc::array_view<std::filesystem::path> paths);
        void setPackagePaths(stdc::array_view<std::filesystem::path> paths);
        std::vector<std::filesystem::path> packagePaths() const;

        Expected<PackageRef> open(const std::filesystem::path &path, bool noLoad);
        PackageRef find(const std::string_view &id, const stdc::VersionNumber &version) const;
        std::vector<PackageRef> find(const std::string_view &id) const;
        std::vector<PackageRef> packages() const;

    protected:
        class Impl;
        static void registerCategoryFactory(ContribCategory *(*fac)(LanguageManager *));

        friend class PackageRef;
        friend class ContribCategory;
        template <class T>
        friend class ContribCategoryRegistrar;
    };

    inline void LanguageManager::addPackagePath(const std::filesystem::path &path) { addPackagePaths({path}); }

} // namespace LangMgr

#endif // LANGUAGE_MANAGER_H
