#include "Manager.h"
#include "Manager_p.h"

#include <set>

#include <stdcorelib/path.h>
#include <stdcorelib/pimpl.h>

#include <LangCore/Core/ManagerLogger.h>
#include <LangCore/Support/Expected.h>
#include <LangCore/Task/G2pTask.h>
#include <LangCore/Task/Task.h>

namespace fs = std::filesystem;

namespace LangCore
{
    Manager::Impl::Impl(Manager *decl) : PackageManager::Impl(decl) {}

    Manager::Impl::~Impl() = default;

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
        // g2p 是必需的；dict 是可选的（可能没有词典插件）
        if (auto result = loadTasksForCategory("g2p"); !result) {
            errMsg = result.error().message();
            return false;
        }

        // dict 加载失败不阻止初始化
        if (auto result = loadTasksForCategory("dict"); !result) {
            MgrLog.langCoreInfo("No dict tasks loaded (this is normal if no dict plugins are installed)");
        }

        impl.initialized = true;
        return true;
    }

    bool Manager::initialized() const {
        __stdc_impl_t;
        return impl.initialized;
    }

    Expected<NO<Task>> Manager::task(const std::string &category, const std::string &id) const {
        if (category.empty())
            return Error(Error::RuntimeError, "category cannot be empty",
                         "Please provide a valid category name (e.g., 'g2p')");

        if (id.empty())
            return Error(Error::RuntimeError, "id cannot be empty",
                         "Please provide a valid task id (e.g., 'g2p-cmn-official')");

        const auto inferenceCate = this->category(category);
        if (!inferenceCate)
            return Error(Error::RuntimeError, "could not find category: " + category,
                         "Available categories: g2p, driver, dict");

        const auto inferenceObject = inferenceCate->getFirstObject(id);
        if (!inferenceObject)
            return Error(Error::RuntimeError, "could not find id: " + id,
                         "Please check the available tasks using tasks() method");

        return inferenceObject.as<Task>();
    }

    Expected<std::vector<NO<Task>>> Manager::tasks(const std::string &category) const {
        if (category.empty())
            return Error(Error::RuntimeError, "category cannot be empty",
                         "Please provide a valid category name (e.g., 'g2p')");

        const auto inferenceCate = this->category(category);
        if (!inferenceCate)
            return Error(Error::RuntimeError, "could not find category: " + category,
                         "Available categories: g2p, driver, dict");

        const auto inferenceObject = inferenceCate->allObjects();
        if (inferenceObject.empty())
            return Error(Error::RuntimeError, "category: " + category + " is empty.",
                         "No tasks available in this category");

        std::vector<NO<Task>> tasks;
        tasks.reserve(inferenceObject.size());
        std::transform(inferenceObject.begin(), inferenceObject.end(), std::back_inserter(tasks),
                       [](const auto &obj) { return obj.template as<Task>(); });
        if (tasks.empty())
            return Error(Error::RuntimeError, "category: " + category + " is empty.",
                         "No tasks available in this category");
        return tasks;
    }

    /// 过滤空指针，返回有效的输入指针
    /// @param input 输入指针列表
    /// @param logWarnings 是否记录警告日志
    /// @return 过滤后的有效指针列表
    static std::vector<G2pInput *>
    filterNullPointers(const std::vector<G2pInput *> &input, bool logWarnings = true) {
        std::vector<G2pInput *> validInput;
        validInput.reserve(input.size());

        for (auto *item : input) {
            if (!item) {
                if (logWarnings) {
                    MgrLog.langCoreWarning("convert() received null pointer in input, skipping");
                }
                continue;
            }
            validInput.push_back(item);
        }

        return validInput;
    }

    static std::vector<std::pair<std::string, std::vector<std::string>>>
    groupLyrics(const std::vector<G2pInput *> &input) {
        std::vector<std::pair<std::string, std::vector<std::string>>> groups;
        std::string lastId;

        for (const auto *item : input) {
            // 跳过空指针
            if (!item) {
                continue;
            }

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
        if (input.empty())
            return {};

        // 验证输入指针，过滤掉空指针
        const auto validInput = filterNullPointers(input, true);

        // 如果所有指针都是空的，返回空结果
        if (validInput.empty())
            return {};

        __stdc_impl_t;
        auto &g2ps = impl.tasks["g2p"];
        const auto _lyrics = groupLyrics(validInput);
        const auto _input = NO<G2pInputV1>::create();
        std::vector<G2pRes> result;

        for (const auto &[g2pId, lyricVec] : _lyrics) {
            _input->g2pInput = lyricVec;
            auto g2pIt = g2ps.find(g2pId);
            if (g2pIt == g2ps.end()) {
                MgrLog.langCoreCritical("Error: fail to find g2p: '%1'", g2pId);
                for (const auto &lyric : lyricVec)
                    result.emplace_back(G2pRes(lyric, g2pId, lyric, {lyric}, "copy", UnknownError));
                continue;
            }

            auto resultExp = g2pIt->second->start(_input);
            if (!resultExp) {
                MgrLog.langCoreCritical("inference failed for g2p '%1': %2", 
                                        g2pId, resultExp.error().message());
                for (const auto &lyric : lyricVec)
                    result.emplace_back(G2pRes(lyric, g2pId, lyric, {lyric}, "copy", ModelInferenceFailed));
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
                    result.emplace_back(G2pRes(lyric, g2pId, lyric, {lyric}, "copy", UnknownError));
            }
        }

        return result;
    }
} // namespace LangCore
