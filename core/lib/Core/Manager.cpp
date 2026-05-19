#include "Manager.h"
#include "Manager_p.h"

#include <set>

#include <stdcorelib/path.h>
#include <stdcorelib/pimpl.h>

#include <LangCore/Core/ManagerLogger.h>
#include <LangCore/Module/Dependency/LevelCompatibilityChecker.h>
#include <LangCore/Support/ContextUtils.h>
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
        // Iterate all contexts with Ready state and load tasks
        for (const auto &[ctxKey, state] : impl.contextStates) {
            if (state != Impl::ContextState::Ready)
                continue;

            auto &ctxModuleInfos = impl.contextModuleInfos[ctxKey];
            for (const auto &moduleInfo : ctxModuleInfos) {
                if (moduleInfo.type != category)
                    continue;

                // Look up via FQID in ObjectPool
                const auto fqid = ContextUtils::formatFqid(ctxKey, moduleInfo.moduleId);
                const auto inferenceCate = this->category(category);
                if (!inferenceCate)
                    continue;

                const auto obj = inferenceCate->getFirstObject(fqid);
                if (!obj)
                    continue;

                impl.tasks[category][ctxKey][moduleInfo.moduleId] = obj.as<Task>();
            }
        }
        return true;
    }

    Expected<void> Manager::initialize() {
        __stdc_impl_t;

        // Phase 1: Default context ("")
        {
            MgrLog.langCoreInfo("Phase 1: Initializing default context");

            ContextKey defaultCtx("");
            const auto moduleInfos = this->getModuleMetadatas(defaultCtx);
            if (moduleInfos.empty()) {
                MgrLog.langCoreCritical("Default context: No modules found");
                return Error(Error::InitializationError,
                             "Ord-1: Default context initialization failed: no modules found");
            }

            // Level compatibility
            LevelCompatibilityChecker::LevelConfig levelConfig;
            levelConfig.currentLevel = impl.currentLevel;
            levelConfig.minimumLevel = impl.minimumLevel;
            levelConfig.maximumLevel = impl.maximumLevel;

            bool hasIncompatible = false;
            for (const auto &info : moduleInfos) {
                auto checkResult = LevelCompatibilityChecker::checkCorePlugin(info.level, levelConfig);
                if (!checkResult.isCompatible) {
                    MgrLog.langCoreCritical("Level Compatibility Check Failed for %1:%2 (level %3)",
                                            info.packageId, info.moduleId, std::to_string(info.level));
                    hasIncompatible = true;
                }
            }
            if (hasIncompatible) {
                return Error(Error::InitializationError,
                             "Ord-1: Default context has incompatible modules");
            }

            // Build dependency graph
            impl.dependencyGraph.clear();
            for (const auto &info : moduleInfos)
                impl.dependencyGraph.addModule(info);

            if (!impl.dependencyGraph.buildGraph()) {
                return Error(Error::DependencyError,
                             "Ord-1: Failed to build dependency graph for default context");
            }

            if (const auto cycles = impl.dependencyGraph.findCycles(); !cycles.empty()) {
                return Error(Error::DependencyError,
                             "Ord-1: Circular dependencies detected in default context");
            }

            // Load packages in order
            auto packageOrder = impl.dependencyGraph.getPackageInitializationOrder();
            int failedPkgCount = 0;
            for (const auto &packageInfo : packageOrder) {
                MgrLog.langCoreInfo("Loading package: %1 from %2", packageInfo.packageId, packageInfo.packagePath);
                auto exp = this->open(packageInfo.packagePath);
                if (!exp) {
                    MgrLog.langCoreCritical("Failed to open package %1: %2", packageInfo.packageId,
                                            exp.error().message());
                    failedPkgCount++;
                    continue;
                }

                Package pkg = exp.take();
                if (!pkg.isLoaded()) {
                    MgrLog.langCoreCritical("Failed to load package %1: %2", packageInfo.packageId,
                                            pkg.error().message());
                    failedPkgCount++;
                    continue;
                }

                for (const auto &moduleInfo : packageInfo.initializationOrder) {
                    if (auto taskExp = createModuleTask(moduleInfo, pkg)) {
                        MgrLog.langCoreInfo("  Created task for module: %1 (type: %2)", moduleInfo.moduleId,
                                            moduleInfo.type);
                    } else {
                        MgrLog.langCoreCritical("  Failed to create task for module: %1: %2", moduleInfo.moduleId,
                                                taskExp.error().message());
                    }
                }
            }

            if (failedPkgCount > 0 && packageOrder.size() == static_cast<size_t>(failedPkgCount)) {
                return Error(Error::InitializationError,
                             "Ord-1: All packages failed to load in default context");
            }

            impl.contextStates[defaultCtx] = Impl::ContextState::Ready;
        }

        // Phase 2: Non-default contexts
        for (const auto &[ctxKey, _] : impl.contextPackagePaths) {
            if (ctxKey.isDefault())
                continue;

            MgrLog.langCoreInfo("Phase 2: Initializing context '%1'", ctxKey.toString());

            const auto moduleInfos = this->getModuleMetadatas(ctxKey);
            if (moduleInfos.empty()) {
                MgrLog.langCoreCritical("Context '%1': No modules found, marking Failed", ctxKey.toString());
                impl.contextStates[ctxKey] = Impl::ContextState::Failed;
                continue;
            }

            // Level compatibility
            LevelCompatibilityChecker::LevelConfig levelConfig;
            levelConfig.currentLevel = impl.currentLevel;
            levelConfig.minimumLevel = impl.minimumLevel;
            levelConfig.maximumLevel = impl.maximumLevel;

            bool hasIncompatible = false;
            for (const auto &info : moduleInfos) {
                auto checkResult = LevelCompatibilityChecker::checkCorePlugin(info.level, levelConfig);
                if (!checkResult.isCompatible) {
                    MgrLog.langCoreCritical("Context '%1': Level incompatible module %2:%3",
                                            ctxKey.toString(), info.packageId, info.moduleId);
                    hasIncompatible = true;
                }
            }
            if (hasIncompatible) {
                MgrLog.langCoreCritical("Context '%1': Has incompatible modules, marking Failed", ctxKey.toString());
                impl.contextStates[ctxKey] = Impl::ContextState::Failed;
                continue;
            }

            // Build dependency graph (fresh)
            impl.dependencyGraph.clear();
            for (const auto &info : moduleInfos)
                impl.dependencyGraph.addModule(info);

            if (!impl.dependencyGraph.buildGraph()) {
                MgrLog.langCoreCritical("Context '%1': Failed to build dependency graph", ctxKey.toString());
                impl.contextStates[ctxKey] = Impl::ContextState::Failed;
                continue;
            }

            if (const auto cycles = impl.dependencyGraph.findCycles(); !cycles.empty()) {
                MgrLog.langCoreCritical("Context '%1': Circular dependencies detected", ctxKey.toString());
                impl.contextStates[ctxKey] = Impl::ContextState::Failed;
                continue;
            }

            // Load packages
            auto packageOrder = impl.dependencyGraph.getPackageInitializationOrder();
            bool allFailed = true;
            for (const auto &packageInfo : packageOrder) {
                auto exp = this->open(packageInfo.packagePath);
                if (!exp) {
                    MgrLog.langCoreCritical("Context '%1': Failed to open package %2",
                                            ctxKey.toString(), packageInfo.packageId);
                    continue;
                }

                Package pkg = exp.take();
                if (!pkg.isLoaded()) {
                    MgrLog.langCoreCritical("Context '%1': Failed to load package %2",
                                            ctxKey.toString(), packageInfo.packageId);
                    continue;
                }

                allFailed = false;
                for (const auto &moduleInfo : packageInfo.initializationOrder) {
                    if (auto taskExp = createModuleTask(moduleInfo, pkg)) {
                        MgrLog.langCoreInfo("  Context '%1': Created task for module: %2",
                                            ctxKey.toString(), moduleInfo.moduleId);
                    } else {
                        MgrLog.langCoreCritical("  Context '%1': Failed to create task for module: %2: %3",
                                                ctxKey.toString(), moduleInfo.moduleId, taskExp.error().message());
                    }
                }
            }

            if (allFailed && !packageOrder.empty()) {
                impl.contextStates[ctxKey] = Impl::ContextState::Failed;
            } else {
                impl.contextStates[ctxKey] = Impl::ContextState::Ready;
            }
        }

        // Phase 3: Load tasks for categories
        if (auto result = loadTasksForCategory("g2p"); !result) {
            return Error(Error::RuntimeError, "Failed to load g2p tasks: " + result.error().message());
        }

        // dict is optional
        if (auto result = loadTasksForCategory("dict"); !result) {
            MgrLog.langCoreInfo("No dict tasks loaded (this is normal if no dict plugins are installed)");
        }

        impl.initialized = true;
        return {};
    }

    bool Manager::initialized() const {
        __stdc_impl_t;
        return impl.initialized;
    }

    Expected<NO<Task>> Manager::task(const std::string &category, const std::string &context,
                                     const std::string &id) const {
        return task(category, context, {}, id);
    }

    Expected<NO<Task>> Manager::task(const std::string &category, const std::string &context,
                                     const stdc::VersionNumber &version, const std::string &id) const {
        // T-1: category validation
        if (category.empty())
            return Error(Error::ValidationError, "T-1: category cannot be empty");

        // T-2: context validation
        if (auto exp = ContextUtils::validateContextName(context); !exp)
            return Error(Error::ValidationError, "T-2: " + exp.error().message());

        // T-3: id validation
        if (id.empty())
            return Error(Error::ValidationError, "T-3: id cannot be empty");

        // T-4: id must not contain ':'
        if (auto exp = ContextUtils::validateModuleId(id); !exp)
            return Error(Error::ValidationError, "T-4: " + exp.error().message());

        __stdc_impl_t;

        // T-5: category exists
        auto catIt = impl.tasks.find(category);
        if (catIt == impl.tasks.end())
            return Error(Error::RuntimeError, "T-5: could not find category: " + category);

        // T-6: context exists and is Ready (two-step: exact match, then unversioned fallback)
        ContextKey ctxKey(context, version);

        // Step 1: exact match
        auto ctxIt = catIt->second.find(ctxKey);

        // Step 2: fallback to unversioned (only if versioned was requested)
        if (ctxIt == catIt->second.end() && !version.isEmpty()) {
            ctxKey = ContextKey(context);
            ctxIt = catIt->second.find(ctxKey);
        }

        if (ctxIt == catIt->second.end()) {
            // Check if context is Failed
            auto stateIt = impl.contextStates.find(ContextKey(context, version));
            if (stateIt == impl.contextStates.end() && !version.isEmpty())
                stateIt = impl.contextStates.find(ContextKey(context));
            if (stateIt != impl.contextStates.end() && stateIt->second == Impl::ContextState::Failed)
                return Error(Error::RuntimeError,
                             "T-6: context '" + ContextKey(context, version).toString() + "' failed initialization");
            return Error(Error::RuntimeError,
                         "T-6: could not find context: " + ContextKey(context, version).toString());
        }

        // T-7: id exists
        auto idIt = ctxIt->second.find(id);
        if (idIt == ctxIt->second.end())
            return Error(Error::RuntimeError, "T-7: could not find id: " + id + " in context " + ctxKey.toString());

        return idIt->second;
    }

    Expected<std::vector<NO<Task>>> Manager::tasks(const std::string &category, const std::string &context) const {
        return tasks(category, context, {});
    }

    Expected<std::vector<NO<Task>>> Manager::tasks(const std::string &category, const std::string &context,
                                                    const stdc::VersionNumber &version) const {
        if (category.empty())
            return Error(Error::ValidationError, "category cannot be empty");

        if (auto exp = ContextUtils::validateContextName(context); !exp)
            return exp.error();

        __stdc_impl_t;

        auto catIt = impl.tasks.find(category);
        if (catIt == impl.tasks.end())
            return Error(Error::RuntimeError, "could not find category: " + category);

        // Two-step lookup: exact match, then unversioned fallback
        ContextKey ctxKey(context, version);
        auto ctxIt = catIt->second.find(ctxKey);
        if (ctxIt == catIt->second.end() && !version.isEmpty()) {
            ctxKey = ContextKey(context);
            ctxIt = catIt->second.find(ctxKey);
        }

        if (ctxIt == catIt->second.end())
            return Error(Error::RuntimeError, "could not find context: " + ContextKey(context, version).toString());

        std::vector<NO<Task>> result;
        result.reserve(ctxIt->second.size());
        for (const auto &[_, t] : ctxIt->second)
            result.push_back(t);

        if (result.empty())
            return Error(Error::RuntimeError,
                         "category: " + category + " is empty in context " + ctxKey.toString());
        return result;
    }

    std::vector<G2pRes> Manager::convert(const std::vector<G2pInput> &input) {
        if (input.empty())
            return {};

        __stdc_impl_t;
        std::vector<G2pRes> result;
        result.reserve(input.size());

        // Group by (context, contextVersion, g2pId) — adjacent grouping
        struct Group {
            std::string context;
            stdc::VersionNumber contextVersion;
            std::string g2pId;
            std::vector<std::string> lyrics;
            std::vector<size_t> resultIndexes;
        };

        std::vector<Group> groups;
        result.resize(input.size());

        for (size_t i = 0; i < input.size(); ++i) {
            const auto &item = input[i];

            // C-2: skip empty lyric
            if (item.lyric.empty()) {
                result[i] = G2pRes("", item.g2pId, item.context, item.contextVersion, "", {}, "skip", NoError);
                continue;
            }

            // C-3: skip empty g2pId
            if (item.g2pId.empty()) {
                result[i] = G2pRes(item.lyric, "", item.context, item.contextVersion, item.lyric, {item.lyric},
                                   "copy", UnknownError);
                MgrLog.langCoreWarning("C-3: empty g2pId for lyric '%1', skipping", item.lyric);
                continue;
            }

            // C-4: validate context chars
            if (auto exp = ContextUtils::validateContextName(item.context); !exp) {
                result[i] = G2pRes(item.lyric, item.g2pId, item.context, item.contextVersion, item.lyric,
                                   {item.lyric}, "copy", UnknownError);
                MgrLog.langCoreWarning("C-4: invalid context '%1' for lyric '%2'", item.context, item.lyric);
                continue;
            }

            // Adjacent grouping
            if (groups.empty() || groups.back().context != item.context ||
                groups.back().contextVersion != item.contextVersion || groups.back().g2pId != item.g2pId) {
                groups.push_back({item.context, item.contextVersion, item.g2pId, {}, {}});
            }
            groups.back().lyrics.push_back(item.lyric);
            groups.back().resultIndexes.push_back(i);
        }

        const auto _input = NO<G2pInputV1>::create();

        for (const auto &group : groups) {
            // Lookup task: NO fallback to default context (C-6)
            auto catIt = impl.tasks.find("g2p");
            NO<Task> taskObj;
            if (catIt != impl.tasks.end()) {
                // Two-step ContextKey lookup
                ContextKey ctxKey(group.context, group.contextVersion);
                auto ctxIt = catIt->second.find(ctxKey);
                if (ctxIt == catIt->second.end() && !group.contextVersion.isEmpty()) {
                    ctxIt = catIt->second.find(ContextKey(group.context));
                }
                if (ctxIt != catIt->second.end()) {
                    auto idIt = ctxIt->second.find(group.g2pId);
                    if (idIt != ctxIt->second.end())
                        taskObj = idIt->second;
                }
            }

            if (!taskObj) {
                // Diagnose why the lookup failed
                bool contextEverRegistered = false;
                for (const auto &[regKey, _] : impl.contextPackagePaths) {
                    if (regKey.context == group.context) {
                        contextEverRegistered = true;
                        break;
                    }
                }

                if (!contextEverRegistered && !group.context.empty()) {
                    MgrLog.langCoreCritical("C-5: context '%1' was never registered via addPackagePath", group.context);
                } else if (contextEverRegistered) {
                    // Context was registered but g2pId or version not found
                    auto ctxDisplay = ContextKey(group.context, group.contextVersion).toString();
                    MgrLog.langCoreCritical("C-6: g2p '%1' not found in context '%2' (check g2pId spelling or version)",
                                            group.g2pId, ctxDisplay);
                } else {
                    MgrLog.langCoreCritical("C-6: fail to find g2p '%1' in context '%2'",
                                            group.g2pId,
                                            ContextKey(group.context, group.contextVersion).toString());
                }
                for (size_t j = 0; j < group.lyrics.size(); ++j) {
                    result[group.resultIndexes[j]] =
                        G2pRes(group.lyrics[j], group.g2pId, group.context, group.contextVersion, group.lyrics[j],
                               {group.lyrics[j]}, "copy", UnknownError);
                }
                continue;
            }

            _input->g2pInput = group.lyrics;
            auto resultExp = taskObj->start(_input);
            if (!resultExp) {
                MgrLog.langCoreCritical("inference failed for g2p '%1': %2",
                                        group.g2pId, resultExp.error().message());
                for (size_t j = 0; j < group.lyrics.size(); ++j) {
                    result[group.resultIndexes[j]] =
                        G2pRes(group.lyrics[j], group.g2pId, group.context, group.contextVersion, group.lyrics[j],
                               {group.lyrics[j]}, "copy", ModelInferenceFailed);
                }
                continue;
            }

            const auto _result = resultExp.take();
            if (const auto g2pRes = _result.as<G2pResultV1>()) {
                if (!g2pRes->errorMessage.empty())
                    MgrLog.langCoreCritical("Error: %1", g2pRes->errorMessage);

                for (size_t j = 0; j < g2pRes->g2pResult.size() && j < group.resultIndexes.size(); ++j) {
                    auto res = g2pRes->g2pResult[j];
                    res.context = group.context;
                    res.contextVersion = group.contextVersion;
                    result[group.resultIndexes[j]] = std::move(res);
                }
            } else {
                MgrLog.langCoreCritical("unexpected result type for g2p '%1'", group.g2pId);
                for (size_t j = 0; j < group.lyrics.size(); ++j) {
                    result[group.resultIndexes[j]] =
                        G2pRes(group.lyrics[j], group.g2pId, group.context, group.contextVersion, group.lyrics[j],
                               {group.lyrics[j]}, "copy", UnknownError);
                }
            }
        }

        return result;
    }
} // namespace LangCore
