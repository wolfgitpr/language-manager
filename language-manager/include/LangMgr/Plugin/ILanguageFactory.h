#ifndef ILANGUAGEFACTORY_H
#define ILANGUAGEFACTORY_H

#include <memory>
#include <string>
#include <vector>

#include <LangMgr/Core/LangCommon.h>
#include <LangMgr/LangMgrGlobal.h>

namespace LangMgr
{

    class ILanguageFactoryPrivate;

    class LANGMGR_EXPORT ILanguageFactory {
    public:
        explicit ILanguageFactory(const std::string &id);
        virtual ~ILanguageFactory();

        virtual bool initialize(std::string &errMsg);

        virtual bool contains(const char32_t &c) const;
        virtual bool contains(const std::u32string &input) const;

        virtual std::string randString() const;

        virtual std::vector<LangNote> split(const std::u32string &input) const;
        std::vector<LangNote> split(const std::vector<LangNote> &input) const;
        std::string analysis(const std::u32string &input) const;
        void correct(const std::vector<LangNote *> &input) const;

    public:
        std::string id() const;

        std::string displayName() const;
        void setDisplayName(const std::string &name) const;

        bool enabled() const;
        void setEnabled(const bool &enable) const;

        bool discardResult() const;
        void setDiscardResult(const bool &discard) const;

    protected:
        ILanguageFactory(ILanguageFactoryPrivate &d, const std::string &id);

        std::unique_ptr<ILanguageFactoryPrivate> d_ptr;

        friend class LanguageEngine;

    }; // ILanguageFactory

} // namespace LangMgr

#endif // ILANGUAGEFACTORY_H
