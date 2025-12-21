#ifndef LANGUAGE_MANAGER_CONTRIBUTE_P_H
#define LANGUAGE_MANAGER_CONTRIBUTE_P_H

#include <list>
#include <map>
#include <shared_mutex>
#include <unordered_map>

#include "Contribute.h"
#include "Expected.h"

#include "LanguageEngine_p.h"
#include "NamedObject_p.h"

namespace LangMgr
{

    class PackageData;

    class ContribSpec::Impl {
    public:
        explicit Impl(std::string category) : category(std::move(category)), state(Invalid) {}
        virtual ~Impl() = default;

        virtual Expected<void> read(const std::filesystem::path &basePath, const JsonObject &obj) {
            return Error(Error::NotImplemented);
        }

        std::string category;
        std::string id;
        stdc::VersionNumber fmtVersion;

        State state;
        PackageData *package;
    };

    class ContribCategory::Impl : public ObjectPool::Impl {
    public:
        explicit Impl(ContribCategory *decl, std::string name, LanguageManager *su) :
            ObjectPool::Impl(decl), name(std::move(name)), su(su) {}
        ~Impl() override;

        std::string name;
        LanguageManager *su;

        std::list<ContribSpec *> contributes;
        std::map<std::string,
                 std::unordered_map<stdc::VersionNumber, std::map<std::string, decltype(contributes)::iterator>>>
            indexes;

        std::shared_mutex &su_mtx() const { return static_cast<LanguageManager::Impl *>(su->_impl.get())->su_mtx; }

        std::vector<ContribSpec *> findContributes(const ContribLocator &loc) const;
    };

} // namespace LangMgr

#endif // LANGUAGE_MANAGER_CONTRIBUTE_P_H
