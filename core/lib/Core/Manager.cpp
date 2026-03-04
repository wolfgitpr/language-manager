#include "Manager.h"
#include "Manager_p.h"

#include <set>

#include <stdcorelib/path.h>
#include <stdcorelib/pimpl.h>

#include <LangCore/Core/ManagerLogger.h>
#include <LangCore/Support/Expected.h>
#include <LangCore/Task/G2pTask.h>
#include <LangCore/Task/TaggerTask.h>
#include <LangCore/Task/Task.h>

namespace fs = std::filesystem;

namespace LangCore
{
    Manager::Impl::Impl(Manager *decl) : PackageManager::Impl(decl) {}

    Manager::Impl::~Impl() {}

    std::vector<NO<Task>> Manager::Impl::priorityTaggers(const std::vector<std::string> &priorityTaggerIds) {
        const std::vector<std::string> order = defaultTaggerOrder;

        const auto &taggers = tasks["tagger"];

        std::vector<NO<Task>> result;
        for (const auto &taggerId : priorityTaggerIds) {
            const auto it = taggers.find(taggerId);
            if (it == taggers.end())
                continue;
            result.push_back(it->second);
        }

        for (const auto &baseId : order) {
            const auto id = "tagger-" + baseId;
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

    Manager *Manager::instance() {
        static Manager instance;
        return &instance;
    }

    bool Manager::initialize(std::string &errMsg) {
        __stdc_impl_t;
        if (const auto loadPackages = this->loadPackagesInOrder(); !loadPackages) {
            errMsg = "Failed to load packages in order";
            return false;
        }

        const auto g2ps = this->tasks("g2p").take();
        for (const auto &g2p : g2ps)
            impl.tasks["g2p"][g2p->spec()->name().text()] = g2p;

        const auto taggers = this->tasks("tagger").take();
        for (const auto &tagger : taggers)
            impl.tasks["tagger"][tagger->spec()->name().text()] = tagger;

        impl.initialized = true;
        return true;
    }

    bool Manager::initialized() const {
        __stdc_impl_t;
        return impl.initialized;
    }

    Expected<NO<Task>> Manager::task(const std::string &category, const std::string &id) const {
        const auto inferenceCate = this->category(category);
        if (!inferenceCate)
            return Error(Error::SessionError, "could not find category: " + category);

        const auto inferenceObject = inferenceCate->getFirstObject(id);
        if (!inferenceObject)
            return Error(Error::SessionError, "could not find id: " + id);

        return inferenceObject.as<Task>();
    }

    Expected<std::vector<NO<Task>>> Manager::tasks(const std::string &category) const {
        const auto inferenceCate = this->category(category);
        if (!inferenceCate)
            return Error(Error::SessionError, "could not find category: " + category);

        const auto inferenceObject = inferenceCate->allObjects();
        if (inferenceObject.empty())
            return Error(Error::SessionError, "category: " + category + " is empty.");

        std::vector<NO<Task>> tasks;
        tasks.reserve(inferenceObject.size());
        std::transform(inferenceObject.begin(), inferenceObject.end(), std::back_inserter(tasks),
                       [](const auto &obj) { return obj.template as<Task>(); });
        return tasks;
    }

    std::vector<std::string> Manager::defaultTaggerOrder() const {
        __stdc_impl_t;
        return impl.defaultTaggerOrder;
    }

    void Manager::setDefaultOrder(const std::vector<std::string> &order) {
        __stdc_impl_t;
        impl.defaultTaggerOrder = order;
    }

    std::vector<std::string> Manager::split(const std::string &input,
                                            const std::vector<std::string> &priorityLanguages) {
        const auto result = this->tag({input}, true, priorityLanguages);
        std::vector<std::string> lyrics;
        for (const auto &elem : result)
            lyrics.push_back(elem.lyric);
        return lyrics;
    }

    static std::vector<std::pair<std::string, std::vector<std::string>>>
    groupLyrics(const std::vector<G2pInput *> &input) {
        std::vector<std::pair<std::string, std::vector<std::string>>> groups;
        std::string lastId;

        for (const auto *item : input) {
            if (groups.empty() || item->g2pId != lastId) {
                groups.emplace_back();
                lastId = item->g2pId;
                groups.back().first = lastId;
            }
            groups.back().second.push_back(item->lyric);
        }

        return groups;
    }

    std::vector<G2pRes> Manager::convert(const std::vector<G2pInput *> &input) {
        __stdc_impl_t;
        auto &g2ps = impl.tasks["g2p"];
        const auto _lyrics = groupLyrics(input);
        const auto _input = NO<G2pStartInput>::create(G2P_API_NAME, G2P_API_CLASS, G2P_API_LEVEL);
        std::vector<G2pRes> result;

        for (const auto &[g2pId, lyric] : _lyrics) {
            _input->g2pInput = lyric;
            const auto targetG2pId = "g2p-" + g2pId;
            if (g2ps.find(targetG2pId) == g2ps.end()) {
                MgrLog.langCoreCritical("Error: fail to find g2p: %1", g2pId);
                continue;
            }

            auto resultExp = g2ps[targetG2pId]->start(_input);
            if (!resultExp)
                throw std::runtime_error(stdc::formatN("inference failed: %1", resultExp.error().message()));

            const auto _result = resultExp.take();
            if (const auto g2pRes = _result.as<G2pOutput>()) {
                result.insert(result.end(), g2pRes->g2pResult.begin(), g2pRes->g2pResult.end());

                if (!g2pRes->errorMessage.empty())
                    MgrLog.langCoreCritical("Error: %1", g2pRes->errorMessage);

            } else {
                throw std::runtime_error("unexpected result type");
            }
        }

        return result;
    }

    std::vector<TaggerRes> Manager::tag(const std::vector<std::string> &input, const bool split,
                                        const std::vector<std::string> &priorityLanguages) {
        __stdc_impl_t;
        std::vector<TaggerRes> inputNote;
        inputNote.reserve(input.size());

        const auto &taggersList = impl.priorityTaggers(priorityLanguages);
        const auto _input = NO<TaggerStartInput>::create(TAGGER_API_NAME, TAGGER_API_CLASS, TAGGER_API_LEVEL);
        _input->split = split;

        for (const auto &lyric : input)
            inputNote.emplace_back(lyric);

        _input->taggerInput = inputNote;

        for (const auto &task : taggersList) {
            auto resExp = task->start(_input);
            _input->taggerInput = resExp.take().as<TaggerOutput>()->taggerResult;
        }
        return _input->taggerInput;
    }
} // namespace LangCore
