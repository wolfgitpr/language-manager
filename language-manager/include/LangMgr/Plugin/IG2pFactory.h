#ifndef IG2PFACTORY_H
#define IG2PFACTORY_H

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <LangMgr/Core/LangCommon.h>
#include <LangMgr/LangMgrGlobal.h>
#include <LangMgr/Plugin/ILanguageFactory.h>

namespace LangMgr
{

    class IG2pFactoryPrivate;

    class LANGMGR_EXPORT IG2pFactory {
    public:
        explicit IG2pFactory(const std::string &id);
        virtual ~IG2pFactory();

        virtual bool initialize(std::string &errMsg);

        virtual std::vector<LangNote> split(const std::u32string &input) const;
        std::vector<LangNote> split(const std::vector<LangNote> &input) const;
        std::string analysis(const std::u32string &input) const;
        void correct(const std::vector<LangNote *> &input) const;

        virtual std::vector<LangNote> convert(const std::vector<std::u32string> &input) const;

        virtual std::pair<std::string, std::string> randString() const;

    public:
        std::string id() const;

        std::string displayName() const;
        void setDisplayName(const std::string &displayName) const;

        std::string author() const;
        void setAuthor(const std::string &author) const;

        std::string description() const;
        void setDescription(const std::string &description) const;

    protected:
        IG2pFactory(IG2pFactoryPrivate &d, const std::string &id);

        std::unique_ptr<IG2pFactoryPrivate> d_ptr;

        std::map<std::string, ILanguageFactory *> m_langFactory;

        friend class LanguageEngine;
    };

} // namespace LangMgr

#endif // IG2PFACTORY_H
