#include "ILanguageFactory.h"
#include "ILanguageFactory_p.h"

#include <algorithm>
#include <memory>

namespace LangMgr
{

    ILanguageFactoryPrivate::ILanguageFactoryPrivate() {}

    ILanguageFactoryPrivate::~ILanguageFactoryPrivate() = default;

    void ILanguageFactoryPrivate::init() {}

    ILanguageFactory::ILanguageFactory(const std::string &id) : ILanguageFactory(*new ILanguageFactoryPrivate(), id) {}

    ILanguageFactory::~ILanguageFactory() = default;

    bool ILanguageFactory::initialize(std::string &errMsg) { return true; }

    ILanguageFactory::ILanguageFactory(ILanguageFactoryPrivate &d, const std::string &id) : d_ptr(&d) {
        d.q_ptr = this;
        d.id = id;

        d.init();
    }

    std::string ILanguageFactory::id() const { return d_ptr->id; }

    std::string ILanguageFactory::displayName() const { return d_ptr->displayName; }

    void ILanguageFactory::setDisplayName(const std::string &name) const { d_ptr->displayName = name; }

    bool ILanguageFactory::enabled() const { return d_ptr->enabled; }

    void ILanguageFactory::setEnabled(const bool &enable) const { d_ptr->enabled = enable; }

    bool ILanguageFactory::discardResult() const { return d_ptr->discardResult; }

    void ILanguageFactory::setDiscardResult(const bool &discard) const { d_ptr->discardResult = discard; }

    std::string ILanguageFactory::randString() const { return {}; }

    bool ILanguageFactory::contains(const char32_t &c) const { return false; }

    bool ILanguageFactory::contains(const std::u32string &input) const { return false; }

    std::vector<LangNote> ILanguageFactory::split(const std::u32string &input) const { return {}; }

    std::vector<LangNote> ILanguageFactory::split(const std::vector<LangNote> &input) const {
        if (!d_ptr->enabled)
            return input;

        std::vector<LangNote> result;
        for (const auto &note : input) {
            if (note.g2pId == "unknown" || note.g2pId == "") {
                const auto splitRes = split(note.lyric);
                for (const auto &res : splitRes) {
                    result.push_back(res);
                }
            } else {
                result.push_back(note);
            }
        }
        return result;
    }

    std::string ILanguageFactory::analysis(const std::u32string &input) const {
        if (!d_ptr->enabled)
            return "unknown";
        return contains(input) ? id() : "unknown";
    }

    void ILanguageFactory::correct(const std::vector<LangNote *> &input) const {
        if (!d_ptr->enabled)
            return;

        for (const auto &note : input) {
            if (note->g2pId == "unknown") {
                if (contains(note->lyric)) {
                    note->language = d_ptr->id;
                }
            }
        }
    }

} // namespace LangMgr
