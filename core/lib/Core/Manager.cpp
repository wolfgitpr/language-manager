#include "Manager.h"
#include "Manager_p.h"

#include <set>

#include <stdcorelib/path.h>
#include <stdcorelib/pimpl.h>

#include <LangCore/Core/ManagerLogger.h>
#include <LangCore/Support/Expected.h>
#include <LangCore/Task/G2pTask.h>
#include <LangCore/Task/SplitterTask.h>
#include <LangCore/Task/TaggerTask.h>
#include <LangCore/Task/Task.h>

namespace fs = std::filesystem;

namespace LangCore
{
    Manager::Impl::Impl(Manager *decl) : PackageManager::Impl(decl) {}

    Manager::Impl::~Impl() = default;

    std::vector<NO<Task>> Manager::Impl::priorityTaggers(const std::vector<std::string> &priorityTaggerIds) {
        const auto &taggers = tasks["tagger"];
        std::vector<NO<Task>> result;
        std::unordered_set<std::string> addedIds;

        for (const auto &baseId : priorityTaggerIds) {
            const std::string id = "tagger-" + baseId;
            if (auto it = taggers.find(id); it != taggers.end() && addedIds.find(id) == addedIds.end()) {
                result.push_back(it->second);
                addedIds.insert(id);
            }
        }

        for (const auto &[id, tagger] : taggers) {
            if (addedIds.find(id) == addedIds.end()) {
                result.push_back(tagger);
                addedIds.insert(id);
            }
        }

        return result;
    }

    Manager::Manager() : PackageManager(*new Impl(this)) {}

    Manager::~Manager() = default;

    Manager *Manager::instance() {
        static Manager instance;
        return &instance;
    }

    Expected<bool> Manager::loadTasksForCategory(const std::string &category) {
        __stdc_impl_t;
        auto categoryTasks = this->tasks(category);
        if (!categoryTasks.hasValue()) {
            return Error(
                Error::RuntimeError,
                stdc::formatN("Failed to load %1 tasks: %2. This indicates that either:\n"
                               "  1. No %1 modules were found in the loaded packages\n"
                               "  2. All %1 modules failed to create (check logs above)\n"
                               "  3. Plugin loading path is incorrect",
                               category, categoryTasks.error().message()));
        }
        for (const auto &task : categoryTasks.take())
            impl.tasks[category][task->spec()->id()] = task;
        return true;
    }

    bool Manager::initialize(std::string &errMsg) {
        __stdc_impl_t;
        if (const auto loadPackages = this->loadPackagesInOrder(); !loadPackages) {
            // 获取依赖解析器的错误信息
            const auto &errors = this->getDependencyErrors();
            if (!errors.empty()) {
                errMsg = "Failed to load packages in order due to dependency or Level compatibility issues:\n";
                for (const auto &error : errors) {
                    errMsg += "  - " + error + "\n";
                }
                errMsg += "\nPlease check:\n";
                errMsg += "  1. Plugin dependencies are correctly declared in package.json\n";
                errMsg += "  2. Plugin Level values are within the system's supported range\n";
                errMsg += "  3. Plugin .dll files are present in the plugin directories\n";
                errMsg += "  4. Configuration files exist and are valid JSON\n";
            } else {
                errMsg = "Failed to load packages in order. No specific dependency errors found.\n";
                errMsg += "Please check:\n";
                errMsg += "  1. Plugin .dll files are present in the plugin directories\n";
                errMsg += "  2. package.json files are valid and not corrupted\n";
                errMsg += "  3. Plugin paths are correctly added to the Manager\n";
                errMsg += "  4. Check detailed logs for Level compatibility issues\n";
            }
            return false;
        }

        // 加载各类任务
        const std::vector<std::string> categories = {"splitter", "g2p", "tagger"};
        for (const auto &category : categories) {
            if (auto result = loadTasksForCategory(category); !result) {
                errMsg = result.error().message();
                return false;
            }
        }

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
            return Error(Error::RuntimeError, "could not find category: " + category);

        const auto inferenceObject = inferenceCate->getFirstObject(id);
        if (!inferenceObject)
            return Error(Error::RuntimeError, "could not find id: " + id);

        return inferenceObject.as<Task>();
    }

    Expected<std::vector<NO<Task>>> Manager::tasks(const std::string &category) const {
        const auto inferenceCate = this->category(category);
        if (!inferenceCate)
            return Error(Error::RuntimeError, "could not find category: " + category);

        const auto inferenceObject = inferenceCate->allObjects();
        if (inferenceObject.empty())
            return Error(Error::RuntimeError, "category: " + category + " is empty.");

        std::vector<NO<Task>> tasks;
        tasks.reserve(inferenceObject.size());
        std::transform(inferenceObject.begin(), inferenceObject.end(), std::back_inserter(tasks),
                       [](const auto &obj) { return obj.template as<Task>(); });
        if (tasks.empty())
            return Error(Error::RuntimeError, "category: " + category + " is empty.");
        return tasks;
    }

    std::vector<std::string> Manager::split(const std::string &input) {
        std::vector<std::string> _input;
        _input.push_back(input);
        return this->split(_input);
    }

    std::vector<std::string> Manager::split(const std::vector<std::string> &input) {
        __stdc_impl_t;
        const auto &splitters = impl.tasks["splitter"];
        const auto _input = NO<SplitterInputV1>::create();
        _input->splitterInput = input;

        for (const auto &[splitterId, task] : splitters) {
            auto resExp = task->start(_input);
            _input->splitterInput = resExp.take().as<SplitterResultV1>()->splitterResult;
        }
        return _input->splitterInput;
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
        const auto _input = NO<G2pInputV1>::create();
        std::vector<G2pRes> result;

        for (const auto &[g2pId, lyricVec] : _lyrics) {
            _input->g2pInput = lyricVec;
            const auto targetG2pId = "g2p-" + g2pId;
            if (g2ps.find(targetG2pId) == g2ps.end()) {
                MgrLog.langCoreCritical("Error: fail to find g2p: '%1'", g2pId);
                for (const auto &lyric : lyricVec)
                    result.emplace_back(G2pRes(lyric, g2pId, lyric, {lyric}, "copy", true, G2pNotFound));
                continue;
            }

            auto resultExp = g2ps[targetG2pId]->start(_input);
            if (!resultExp) {
                MgrLog.langCoreCritical("inference failed for g2p '%1': %2", 
                                        g2pId, resultExp.error().message());
                for (const auto &lyric : lyricVec)
                    result.emplace_back(G2pRes(lyric, g2pId, lyric, {lyric}, "copy", true, 
                                              TaskError));
                continue;
            }

            const auto _result = resultExp.take();
            if (const auto g2pRes = _result.as<G2pResultV1>()) {
                result.insert(result.end(), g2pRes->g2pResult.begin(), g2pRes->g2pResult.end());

                if (!g2pRes->errorMessage.empty())
                    MgrLog.langCoreCritical("Error: %1", g2pRes->errorMessage);

            } else {
                MgrLog.langCoreCritical("unexpected result type for g2p '%1'", g2pId);
                for (const auto &lyric : lyricVec)
                    result.emplace_back(G2pRes(lyric, g2pId, lyric, {lyric}, "copy", true, 
                                              InvalidG2pId));
            }
        }

        return result;
    }

    std::vector<TaggerRes> Manager::tag(const std::vector<std::string> &input, const bool split, bool discard,
                                        const std::vector<std::string> &priorityLanguages) {
        __stdc_impl_t;
        std::vector<TaggerRes> inputNote;
        inputNote.reserve(input.size());

        const auto splitRes = split ? this->split(input) : input;

        const auto &taggersList = impl.priorityTaggers(priorityLanguages);
        const auto _input = NO<TaggerInputV1>::create();
        for (const auto &lyric : splitRes)
            inputNote.emplace_back(lyric);

        _input->taggerInput = inputNote;

        for (const auto &task : taggersList) {
            auto resExp = task->start(_input);
            _input->taggerInput = resExp.take().as<TaggerResultV1>()->taggerResult;
        }

        auto res = _input->taggerInput;

        res.erase(
            std::remove_if(res.begin(), res.end(), [discard](const TaggerRes &it) { return discard && it.discard; }),
            res.end());
        return res;
    }
} // namespace LangCore
