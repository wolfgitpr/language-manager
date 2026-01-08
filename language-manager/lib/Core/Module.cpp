#include "Module.h"
#include "Module_p.h"

#include <cstdlib>
#include <fstream>
#include <regex>
#include <utility>

#include <stdcorelib/path.h>
#include <stdcorelib/pimpl.h>
#include <stdcorelib/str.h>

#include "EngineFactory.h"
#include "EngineFactoryPlugin.h"

#include "Manager_p.h"
#include "PackageRef_p.h"

namespace fs = std::filesystem;

namespace LangMgr
{

    std::string ModuleLocator::toString() const {
        if (_package.empty()) {
            return _id;
        }
        if (_version.isEmpty()) {
            if (_id.empty()) {
                return _package;
            }
            return stdc::formatN("%1/%2", _package, _id);
        }
        if (_id.empty()) {
            return stdc::formatN("%1[%2]", _package, _version.toString());
        }
        return stdc::formatN("%1[%2]/%3", _package, _version.toString(), _id);
    }

    // Format: id/sid, id[version]/sid, and sid
    ModuleLocator ModuleLocator::fromString(const std::string_view &token) {
        if (token.empty()) {
            return {};
        }

        ModuleLocator result;
        if (const size_t slashPos = token.find('/'); slashPos != std::string::npos) {
            // Case: id/sid or id[version]/sid
            auto leftPart = token.substr(0, slashPos);
            const auto rightPart = token.substr(slashPos + 1);
            if (!isValidLocator(rightPart)) {
                return {};
            }
            result._id = rightPart;

            if (const size_t openBracket = leftPart.find('['); openBracket != std::string::npos) {
                if (leftPart.back() != ']') {
                    return {};
                }
                const auto package = leftPart.substr(0, openBracket);
                if (!isValidLocator(package)) {
                    return {};
                }
                // id[version]
                result._package = package;
                result._version = stdc::VersionNumber::fromString(
                    leftPart.substr(openBracket + 1, leftPart.size() - openBracket - 1));
            } else {
                if (!isValidLocator(leftPart)) {
                    return {};
                }
                // just id
                result._package = leftPart;
            }
        } else {
            // Case: sid only
            if (!isValidLocator(token)) {
                return {};
            }
            result._id = token;
        }
        return result;
    }

    bool ModuleLocator::isValidLocator(const std::string_view &token) {
        if (token.empty()) {
            return false;
        }
        for (const auto &ch : token) {
            switch (ch) {
            case '/':
            case '\\':
            case '[':
            case ']':
            case ':':
            case ';':
            case '\'':
            case '\"':
                return false;
            default:
                break;
            }
        }
        return true;
    }

    ModuleDefinition::~ModuleDefinition() = default;

    const std::string &ModuleDefinition::id() const {
        __stdc_impl_t;
        return impl.id;
    }

    const std::string &ModuleDefinition::category() const {
        __stdc_impl_t;
        return impl.category;
    }

    const std::string &ModuleDefinition::className() const {
        __stdc_impl_t;
        return impl.className;
    }

    DisplayText ModuleDefinition::name() const {
        __stdc_impl_t;
        return impl.name;
    }

    int ModuleDefinition::apiLevel() const {
        __stdc_impl_t;
        return impl.apiLevel;
    }

    const JsonObject &ModuleDefinition::manifestConfiguration() const {
        __stdc_impl_t;
        return impl.manifestConfiguration;
    }

    NO<TaskConfiguration> ModuleDefinition::configuration() const {
        __stdc_impl_t;
        return impl.configuration;
    }

    const std::filesystem::path &ModuleDefinition::path() const {
        __stdc_impl_t;
        return impl.path;
    }

    Expected<NO<Task>> ModuleDefinition::createTask(const NO<TaskRuntimeOptions> &runtimeOptions) const {
        __stdc_impl_t;
        return impl.interp->createTask(this, runtimeOptions);
    }

    ModuleDefinition::State ModuleDefinition::state() const {
        __stdc_impl_t;
        return impl.state;
    }

    Package ModuleDefinition::parent() const {
        __stdc_impl_t;
        return Package(impl.package);
    }

    Manager *ModuleDefinition::Mgr() const {
        __stdc_impl_t;
        return impl.package->mgr;
    }

    ModuleDefinition::ModuleDefinition(Impl &impl) : _impl(&impl) {}

    ModuleDefinition::ModuleDefinition(std::string category) : _impl(new Impl(std::move(category))) {}

    ModuleCategory::Impl::~Impl() {}

    std::vector<ModuleDefinition *> ModuleCategory::Impl::findModuleSpecs(const ModuleLocator &loc) const {
        std::shared_lock lock(su_mtx());
        if (loc.package().empty() || loc.version().isEmpty()) {
            return {};
        }
        const auto it = indexes.find(loc.package());
        if (it == indexes.end()) {
            return {};
        }
        const auto &versionMap = it->second;

        const auto it2 = versionMap.find(loc.version());
        if (it2 == versionMap.end()) {
            return {};
        }
        const auto &inferenceMap = it2->second;

        if (!loc.id().empty()) {
            const auto it3 = inferenceMap.find(loc.id());
            if (it3 == inferenceMap.end()) {
                return {};
            }
            return {*it3->second};
        }

        std::vector<ModuleDefinition *> res;
        res.reserve(inferenceMap.size());
        for (const auto &[fst, snd] : inferenceMap) {
            res.push_back(*snd);
        }
        return res;
    }

    ModuleCategory::~ModuleCategory() = default;

    const std::string &ModuleCategory::name() const {
        __stdc_impl_t;
        return impl.name;
    }

    Manager *ModuleCategory::Mgr() const {
        __stdc_impl_t;
        return impl.mgr;
    }

    Expected<void> ModuleDefinition::Impl::read(const std::filesystem::path &basePath, const JsonObject &obj) {
        fs::path configPath;
        stdc::VersionNumber fmtVersion_;
        std::string id_;
        std::string className_;

        DisplayText name_;
        int apiLevel_;

        JsonObject configuration_;

        // Parse desc
        {
            // id
            auto it = obj.find("moduleId");
            if (it == obj.end()) {
                return Error{
                    Error::InvalidFormat,
                    R"(missing "moduleId" field in inference module field)",
                };
            }
            id_ = it->second.toString();
            if (!ModuleLocator::isValidLocator(id_)) {
                return Error{
                    Error::InvalidFormat,
                    R"("moduleId" field has invalid value in inference module field)",
                };
            }

            // class
            it = obj.find("class");
            if (it == obj.end()) {
                return Error{
                    Error::InvalidFormat,
                    R"(missing "class" field in inference module field)",
                };
            }
            className_ = it->second.toString();
            if (className_.empty()) {
                return Error{
                    Error::InvalidFormat,
                    R"("class" field has invalid value in inference module field)",
                };
            }

            // configuration
            it = obj.find("configuration");
            if (it == obj.end()) {
                return Error{
                    Error::InvalidFormat,
                    R"(missing "configuration" field in inference module field)",
                };
            }

            std::string configPathString = it->second.toString();
            if (configPathString.empty()) {
                return Error{
                    Error::InvalidFormat,
                    R"("configuration" field has invalid value in inference module field)",
                };
            }

            configPath = stdc::path::from_utf8(configPathString);
            if (auto configPathExtension = stdc::to_lower(configPath.extension().string());
                configPathExtension != ".json") {
                configPath += ".json";
            }
            if (configPath.is_relative()) {
                configPath = basePath / configPath;
            }
        }

        // Read configuration
        JsonObject configObj;
        {
            std::ifstream file(configPath);
            if (!file.is_open()) {
                return Error{
                    Error::FileNotOpen,
                    stdc::formatN(R"(%1: failed to open inference manifest)", configPath),
                };
            }

            std::stringstream ss;
            ss << file.rdbuf();

            std::string error2;
            auto root = JsonValue::fromJson(ss.str(), true, &error2);
            if (!error2.empty()) {
                return Error{
                    Error::InvalidFormat,
                    stdc::formatN(R"(%1: invalid inference manifest format: %2)", configPath, error2),
                };
            }
            if (!root.isObject()) {
                return Error{
                    Error::InvalidFormat,
                    stdc::formatN(R"(%1: invalid inference manifest format)", configPath),
                };
            }
            configObj = root.toObject();
        }

        // Get attributes
        // $version
        {
            if (auto it = configObj.find("$version"); it == configObj.end()) {
                fmtVersion_ = stdc::VersionNumber(1);
            } else {
                fmtVersion_ = stdc::VersionNumber::fromString(it->second.toString());
                if (fmtVersion_ > stdc::VersionNumber(1)) {
                    return Error{
                        Error::FeatureNotSupported,
                        stdc::formatN(R"(%1: format version "%2" is not supported)", configPath,
                                      fmtVersion_.toString()),
                    };
                }
            }
        }
        // name
        {
            if (auto it = configObj.find("name"); it != configObj.end()) {
                name_ = it->second;
            }
            if (name_.isEmpty()) {
                name_ = id_;
            }
        }
        // level
        {
            auto it = configObj.find("level");
            if (it == configObj.end()) {
                return Error{
                    Error::InvalidFormat,
                    stdc::formatN(R"(%1: missing "level" field)", configPath),
                };
            }
            apiLevel_ = it->second.toInt();
            if (apiLevel_ == 0) {
                return Error{
                    Error::InvalidFormat,
                    stdc::formatN(R"(%1: "level" field has invalid value)", configPath),
                };
            }
        }

        // configuration
        {
            if (auto it = configObj.find("configuration"); it != configObj.end()) {
                if (!it->second.isObject()) {
                    return Error{
                        Error::InvalidFormat,
                        stdc::formatN(R"(%1: "configuration" field has invalid value)", configPath),
                    };
                }
                configuration_ = it->second.toObject();
            }
        }

        path = fs::canonical(configPath).parent_path();
        fmtVersion = fmtVersion_;
        id = std::move(id_);
        className = std::move(className_);
        name = name_;
        apiLevel = apiLevel_;
        manifestConfiguration = std::move(configuration_);
        return {};
    }

    std::vector<ModuleDefinition *> ModuleCategory::findDefinitions(const ModuleLocator &identifier) const {
        __stdc_impl_t;
        std::vector<ModuleDefinition *> res;
        auto temp = impl.findModuleSpecs(identifier);
        res.reserve(res.size());
        for (const auto &item : std::as_const(temp)) {
            res.push_back(static_cast<ModuleDefinition *>(item));
        }
        return res;
    }

    std::vector<ModuleDefinition *> ModuleCategory::definitions() const {
        __stdc_impl_t;
        std::shared_lock lock(impl.su_mtx());
        std::vector<ModuleDefinition *> res;
        res.reserve(impl.modules.size());
        for (const auto &item : impl.modules) {
            res.push_back(static_cast<ModuleDefinition *>(item));
        }
        return res;
    }

    Expected<ModuleDefinition *> ModuleCategory::parseDefinition(const std::filesystem::path &basePath,
                                                                 const JsonValue &config) const {
        __stdc_impl_t;
        if (!config.isObject()) {
            return Error{
                Error::InvalidFormat,
                R"(invalid inference specification)",
            };
        }
        auto spec = new ModuleDefinition(this->category());
        if (const auto exp = spec->_impl->read(basePath, config.toObject()); !exp) {
            delete spec;
            return exp.error();
        }
        return spec;
    }

    Expected<void> ModuleCategory::loadDefinitionBase(ModuleDefinition *definition,
                                                      const ModuleDefinition::State state) {
        __stdc_impl_t;

        const auto spec_impl = definition->_impl.get();
        switch (state) {
        case ModuleDefinition::Initialized:
            {
                std::unique_lock lock(impl.su_mtx());
                const auto lib = spec_impl->package;
                const auto it = impl.modules.insert(impl.modules.end(), definition);
                impl.indexes[lib->id][lib->version][spec_impl->id] = it;
                return Expected<void>();
            }

        case ModuleDefinition::Ready:
        case ModuleDefinition::Finished:
            {
                return Expected<void>();
            }

        case ModuleDefinition::Deleted:
            {
                std::unique_lock lock(impl.su_mtx());
                const auto lib = spec_impl->package;
                const auto it = impl.indexes.find(lib->id);
                if (it == impl.indexes.end()) {
                    return Expected<void>();
                }
                auto &versionMap = it->second;
                const auto it2 = versionMap.find(lib->version);
                if (it2 == versionMap.end()) {
                    return Expected<void>();
                }
                auto &inferenceMap = it2->second;
                const auto it3 = inferenceMap.find(spec_impl->id);
                if (it3 == inferenceMap.end()) {
                    return Expected<void>();
                }
                impl.modules.erase(it3->second);
                inferenceMap.erase(it3);
                if (inferenceMap.empty()) {
                    versionMap.erase(it2);
                    if (versionMap.empty()) {
                        impl.indexes.erase(it);
                    }
                }
                return Expected<void>();
            }
        default:
            break;
        }
        std::abort();
    }

    Expected<void> ModuleCategory::loadDefinition(ModuleDefinition *spec, const ModuleDefinition::State state) {
        __stdc_impl_t;
        const auto spec_impl = spec->_impl.get();
        switch (state) {
        case ModuleDefinition::Initialized:
            {
                const auto &key = spec->className();
                NO<EngineFactory> interp;

                // Search interpreter cache
                if (const auto it = impl.interpreters.find(key); it != impl.interpreters.end()) {
                    interp = it->second;
                } else {
                    // Search interpreter
                    const auto plugin = Mgr()->plugin<EngineFactoryPlugin>(spec->className().c_str());
                    if (!plugin) {
                        return Error{
                            Error::FeatureNotSupported,
                            stdc::formatN(R"(required interpreter "%1" of inference "%2" not found)", spec->className(),
                                          spec->id()),
                        };
                    }
                    interp = plugin->create();
                    impl.interpreters[key] = interp;
                }

                // Check api level
                if (interp->apiLevel() < spec->apiLevel()) {
                    return Error{
                        Error::FeatureNotSupported,
                        stdc::formatN(
                            R"(required interpreter "%1" of api level %2 doesn't support inference "%3" of api level %4)",
                            spec->className(), interp->apiLevel(), spec->id(), spec->apiLevel()),
                    };
                }

                auto config = interp->createConfiguration(spec);
                if (!config) {
                    return Error{
                        Error::InvalidFormat,
                        stdc::formatN(R"(failed to parse inference configuration of "%1": %2)", spec->id(),
                                      config.error().message()),
                    };
                }
                spec_impl->configuration = config.get();
                spec_impl->interp = interp;
                return loadDefinitionBase(spec, state);
            }

        case ModuleDefinition::Ready:
        case ModuleDefinition::Finished:
            return {};

        case ModuleDefinition::Deleted:
            return loadDefinitionBase(spec, state);
        default:
            break;
        }
        return {};
    }

    std::vector<ModuleDefinition *> ModuleCategory::find(const ModuleLocator &loc) const {
        __stdc_impl_t;
        return impl.findModuleSpecs(loc);
    }

    ModuleCategory::ModuleCategory(Impl &impl) : ObjectPool(impl) {}

    ModuleCategory::ModuleCategory(std::string name, Manager *mgr) :
        ObjectPool(*new Impl(this, std::move(name), mgr)) {}

} // namespace LangMgr
