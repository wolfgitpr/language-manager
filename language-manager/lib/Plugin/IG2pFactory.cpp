#include "IG2pFactory.h"
#include "IG2pFactory_p.h"

#include <ctime>
#include <random>

namespace LangMgr
{

    IG2pFactoryPrivate::IG2pFactoryPrivate() {}

    IG2pFactoryPrivate::~IG2pFactoryPrivate() = default;

    void IG2pFactoryPrivate::init() {}

    IG2pFactory::IG2pFactory(const std::string &id) : IG2pFactory(*new IG2pFactoryPrivate(), id) {}

    IG2pFactory::~IG2pFactory() = default;

    IG2pFactory::IG2pFactory(IG2pFactoryPrivate &d, const std::string &id) : d_ptr(&d) {
        d.q_ptr = this;
        d.id = id;

        d.init();
    }

    bool IG2pFactory::initialize(std::string &errMsg) { return true; }

    std::string IG2pFactory::id() const { return d_ptr->id; }

    std::string IG2pFactory::displayName() const { return d_ptr->displayName; }

    void IG2pFactory::setDisplayName(const std::string &displayName) const { d_ptr->displayName = displayName; }

    std::string IG2pFactory::author() const { return d_ptr->author; }

    void IG2pFactory::setAuthor(const std::string &author) const { d_ptr->author = author; }

    std::string IG2pFactory::description() const { return d_ptr->description; }

    void IG2pFactory::setDescription(const std::string &description) const { d_ptr->description = description; }

    std::pair<std::string, std::string> IG2pFactory::randString() const {
        static std::random_device rd;
        static std::mt19937 gen(rd());

        if (m_langFactory.empty()) {
            return {"", ""};
        }

        std::uniform_int_distribution<> dis(0, m_langFactory.size() - 1);
        const int randomIndex = dis(gen);

        auto it = m_langFactory.begin();
        std::advance(it, randomIndex);

        return {it->second->randString(), it->second->id()};
    }

    std::string IG2pFactory::analysis(const std::u32string &input) const {
        for (const auto &[id, factory] : m_langFactory) {
            if (const auto result = factory->analysis(input); result != "unknown")
                return result;
        }
        return "unknown";
    }

    std::vector<LangNote> IG2pFactory::split(const std::u32string &input) const { return split({LangNote(input)}); }

    std::vector<LangNote> IG2pFactory::split(const std::vector<LangNote> &input) const {
        std::vector<LangNote> result = input;
        for (const auto &[id, factory] : m_langFactory) {
            result = factory->split(result);
        }
        return result;
    }

    void IG2pFactory::correct(const std::vector<LangNote *> &input) const {
        for (const auto &[id, factory] : m_langFactory) {
            factory->correct(input);
        }
    }

    std::vector<LangNote> IG2pFactory::convert(const std::vector<std::u32string> &input) const {
        std::vector<LangNote> result;
        for (const auto &i : input) {
            LangNote langNote;
            langNote.lyric = i;
            langNote.syllable = i;
            langNote.candidates = {i};
            result.push_back(langNote);
        }
        return result;
    }
} // namespace LangMgr
