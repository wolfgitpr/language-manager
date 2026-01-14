#include "Manager.h"

#include <fstream>

#include "Manager_p.h"

#include <iostream>
#include <mutex>
#include <set>

#include <stdcorelib/path.h>
#include <stdcorelib/pimpl.h>
#include <stdcorelib/stlextra/algorithms.h>

#include <LangMgr/Module/Dependency/DependencyResolver.h>
#include <LangMgr/Module/Dependency/VersionUtils.h>

#include <LangMgr/Support/Expected.h>
#include <LangMgr/Support/JSON.h>
#include <LangMgr/Task/Task.h>

#include "Module_p.h"
#include "Package_p.h"

namespace fs = std::filesystem;

namespace LangMgr
{
    Manager::Impl::Impl(Manager *decl) : PackageManager::Impl(decl) {}

    Manager::Impl::~Impl() {}

    std::vector<NO<Task>> Manager::Impl::priorityTaggers(const std::vector<std::string> &priorityTaggerIds) const {
        const std::vector<std::string> order = defaultTaggerOrder;

        std::vector<NO<Task>> result;
        for (const auto &g2pId : priorityTaggerIds) {
            const auto it = taggers.find(g2pId);
            if (it == taggers.end())
                continue;
            result.push_back(it->second);
        }

        for (const auto &id : order) {
            if (std::find(priorityTaggerIds.begin(), priorityTaggerIds.end(), id) != priorityTaggerIds.end())
                continue;

            const auto it = taggers.find(id);
            if (it == taggers.end())
                continue;
            result.push_back(it->second);
        }
        return result;
    }

    Manager::Manager() : PackageManager(*new Impl(this)) {}

    Manager::~Manager() = default;

    Manager *Manager::instance() { return static_cast<Manager *>(PackageManager::instance()); }

    bool Manager::initialize(std::string &errMsg) {
        __stdc_impl_t;
        impl.initialized = true;
        return true;
    }

    bool Manager::initialized() const {
        __stdc_impl_t;
        return impl.initialized;
    }

    Expected<NO<Task>> Manager::tagger(const std::string &id) const {
        __stdc_impl_t;
        const auto it = impl.taggers.find(id);
        if (it == impl.taggers.end()) {
            std::cerr << "LangMgr::Manager::tagger(): factory does not exist:" << id << std::endl;
            return Expected<NO<Task>>();
        }
        return it->second;
    }

    std::vector<NO<Task>> Manager::taggers() const {
        __stdc_impl_t;
        std::vector<NO<Task>> result;
        for (auto [id, tagger] : impl.taggers)
            result.push_back(tagger);
        return result;
    }

    std::vector<std::string> Manager::defaultOrder() const {
        __stdc_impl_t;
        return impl.defaultTaggerOrder;
    }

    void Manager::setDefaultOrder(const std::vector<std::string> &order) {
        __stdc_impl_t;
        impl.defaultTaggerOrder = order;
    }

    std::vector<TaggerRes> Manager::split(const std::string &input,
                                          const std::vector<std::string> &priorityTaggerIds) const {
        __stdc_impl_t;
        // const auto &taggersList = impl.priorityTaggers(priorityTaggerIds);
        // std::vector result = {TaggerRes(utf8strToU32str(input))};
        // for (const auto &tagger : taggersList)
        //     result = tagger->start(result);
        // return result;
        return {};
    }

    void Manager::convert(const std::vector<TaggerRes *> &input) const {
        // __stdc_impl_t;
        // std::map<std::string, std::vector<int>> indexMap;
        // std::map<std::string, std::vector<std::u32string>> lyricMap;
        //
        // for (int i = 0; i < input.size(); ++i) {
        //     const TaggerRes *note = input.at(i);
        //     indexMap[note->g2pId].push_back(i);
        //     lyricMap[note->g2pId].push_back(note->lyric);
        // }
        //
        // for (const auto &[taggerId, indices] : indexMap) {
        //     const auto &rawLyrics = lyricMap[taggerId];
        //     auto [taggerType, configId] = impl.extractConfig(taggerId);
        //
        //     auto g2pFactory = this->tagger(taggerId);
        //     if (!g2pFactory)
        //         g2pFactory = this->tagger("unknown");
        //
        //     const auto &tempRes = g2pFactory->convert(rawLyrics);
        //     for (int i = 0; i < tempRes.size(); i++) {
        //         const auto &index = indices[i];
        //         input[index]->error = tempRes[i].error;
        //         input[index]->syllable = tempRes[i].syllable;
        //         input[index]->candidates = tempRes[i].candidates;
        //     }
        // }
    }

    std::vector<std::string> Manager::tag(const std::vector<std::string> &input,
                                          const std::vector<std::string> &priorityTaggerIds,
                                          const std::vector<std::string> &reservedTokens) const {
        // __stdc_impl_t;
        // const auto &taggersList = impl.priorityTaggers(priorityTaggerIds);
        // std::vector<TaggerRes *> inputNote;
        // for (const auto &lyric : input) {
        //     inputNote.push_back(new TaggerRes(utf8strToU32str(lyric)));
        // }
        //
        // for (const auto &tagger : taggersList)
        //     tagger->correct(inputNote);
        //
        // std::vector<std::string> result;
        // for (const auto &note : inputNote)
        //     result.push_back(note->language);
        //
        // for (const auto note : inputNote) {
        //     delete note;
        // }
        //
        // return result;
        return {};
    }
} // namespace LangMgr
