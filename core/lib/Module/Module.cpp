#include "LangCore/Module/Module.h"
#include "Module_p.h"

#include <cstdlib>
#include <fstream>
#include <mutex>
#include <utility>

#include <stdcorelib/path.h>
#include <stdcorelib/pimpl.h>
#include <stdcorelib/str.h>

#include <LangCore/Package/Package.h>

#include "PackageManager_p.h"
#include "Package_p.h"

namespace fs = std::filesystem;

namespace LangCore
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

    ModuleSpec::~ModuleSpec() = default;

    const std::string &ModuleSpec::id() const {
        __stdc_impl_t;
        return impl.id;
    }

    const std::string &ModuleSpec::category() const {
        __stdc_impl_t;
        return impl.category;
    }

    const std::string &ModuleSpec::className() const {
        __stdc_impl_t;
        return impl.className;
    }

    std::string ModuleSpec::name() const {
        __stdc_impl_t;
        // 无感调用：自动返回当前语言的本地化文本
        return impl.name.text();
    }

    std::string ModuleSpec::configurationDisplayName(const std::string &configKey) const {
        __stdc_impl_t;
        
        auto it = impl.configurationDisplayNames.find(configKey);
        if (it != impl.configurationDisplayNames.end()) {
            // 无感调用：自动返回当前语言的本地化文本
            return it->second.text();
        }
        
        // 如果没有找到显示名称，返回原始键名
        return configKey;
    }

    int ModuleSpec::apiLevel() const {
        __stdc_impl_t;
        return impl.apiLevel;
    }

    const JsonObject &ModuleSpec::manifestConfiguration() const {
        __stdc_impl_t;
        return impl.manifestConfiguration;
    }

    NO<TaskConfiguration> ModuleSpec::configuration() const {
        __stdc_impl_t;
        return impl.configuration;
    }

    const std::filesystem::path &ModuleSpec::path() const {
        __stdc_impl_t;
        return impl.path;
    }

    ModuleSpec::State ModuleSpec::state() const {
        __stdc_impl_t;
        return impl.state;
    }

    Package ModuleSpec::parent() const {
        __stdc_impl_t;
        return Package(impl.package);
    }

    PackageManager *ModuleSpec::Mgr() const {
        __stdc_impl_t;
        return impl.package->mgr;
    }

    ModuleSpec::ModuleSpec(Impl &impl) : _impl(&impl) {}

    ModuleSpec::ModuleSpec(std::string category) : _impl(new Impl(std::move(category))) {}

    ModuleCategory::Impl::~Impl() {}

    std::vector<ModuleSpec *> ModuleCategory::Impl::findModuleSpecs(const ModuleLocator &loc) const {
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

        const auto &moduleMap = it2->second;

        if (!loc.id().empty()) {
            // 查找指定moduleId的所有level版本
            const auto it3 = moduleMap.find(loc.id());
            if (it3 == moduleMap.end()) {
                return {};
            }

            std::vector<ModuleSpec *> res;
            for (const auto &[level, iter] : it3->second) {
                res.push_back(*iter);
            }
            return res;
        }

        std::vector<ModuleSpec *> res;
        for (const auto &[moduleId, levelMap] : moduleMap) {
            for (const auto &[level, iter] : levelMap) {
                res.push_back(*iter);
            }
        }
        return res;
    }

    ModuleCategory::~ModuleCategory() = default;

    const std::string &ModuleCategory::name() const {
        __stdc_impl_t;
        return impl.name;
    }

    PackageManager *ModuleCategory::Mgr() const {
        __stdc_impl_t;
        return impl.mgr;
    }

    Expected<void> ModuleSpec::Impl::read(const std::filesystem::path &basePath, const JsonObject &obj) {
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
                    Error::ConfigError,
                    R"(missing "moduleId" field in inference module field)",
                };
            }
            id_ = it->second.toString();
            if (!ModuleLocator::isValidLocator(id_)) {
                return Error{
                    Error::ConfigError,
                    R"("moduleId" field has invalid value in inference module field)",
                };
            }

            // class
            it = obj.find("class");
            if (it == obj.end()) {
                return Error{
                    Error::ConfigError,
                    R"(missing "class" field in inference module field)",
                };
            }
            className_ = it->second.toString();
            if (className_.empty()) {
                return Error{
                    Error::ConfigError,
                    R"("class" field has invalid value in inference module field)",
                };
            }

            // configuration
            it = obj.find("configuration");
            if (it == obj.end()) {
                return Error{
                    Error::ConfigError,
                    R"(missing "configuration" field in inference module field)",
                };
            }

            std::string configPathString = it->second.toString();
            if (configPathString.empty()) {
                return Error{
                    Error::ConfigError,
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
                    Error::FileSystemError,
                    stdc::formatN(R"(%1: failed to open inference manifest)", configPath),
                };
            }

            std::stringstream ss;
            ss << file.rdbuf();

            std::string error2;
            auto root = JsonValue::fromJson(ss.str(), true, &error2);
            if (!error2.empty()) {
                return Error{
                    Error::ConfigError,
                    stdc::formatN(R"(%1: invalid inference manifest format: %2)", configPath, error2),
                };
            }
            if (!root.isObject()) {
                return Error{
                    Error::ConfigError,
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
                        Error::NotImplementedError,
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
                    Error::ConfigError,
                    stdc::formatN(R"(%1: missing "level" field)", configPath),
                };
            }
            apiLevel_ = it->second.toInt();
            if (apiLevel_ == 0) {
                return Error{
                    Error::ConfigError,
                    stdc::formatN(R"(%1: "level" field has invalid value)", configPath),
                };
            }
        }

        // configuration
        {
            if (auto it = configObj.find("configuration"); it != configObj.end()) {
                if (!it->second.isObject()) {
                    return Error{
                        Error::ConfigError,
                        stdc::formatN(R"(%1: "configuration" field has invalid value)", configPath),
                    };
                }
                configuration_ = it->second.toObject();
            }
        }

        // configurationDisplayNames
        {
            if (auto it = configObj.find("configurationDisplayNames"); it != configObj.end()) {
                if (!it->second.isObject()) {
                    return Error{
                        Error::ConfigError,
                        stdc::formatN(R"(%1: "configurationDisplayNames" field has invalid value)", configPath),
                    };
                }
                const auto &displayNamesObj = it->second.toObject();
                for (const auto &[key, value] : displayNamesObj) {
                    configurationDisplayNames[key] = DisplayText(value);
                }
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

    std::vector<ModuleSpec *> ModuleCategory::findSpec(const ModuleLocator &identifier) const {
        __stdc_impl_t;
        std::vector<ModuleSpec *> res;
        auto temp = impl.findModuleSpecs(identifier);
        res.reserve(res.size());
        for (const auto &item : std::as_const(temp)) {
            res.push_back(static_cast<ModuleSpec *>(item));
        }
        return res;
    }

    std::vector<ModuleSpec *> ModuleCategory::specs() const {
        __stdc_impl_t;
        std::shared_lock lock(impl.su_mtx());
        std::vector<ModuleSpec *> res;
        res.reserve(impl.modules.size());
        for (const auto &item : impl.modules) {
            res.push_back(static_cast<ModuleSpec *>(item));
        }
        return res;
    }

    Expected<ModuleSpec *> ModuleCategory::parseSpec(const std::filesystem::path &basePath,
                                                     const JsonValue &config) const {
        __stdc_impl_t;
        if (!config.isObject()) {
            return Error{
                Error::ConfigError,
                R"(invalid inference specification)",
            };
        }
        auto spec = new ModuleSpec(this->category());
        if (const auto exp = spec->_impl->read(basePath, config.toObject()); !exp) {
            delete spec;
            return exp.error();
        }
        return spec;
    }

    Expected<void> ModuleCategory::loadSpecBase(ModuleSpec *spec, const ModuleSpec::State state) {
        __stdc_impl_t;

        const auto spec_impl = spec->_impl.get();
        switch (state) {
        case ModuleSpec::Initialized:
            {
                std::unique_lock lock(impl.su_mtx());
                const auto lib = spec_impl->package;
                const auto it = impl.modules.insert(impl.modules.end(), spec);

                impl.indexes[lib->id][lib->version][spec_impl->id][spec_impl->apiLevel] = it;
                return {};
            }

        case ModuleSpec::Deleted:
            {
                std::unique_lock lock(impl.su_mtx());
                const auto lib = spec_impl->package;
                const auto it = impl.indexes.find(lib->id);
                if (it == impl.indexes.end()) {
                    return {};
                }

                auto &versionMap = it->second;
                const auto it2 = versionMap.find(lib->version);
                if (it2 == versionMap.end()) {
                    return {};
                }

                auto &moduleMap = it2->second;
                const auto it3 = moduleMap.find(spec_impl->id);
                if (it3 == moduleMap.end()) {
                    return {};
                }

                auto &levelMap = it3->second;
                const auto it4 = levelMap.find(spec_impl->apiLevel);
                if (it4 == levelMap.end()) {
                    return {};
                }

                impl.modules.erase(it4->second);
                levelMap.erase(it4);

                if (levelMap.empty()) {
                    moduleMap.erase(it3);
                }
                if (moduleMap.empty()) {
                    versionMap.erase(it2);
                }
                if (versionMap.empty()) {
                    impl.indexes.erase(it);
                }
                return {};
            }
        default:
            break;
        }
        std::abort();
    }

    Expected<void> ModuleCategory::loadSpec(ModuleSpec *spec, const ModuleSpec::State state) {
        __stdc_impl_t;
        switch (state) {
        case ModuleSpec::Initialized:
            return loadSpecBase(spec, state);

        case ModuleSpec::Ready:
        case ModuleSpec::Finished:
            return {};

        case ModuleSpec::Deleted:
            return loadSpecBase(spec, state);
        default:
            break;
        }
        return {};
    }

    std::vector<ModuleSpec *> ModuleCategory::find(const ModuleLocator &loc) const {
        __stdc_impl_t;
        return impl.findModuleSpecs(loc);
    }

    ModuleCategory::ModuleCategory(Impl &impl) : ObjectPool(impl) {}

    ModuleCategory::ModuleCategory(std::string name, PackageManager *mgr) :
        ObjectPool(*new Impl(this, std::move(name), mgr)) {}

} // namespace LangCore
